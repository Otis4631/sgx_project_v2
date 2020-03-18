#pragma once 

#include "Enclave_t.h" /* print_string */
#include <stdarg.h>
#include <stdio.h> /* vsnprintf */
#include <string.h>
#include <sgx_trts.h>

#ifdef __cplusplus
extern "C" {
#endif
    #include "im2col.h"
    #include "e_gemm.h"
    #include "rc4.h"
    #include "e_activation.h"
    #include "types.h"
    #include "blas.h"
    #include "float.h"
    #include "ecall_batchnorm_layer.h"
#ifdef __cplusplus
}
#endif
const uint8_t pass[] = "lizheng";
const size_t pass_len = 8;
void print_string2hex(uint8_t* str, size_t n);
float rand_uniform(float min, float max);
int printf(const char* fmt, ...);
float sum_array(float *a, int n);
void add_bias(float *output, float *biases, int batch, int n, int size);
void backward_bias(float *bias_updates, float *delta, int batch, int n, int size);
void scale_bias(float *output, float *scales, int batch, int n, int size);
void ce_forward(int batch, int classes, float *pred, float *truth, float *delta, float *error);

#ifdef __cplusplus
#include <vector>
using namespace std;
void gen_random_bytes(size_t bytes_len, vector<uint8_t> &tmp);

#include "dnnl_forward.h"
#endif
