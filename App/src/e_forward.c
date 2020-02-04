#include "e_forward.h"
#include "Enclave_u.h"
#include "sgx_err.h"
#include "layer.h"
#include "network.h"
#include "blas.h" // fill_cpu
#include "types.h"
typedef layer cost_layer;

extern sgx_enclave_id_t EID;
int e_gemm(int TA, int TB, int M, int N, int K, float ALPHA, 
        float *A, int lda, 
        float *B, int ldb,
        float BETA,
        float *C, int ldc){
            long a_size = lda * M;
            long b_size = ldb * K;
            long c_size = ldc * M;
            double total = a_size + b_size + c_size;
            #ifdef SGX_DEBUG
            printf("Total memory: %.2f MB\n", total / (1024 * 1024));
            #endif
            sgx_status_t ret = ecall_gemm(EID, TA, TB, M, N, K, ALPHA, A, lda, B, ldb, BETA, C, ldc, a_size, b_size, c_size);
            if(ret != SGX_SUCCESS) {
                printf("file:App/gemm.c: ERROR when doing ecall_gemm!\n");
                print_error_message(ret);
                return -1;
            }
}

int e_forward_convolutional_layer(layer l, network net)
{

    fill_cpu(l.outputs*l.batch, 0, l.output, 1);
    int m = l.n;                // 该层卷积核个数
    int k = l.size * l.size * l.c;  // 该层每个卷积核的参数元素个数
    int n = l.out_h * l.out_w;        // 该层每个特征图的尺寸（元素个数）

        // gemm(0,0,m,n,k,1,a,k,b,n,1,c,n);

    long a_size = k * m;
    long b_size = n * k;
    long c_size = n * m;
    sgx_status_t ret = ecall_forward_convolutional_layer(EID, l.batch, l.c, l.h, l.w, l.size, l.stride, l.pad, l.n, l.out_h, l.out_w, 
                                        l.weights, a_size, 
                                        net.input, l.inputs * l.batch,
                                        l.output, l.batch * l.outputs, 
                                        l.biases, l.n,
                                        l.activation);
    if(ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }

}

int e_forward_connected_layer(layer l, network net) {

    int i;
    fill_cpu(l.outputs*l.batch, 0, l.output, 1);

    int m = l.batch;
    int k = l.inputs;
    int n = l.outputs;

    float *a = net.input;
    float *b = l.weights;
    float *c = l.output;

    int lda = k;
    int ldb = k;
    int ldc = n;
    long a_size = lda * m;
    long b_size = ldb * n;
    long c_size = ldc * m;

    if(!l.batch_normalize){
        l.rolling_mean = NULL;
        l.rolling_variance = NULL;
        l.scales = NULL;
        l.x = NULL;
        l.x_norm = NULL;
    }
    
    sgx_status_t ret = 
        ecall_forward_connected_layer(EID, 0, 1, m, n, k, l.batch_normalize, net.train, 
                                        l.rolling_mean, l.rolling_variance, l.scales, l.x,
                                        l.x_norm, a, k, b, k, c, n, a_size, b_size, c_size,
                                        l.biases, l.mean, l.variance,l.activation);
    if(ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }
}


int e_forward_maxpool_layer(layer l, network net){
    ecall_forward_maxpool_layer(EID, l.pad, l.h, l.w, l.out_h, l.out_w, l.c, l.batch, l.size, l.stride, 
                        net.input, l.inputs*l.batch,
                        l.output, l.outputs*l.batch,
                        l.indexes);
}

int e_normalize_array(float *a, size_t arr_len, size_t batch) {
    sgx_status_t ret = ecall_normalize_array(EID, a, arr_len, batch); 
    if(ret != SGX_SUCCESS)
        {
            print_error_message(ret);
            return -1;
        }
}

int e_forward_cost_layer(layer l, network net)
{
    sgx_status_t ret = ecall_forward_cost_layer(EID, l.cost_type, l.batch, l.inputs, net.input, l.batch*l.inputs, net.truth,l.delta, l.output, l.cost); 
    if(ret != SGX_SUCCESS)
        {
            print_error_message(ret);
            return -1;
        }
}
