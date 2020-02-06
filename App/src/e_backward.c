
// #include "ecall_layer_forward.h"
#include "Enclave_u.h"
#include "e_backward.h"
// #include "blas.h" // fill_cpu
#include "types.h"
#include "sgx_err.h"

extern sgx_enclave_id_t EID;

void e_backward_dropout_layer(layer l, network net) {
    if(!net.delta) return ;
    sgx_status_t ret = ecall_backward_dropout_layer(EID, net.train, l.batch, l.inputs, l.probability, l.scale, l.batch * l.inputs,
                                    l.rand, net.input, net.delta);
    if(ret != SGX_SUCCESS) {
        print_error_message(ret);
        return ;
    }
}

void e_backward_connected_layer(layer l, network net) {
    int a_len = l.outputs * net.batch;
    int b_len = net.batch * l.inputs;
    int c_len = l.outputs * l.inputs;
    int ndelta_len = b_len;
    if(!net.delta) {
        ndelta_len = 0;
    }
    sgx_status_t ret = ecall_backward_connected_layer(EID, l.batch_normalize, l.out_c, l.out_w, l.out_h ,net.batch, l.outputs, l.inputs, l.activation, a_len, b_len, c_len, ndelta_len,
        l.output, net.input, l.delta, net.delta, l.weights, l.bias_updates, l.weight_updates,
        l.scale_updates, l.x, l.x_norm,
        l.mean, l.variance, l.mean_delta,
        l.variance_delta, l.scales);
    if(ret != SGX_SUCCESS) {
        print_error_message(ret);
        return ;
    }
}

void e_backward_cost_layer(layer l, network net)
{
    sgx_status_t ret = ecall_backward_cost_layer(EID, l.batch*l.inputs, l.scale, l.delta, net.delta);
    if(ret != SGX_SUCCESS) {
        print_error_message(ret);
        return ;
    }
}

int e_backward_convolutional_layer(layer l, network net) {
    int output_len = l.batch * l.outputs;
    int input_len = l.batch * l.inputs;
    int weight_len = l.size * l.size * l.c * l.n;
    int bias_len = l.n * l.batch;
    sgx_status_t ret = ecall_backward_convolutional_layer(
                    EID, l.batch, l.n, l.size, l.c, l.out_h, l.out_w, l.h, l.w,
                    l.stride, l.pad, bias_len, output_len, input_len, weight_len, l.activation,
                    l.output, net.input, l.delta, net.delta, l.weights, l.bias_updates, l.weight_updates,
                    l.batch_normalize,
                    l.scale_updates, l.x, l.x_norm,
                    l.mean, l.variance, l.mean_delta,
                    l.variance_delta, l.scales
                    );
    if(ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }
}