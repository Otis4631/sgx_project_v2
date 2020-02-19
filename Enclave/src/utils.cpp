#include "enclave.h"

int printf(const char* fmt, ...)
{
    char buf[BUFSIZ] = { '\0' };
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
    return (int)strnlen(buf, BUFSIZ - 1) + 1;
}
float rand_uniform(float min, float max)
{
    if(max < min){
        float swap = min;
        min = max;
        max = swap;
    }
    int tmp;
    sgx_read_rand((uint8_t *)&tmp, 4);
    tmp = abs(tmp);
    return  (tmp * 1.0 / RAND_MAX) * (max - min) + min;
}

void ce_forward(int batch, int classes, float *pred, float *truth, float *delta, float *error)
{

    softmax_cpu(pred, classes, batch, classes, 1, 0, 1, 1, delta); //delta 中暂存softmax的值
    size_t index = 0;
    for(int i = 0; i < batch; i++) {
        index = (i * classes + (int)truth[i] % classes);
        float a = delta[index];
        error[i] = -log(a);
        delta[index] -= 1;
    }
}

void scale_bias(float *output, float *scales, int batch, int n, int size)
{
    int i,j,b;
    for(b = 0; b < batch; ++b){
        for(i = 0; i < n; ++i){
            for(j = 0; j < size; ++j){
                output[(b*n + i)*size + j] *= scales[i];
            }
        }
    }
}
void ecall_activate_array(float *x, int n, ACTIVATION a) {
    crypt_aux((unsigned char*)pass, pass_len, (unsigned char*)x, 4, n);
    activate_array(x, n, a);
    crypt_aux((unsigned char*)pass, pass_len, (unsigned char*)x, 4, n);
}
void backward_bias(float *bias_updates, float *delta, int batch, int n, int size)
{
    int i,b;
    // 遍历batch中每张输入图片
    // 注意，最后的偏置更新值是所有输入图片的总和（多张图片无非就是重复一张图片的操作，求和即可）。
    // 总之：一个卷积核对应一个偏置更新值，该偏置更新值等于batch中所有输入图片累积的偏置更新值，
    // 而每张图片也需要进行偏置更新值求和（因为每个卷积核在每张图片多个位置做了卷积运算，这都对偏置更新值有贡献）以得到每张图片的总偏置更新值。
    for(b = 0; b < batch; ++b){
        // 求和得一张输入图片的总偏置更新值
        for(i = 0; i < n; ++i){
            bias_updates[i] += sum_array(delta+size*(i+b*n), size);
        }
        int k66;
    }
}
void add_bias(float *output, float *biases, int batch, int n, int size)
{
    int i,j,b;
    for(b = 0; b < batch; ++b){
        for(i = 0; i < n; ++i){
            for(j = 0; j < size; ++j){
                output[(b*n + i)*size + j] += biases[i];
            }
        }
    }
}

void ecall_rc4_crypt( unsigned char *key, unsigned long key_len, unsigned char *Data, unsigned long Len) {
    rc4_crypt(key, key_len, Data, Len);
}

float sum_array(float *a, int n)
{
    int i;
    float sum = 0;
    for(i = 0; i < n; ++i) sum += a[i];
    return sum;
}

float mean_array(float *a, int n)
{
    return sum_array(a,n)/n;
}

float variance_array(float *a, int n)
{
    int i;
    float sum = 0;
    float mean = mean_array(a, n);
    for(i = 0; i < n; ++i) sum += (a[i] - mean)*(a[i]-mean);
    float variance = sum/n;
    return variance;
}

void normalize_array(float *a, int n)
{
    int i;
    float mu = mean_array(a,n);
    float sigma = sqrt(variance_array(a,n));
    for(i = 0; i < n; ++i){
        a[i] = (a[i] - mu)/sigma;
    }

}

void ecall_normalize_array(float * a, size_t n, size_t batch){
    n /= batch;
    crypt_aux(pass, pass_len, (unsigned char*) a, sizeof(float) * n, batch);
    int offset = 0;
    for(int i=0; i < batch; i++) {
        offset = i * n;
        normalize_array(a + offset, n);
   }
    crypt_aux(pass, pass_len, (unsigned char*) a, sizeof(float) * n, batch);

}

