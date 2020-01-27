#ifndef ENCLAVE_U_H__
#define ENCLAVE_U_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include <string.h>
#include "sgx_edger8r.h" /* for sgx_status_t etc. */

#include "types.h"

#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OCALL_PRINT_STRING_DEFINED__
#define OCALL_PRINT_STRING_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_print_string, (const char* str));
#endif

sgx_status_t hello(sgx_enclave_id_t eid);
sgx_status_t ecall_normalize_array(sgx_enclave_id_t eid, float* array, size_t arr_len, size_t batch);
sgx_status_t ecall_gemm(sgx_enclave_id_t eid, int TA, int TB, int M, int N, int K, float ALPHA, float* A, int lda, float* B, int ldb, float BETA, float* C, int ldc, int a_size, int b_size, int c_size);
sgx_status_t ecall_activate_array(sgx_enclave_id_t eid, float* x, int n, ACTIVATION a);
sgx_status_t ecall_avgpool_forward(sgx_enclave_id_t eid, int batch, int c, int fig_size, float* input, int input_len, float* output, int output_len);
sgx_status_t ecall_forward_connected_layer(sgx_enclave_id_t eid, int TA, int TB, int M, int N, int K, float ALPHA, float* A, int lda, float* B, int ldb, float BETA, float* C, int ldc, long int a_size, long int b_size, long int c_size, float* bias, int bias_len, ACTIVATION a);
sgx_status_t ecall_forward_maxpool_layer(sgx_enclave_id_t eid, int pad, int h, int w, int out_h, int out_w, int c, int batch, int size, int stride, float* input, int input_len, float* output, int out_len, int* indcies);
sgx_status_t ecall_forward_convolutional_layer(sgx_enclave_id_t eid, int batch, int ic, int h, int w, int size, int stride, int pad, int n_filters, int out_h, int out_w, float* weights, int weight_len, float* input, int in_len, float* output, int out_len, float* biases, int bias_len, ACTIVATION activation);
sgx_status_t ecall_rc4_crypt(sgx_enclave_id_t eid, unsigned char* key, unsigned long int key_len, unsigned char* Data, unsigned long int Len);
sgx_status_t ecall_forward_cost_layer(sgx_enclave_id_t eid, COST_TYPE cost_type, int batch, int in_len, float* input, size_t input_size, float* truth, float* delta, float* output, float* cost);
sgx_status_t ecall_backward_convolutional_layer(sgx_enclave_id_t eid, size_t batch, size_t m, size_t size, size_t ic, size_t out_h, size_t out_w, size_t h, size_t w, size_t stride, size_t pad, size_t bias_len, size_t output_len, size_t input_len, size_t weight_len, ACTIVATION activation, float* output, float* input, float* delta, float* ndelta, float* weight, float* bias_updates, float* weight_updates);
sgx_status_t ecall_backward_cost_layer(sgx_enclave_id_t eid, size_t input_size, int scale, float* delta, float* n_delta);
sgx_status_t ecall_backward_connected_layer(sgx_enclave_id_t eid, int batch, int outputs, int inputs, ACTIVATION a, size_t a_len, size_t b_len, size_t c_len, size_t nd_len, float* output, float* input, float* delta, float* n_delta, float* weights, float* bias_updates, float* weight_updates);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
