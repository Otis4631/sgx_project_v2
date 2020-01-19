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
sgx_status_t ecall_gemm(sgx_enclave_id_t eid, int TA, int TB, int M, int N, int K, float ALPHA, float* A, int lda, float* B, int ldb, float BETA, float* C, int ldc, int a_size, int b_size, int c_size);
sgx_status_t ecall_activate_array(sgx_enclave_id_t eid, float* x, int n, ACTIVATION a);
sgx_status_t ecall_avgpool_forward(sgx_enclave_id_t eid, int batch, int c, int fig_size, float* input, int input_len, float* output, int output_len);
sgx_status_t ecall_forward_connected_layer(sgx_enclave_id_t eid, int TA, int TB, int M, int N, int K, float ALPHA, float* A, int lda, float* B, int ldb, float BETA, float* C, int ldc, long int a_size, long int b_size, long int c_size, float* bias, int bias_len, ACTIVATION a);
sgx_status_t ecall_forward_maxpool_layer(sgx_enclave_id_t eid, int pad, int h, int w, int out_h, int out_w, int c, int batch, int size, int stride, float* input, int input_len, float* output, int out_len, int* indcies);
sgx_status_t ecall_forward_convolutional_layer(sgx_enclave_id_t eid, int batch, int ic, int h, int w, int size, int stride, int pad, int n_filters, int out_h, int out_w, float* weights, int weight_len, float* input, int in_len, float* output, int out_len, float* biases, int bias_len, ACTIVATION activation);
sgx_status_t ecall_rc4_crypt(sgx_enclave_id_t eid, unsigned char* key, unsigned long int key_len, unsigned char* Data, unsigned long int Len);
sgx_status_t ecall_forward_cost_layer(sgx_enclave_id_t eid, COST_TYPE cost_type, int batch, int in_len, float* input, size_t input_size, float* truth, float* delta, float* output, float* cost);
sgx_status_t ecall_backward_connected_layer(sgx_enclave_id_t eid, int batch, int outputs, int inputs, ACTIVATION a, size_t a_len, size_t b_len, size_t c_len, float* output, float* input, float* delta, float* n_delta, float* bias, float* weights);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
