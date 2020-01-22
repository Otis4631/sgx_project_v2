#include "enclave.h"


// struct connect_backward_struct {
//     int batch, int outputs, int inputs, ACTIVATION a, 
//     [in, count=outputs * batch] float* output, size_t out_len,
//     [in, out, count=outputs * batch]  float* delta,
//     [in, out, count=] float* bias
// };

void ecall_backward_connected_layer(int batch, int outputs, int inputs, ACTIVATION activation, size_t a_len, size_t b_len, size_t c_len, size_t n_delta_len,
                                    float* output, 
                                    float* input,
                                    float* delta,
                                    float* n_delta,
                                    float* weights,
                                    float* bias_updates,
                                    float* weight_updates)
{
    //a_len: outputs*batch
    //b_len: batch*inputs
    //c_len: outputs*inputs

    crypt_aux((uint8_t *)pass, pass_len, (unsigned char*)input, sizeof(float) * b_len, batch);
    crypt_aux((uint8_t *)pass, pass_len, (unsigned char*)output, sizeof(float) * a_len, batch);
    int i;

    gradient_array(output, a_len, activation, delta);

    for(i = 0; i < batch; ++i){
       // y= alpha*x + y
       // void axpy_cpu(int N, float ALPHA, float *X, int INCX, float *Y, int INCY);

        axpy_cpu(outputs, 1, delta + i*outputs, 1, bias_updates, 1);
    }
    // if(l.batch_normalize){
    //     backward_scale_cpu(l.x_norm, l.delta, l.batch, l.outputs, 1, l.scale_updates);

    //     scale_bias(l.delta, l.scales, l.batch, l.outputs, 1);

    //     mean_delta_cpu(l.delta, l.variance, l.batch, l.outputs, 1, l.mean_delta);
    //     variance_delta_cpu(l.x, l.delta, l.mean, l.variance, l.batch, l.outputs, 1, l.variance_delta);
    //     normalize_delta_cpu(l.x, l.mean, l.variance, l.mean_delta, l.variance_delta, l.batch, l.outputs, 1, l.delta);
    // }

    // 计算当前全连接层的权重更新值
    int m = outputs;
    int k = batch;
    int n = inputs;
    float *a = delta;
    float *b = input;
    float *c = weight_updates;

    gemm(1,0,m,n,k,1,a,m,b,n,1,c,n);

    // 由当前全连接层计算上一层的敏感度图（完成绝大部分计算：当前全连接层敏感度图乘以当前层还未更新的权重）
    m = batch;
    k = outputs;
    n = inputs;

    a = delta;
    b = weights;
    c = n_delta;

    if(c) gemm(0,0,m,n,k,1,a,k,b,n,1,c,n);
}


void ecall_backward_cost_layer(size_t input_size, int scale, 
                                float* delta,
                                float* n_delta)
{
    axpy_cpu(input_size, scale, delta, 1, n_delta, 1);
}