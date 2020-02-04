#include "enclave.h"


void backward_scale_cpu(float *x_norm, float *delta, int batch, int n, int size, float *scale_updates)
{
    int i,b,f, index;
    for(f = 0; f < n; ++f){
        float sum = 0;
        for(b = 0; b < batch; ++b){
            for(i = 0; i < size; ++i){
                index = i + size*(f + n*b);
                sum += delta[index] * x_norm[index];
            }
        }
        scale_updates[f] += sum;
    }
}

void mean_delta_cpu(float *delta, float *variance, int batch, int filters, int spatial, float *mean_delta)
{

    int i,j,k;
    for(i = 0; i < filters; ++i){
        mean_delta[i] = 0;
        for (j = 0; j < batch; ++j) {
            for (k = 0; k < spatial; ++k) {
                int index = j*filters*spatial + i*spatial + k;
                mean_delta[i] += delta[index];
            }
        }
        mean_delta[i] *= (-1./sqrt(variance[i] + .00001f));
    }
}
void  variance_delta_cpu(float *x, float *delta, float *mean, float *variance, int batch, int filters, int spatial, float *variance_delta)
{

    int i,j,k;
    for(i = 0; i < filters; ++i){
        variance_delta[i] = 0;
        for(j = 0; j < batch; ++j){
            for(k = 0; k < spatial; ++k){
                int index = j*filters*spatial + i*spatial + k;
                variance_delta[i] += delta[index]*(x[index] - mean[i]);
            }
        }
        variance_delta[i] *= -.5 * pow(variance[i] + .00001f, (float)(-3./2.));
    }
}
void normalize_delta_cpu(float *x, float *mean, float *variance, float *mean_delta, float *variance_delta, int batch, int filters, int spatial, float *delta)
{
    int f, j, k;
    for(j = 0; j < batch; ++j){
        for(f = 0; f < filters; ++f){
            for(k = 0; k < spatial; ++k){
                int index = j*filters*spatial + f*spatial + k;
                delta[index] = delta[index] * 1./(sqrt(variance[f] + .00001f)) + variance_delta[f] * 2. * (x[index] - mean[f]) / (spatial * batch) + mean_delta[f]/(spatial*batch);
            }
        }
    }
}


void forward_batchnorm_layer(LAYER_TYPE layer_type, int train, size_t outputs, size_t batch, size_t out_c, size_t out_h, size_t out_w,
                            float* output, float* input, float* mean, float* rolling_mean, float* variance, float* rolling_variance,
                            float* x, float* x_norm, float* scales)
{
    if(layer_type == BATCHNORM) copy_cpu(outputs*batch, input, 1, output, 1);
    copy_cpu(outputs*batch, output, 1, x, 1);
    if(train){
        mean_cpu(output, batch, out_c, out_h*out_w, mean);
        variance_cpu(output, mean, batch, out_c, out_h*out_w, variance);

        scal_cpu(out_c, .99, rolling_mean, 1);
        axpy_cpu(out_c, .01, mean, 1, rolling_mean, 1);
        scal_cpu(out_c, .99, rolling_variance, 1);
        axpy_cpu(out_c, .01, variance, 1, rolling_variance, 1);

        normalize_cpu(output, mean, variance, batch, out_c, out_h*out_w);   
        copy_cpu(outputs*batch, output, 1, x_norm, 1);
    } else {
        normalize_cpu(output, rolling_mean, rolling_variance, batch, out_c, out_h*out_w);
    }
    scale_bias(output, scales, batch, out_c, out_h*out_w);
}


void backward_batchnorm_layer(size_t batch, size_t out_c, size_t out_w, size_t out_h,
                              float *bias_updates, float *delta, float *scale_updates, float *x_norm, float *x,
                              float *mean, float *variance, float *mean_delta, float* variance_delta, float* scales)
{

    backward_scale_cpu(x_norm, delta, batch, out_c, out_w*out_h, scale_updates);

    scale_bias(delta, scales, batch, out_c, out_h*out_w);

    mean_delta_cpu(delta, variance, batch, out_c, out_w*out_h, mean_delta);
    variance_delta_cpu(x, delta, mean, variance, batch, out_c, out_w*out_h, variance_delta);
    normalize_delta_cpu(x, mean, variance, mean_delta, variance_delta, batch, out_c, out_w*out_h, delta);
}
