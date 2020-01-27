#pragma once 

#include "Enclave_t.h" /* print_string */
#include <stdarg.h>
#include <stdio.h> /* vsnprintf */
#include <string.h>

extern "C" {
    #include "im2col.h"
    #include "e_gemm.h"
    #include "rc4.h"
    #include "e_activation.h"
    #include "types.h"
    #include "blas.h"
    #include "float.h"
}
const uint8_t pass[] = "lizheng";
const size_t pass_len = 8;
int printf(const char* fmt, ...);
float sum_array(float *a, int n);
void add_bias(float *output, float *biases, int batch, int n, int size);
void backward_bias(float *bias_updates, float *delta, int batch, int n, int size);