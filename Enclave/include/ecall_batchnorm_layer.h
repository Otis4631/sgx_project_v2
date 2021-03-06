#ifndef ECALL_BATCHNORM_LAYER_H
#define ECALL_BATCHNORM_LAYER_H

//#include "image.h"
#include "layer.h"
#include "types.h"

void forward_batchnorm_layer(LAYER_TYPE layer_type, int train, size_t outputs, size_t batch, size_t out_c, size_t out_h, size_t out_w,
                            float* output, float* input, float* mean, float* rolling_mean, float* variance, float* rolling_variance,
                            float* x, float* x_norm, float* scales);

void backward_batchnorm_layer(size_t batch, size_t out_c, size_t out_w, size_t out_h,
                              float *bias_updates, float *delta, float *scale_updates, float *x_norm, float *x,
                              float *mean, float *variance, float *mean_delta, float* variance_delta, float* scales);


#endif
