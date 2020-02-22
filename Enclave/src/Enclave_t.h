#ifndef ENCLAVE_T_H__
#define ENCLAVE_T_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include "sgx_edger8r.h" /* for sgx_ocall etc. */

#include "types.h"
#include "types.h"

#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ecall_layer
#define _ecall_layer
typedef struct ecall_layer {
	size_t batch;
	size_t inputs;
	size_t outputs;
	size_t h;
	size_t w;
	size_t c;
	size_t out_h;
	size_t out_w;
	size_t out_c;
	int batch_normalize;
	ACTIVATION a;
	size_t input_len;
	float* input;
	size_t output_len;
	float* output;
	size_t weight_len;
	float* weights;
	size_t bias_len;
	float* biases;
	size_t rolling_len;
	float* rolling_mean;
	float* rolling_variance;
	size_t x_len;
	float* x;
	float* x_norm;
} ecall_layer;
#endif

void ecall_forward_dropout_layer(int train, size_t batch, size_t inputs, float probability, float scale, size_t in_len, float* rand, float* input);
void ecall_normalize_array(float* array, size_t arr_len, size_t batch);
void ecall_gemm(int TA, int TB, int M, int N, int K, float ALPHA, float** A, int lda, float** B, int ldb, float BETA, float** C, int ldc);
void ecall_activate_array(float* x, int n, ACTIVATION a);
void ecall_avgpool_forward(int batch, int c, int fig_size, float* input, int input_len, float* output, int output_len);
void ecall_forward_connected_layer(int TA, int TB, int M, int outputs, int K, int BN, int train, float* rolling_mean, float* rolling_variance, float* scales, float* x, float* x_norm, float* A, int lda, float* B, int ldb, float* C, int ldc, long int a_size, long int b_size, long int c_size, float* bias, float* mean, float* variance, ACTIVATION a);
void ecall_forward_maxpool_layer(int pad, int h, int w, int out_h, int out_w, int c, int batch, int size, int stride, float* input, int input_len, float* output, int out_len, int* indcies);
void ecall_forward_convolutional_layer(int batch, int ic, int h, int w, int size, int stride, int pad, int n_filters, int out_h, int out_w, float* weights, int weight_len, float* input, int in_len, float* output, int out_len, float* biases, int bias_len, int batch_normalize, int train, int outputs, float* rolling_mean, float* rolling_variance, float* scales, float* x, float* x_norm, float* mean, float* variance, ACTIVATION activation);
void ecall_rc4_crypt(unsigned char* key, unsigned long int key_len, unsigned char* Data, unsigned long int Len);
void ecall_forward_cost_layer(COST_TYPE cost_type, int batch, int in_len, float* input, size_t input_size, float* truth, float* delta, float* output, float* cost);
void ecall_backward_dropout_layer(int train, size_t batch, size_t inputs, float probability, float scale, size_t in_len, float* rand, float* input, float* ndelta);
void ecall_backward_convolutional_layer(size_t batch, size_t out_c, size_t size, size_t ic, size_t out_h, size_t out_w, size_t h, size_t w, size_t stride, size_t pad, size_t bias_len, size_t output_len, size_t input_len, size_t weight_len, ACTIVATION activation, float* output, float* input, float* delta, float* ndelta, float* weight, float* bias_updates, float* weight_updates, int bn, float* scale_updates, float* x, float* x_norm, float* mean, float* variance, float* mean_delta, float* variance_delta, float* scales);
void ecall_backward_cost_layer(size_t input_size, int scale, float* delta, float* n_delta);
void ecall_backward_connected_layer(int bn, size_t out_c, size_t out_w, size_t out_h, int batch, int outputs, int inputs, ACTIVATION a, size_t a_len, size_t b_len, size_t c_len, size_t nd_len, float* output, float* input, float* delta, float* n_delta, float* weights, float* bias_updates, float* weight_updates, float* scale_updates, float* x, float* x_norm, float* mean, float* variance, float* mean_delta, float* mean_variance, float* scale);

sgx_status_t SGX_CDECL ocall_print_string(const char* str);
sgx_status_t SGX_CDECL gemm_segmentation(int TA, int TB, int M, int N, int K, float ALPHA, float** A, int lda, float** B, int ldb, float BETA, float** C, int ldc);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
