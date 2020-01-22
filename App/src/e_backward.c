
// #include "ecall_layer_forward.h"
#include "Enclave_u.h"
#include "e_backward.h"
// #include "blas.h" // fill_cpu
#include "types.h"

extern sgx_enclave_id_t EID;

//  public void ecall_backward_connected_layer(int batch, int outputs, int inputs, ACTIVATION a, size_t a_len, size_t b_len, size_t c_len,
//                                     [in, count=a_len]           float* output, 
//                                     [in, count=b_len]           float* input,
//                                     [in, out, count=a_len]      float* delta,
//                                     [in, out, count=b_len]      float* n_delta,
//                                     [in, out, count=outputs]    float* bias_updates,
//                                     [in, out, count=c_len]      float* weight_updates);




void e_backward_connected_layer(layer l, network net) {
    int a_len = l.outputs * net.batch;
    int b_len = net.batch * l.inputs;
    int c_len = l.outputs * l.inputs;
    int ndelta_len = b_len;
    if(!net.delta){
        ndelta_len = 0;
    }
    sgx_status_t ret = ecall_backward_connected_layer(EID, net.batch, l.outputs, l.inputs, l.activation, a_len, b_len, c_len, ndelta_len,
        l.output, net.input, l.delta, net.delta, l.weights, l.bias_updates, l.weight_updates);
    if(ret != SGX_SUCCESS) {
        printf("something goes wrong with backward connect layer!\n");
        return ;
    }
}

void e_backward_cost_layer(layer l, network net)
{
     sgx_status_t ret = ecall_backward_cost_layer(EID, l.batch*l.inputs, l.scale, l.delta, net.delta);
    if(ret != SGX_SUCCESS) {
        printf("something goes wrong with backward connect layer!\n");
        return ;
    }
}