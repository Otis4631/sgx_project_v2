

#include "enclave.h"
extern "C"{
    #include "ecall_batchnorm_layer.h"

}
#include "types.h"


void ecall_gemm(int TA, int TB, int M, int N, int K, float ALPHA, 
        float *A, int lda, 
        float *B, int ldb,
        float BETA,
        float *C, int ldc, int a_size, int b_size, int c_size) {

            crypt_aux((unsigned char*)pass, pass_len, (unsigned char*)B, 4, b_size);
            //rc4_crypt("lizheng", 7, C, c_size);
            gemm(TA,  TB,  M, N, K, ALPHA, A, lda, B, ldb, BETA, C, ldc);
            crypt_aux((unsigned char*)pass, pass_len, (unsigned char*)C, 4, c_size);
}

void ecall_avgpool_forward(int batch, int c, int fig_size, float* input, int input_len, float* output, int output_len) {
        crypt_aux((unsigned char*)pass, pass_len, (unsigned char*)input, sizeof(float), input_len);
        int b,k,i;
        for(b = 0; b < batch; ++b){
            for(k = 0; k < c; ++k){
                int out_index = k + b*c;
                output[out_index] = 0;

                for(i = 0; i < fig_size; ++i){
                    int in_index = i + fig_size*(k + b*c);
                    output[out_index] += input[in_index];
                }
                output[out_index] /= fig_size;
            }
        }
        crypt_aux((unsigned char*)pass, pass_len, (unsigned char*)output, sizeof(float), output_len);
}

void ecall_forward_connected_layer( int TA, int TB, int batch, int outputs, int inputs, int batch_normalize, int train, 
                                    float *rolling_mean, float *rolling_variance, float *scales, float *x, float *x_norm, 
                                    float *input, int lda, 
                                    float *weights, int ldb,
                                    float *output, int ldc, 
                                    long a_size, long b_size, long c_size,
                                    float * biases,
                                    float *mean,
                                    float *variance,
                                    ACTIVATION a){
            int M = batch;
            int K = inputs;
            int N = outputs;

            crypt_aux((unsigned char*)pass, pass_len, (unsigned char*)input, sizeof(float) * inputs, batch);
            gemm(0, 1, M, N, K, 1, input, K, weights, K, 1, output, N);
            if (batch_normalize){
                forward_batchnorm_layer(CONNECTED, train, outputs, batch, outputs, 1, 1,
                            output, input, mean, rolling_mean, variance, rolling_variance,
                            x, x_norm, scales);
            }
            add_bias(output, biases, batch, outputs, 1);
            activate_array(output, c_size, a);
            crypt_aux((unsigned char*)pass, pass_len, (unsigned char*)output, sizeof(float) * N, batch);
}

void ecall_forward_maxpool_layer(int pad, int raw_h, int raw_w, int out_h, int out_w, int c, int batch, int size, int stride, 
                                float *input, int input_len,
                                float *output, int out_len,
                                int* indices)
{
    crypt_aux((unsigned char*)pass, pass_len, (unsigned char*)input, sizeof(float) * raw_h * raw_w * c, batch);
    int b,i,j,k,m,n;
    // 初始偏移设定为四周补0长度的负值
    int w_offset = -pad;
    int h_offset = -pad;

    // 获取当前层的输出尺寸
    int h = out_h;
    int w = out_w;

    // 获取当前层输入图像的通道数，为什么是输入通道数？不应该为输出通道数吗？实际二者没有区别，对于最大池化层来说，输入有多少通道，输出就有多少通道！

    // 遍历batch中每一张输入图片，计算得到与每一张输入图片具有相同通道数的输出图
    for(b = 0; b < batch; ++b){
        // 对于每张输入图片，将得到通道数一样的输出图，以输出图为基准，按输出图通道，行，列依次遍历
        // （这对应图像在output的存储方式，每张图片按行铺排成一大行，然后图片与图片之间再并成一行）。
        // 以输出图为基准进行遍历，最终循环的总次数刚好覆盖池化核在输入图片不同位置进行池化操作。
        for(k = 0; k < c; ++k){
            for(i = 0; i < h; ++i){
                for(j = 0; j < w; ++j){

                    // out_index为输出图中的索引：out_index = b * c * w * h + k * w * h + h * w + w，展开写可能更为清晰些
                    int out_index = j + w*(i + h*(k + c*b));

                    float max = -FLT_MAX;   // FLT_MAX为c语言中float.h定义的对大浮点数，此处初始化最大元素值为最小浮点数
                    int max_i = -1;         // 最大元素值的索引初始化为-1

                    // 下面两个循环回到了输入图片，计算得到的cur_h以及cur_w都是在当前层所有输入元素的索引，内外循环的目的是找寻输入图像中，
                    // 以(h_offset + i*l.stride, w_offset + j*l.stride)为左上起点，尺寸为l.size池化区域中的最大元素值max及其在所有输入元素中的索引max_i
                    for(n = 0; n < size; ++n){
                        for(m = 0; m < size; ++m){
                            // cur_h，cur_w是在所有输入图像中第k通道中的cur_h行与cur_w列，index是在所有输入图像元素中的总索引。
                            // 为什么这里少一层对输入通道数的遍历循环呢？因为对于最大池化层来说输入与输出通道数是一样的，并在上面的通道数循环了！
                            int cur_h = h_offset + i*stride + n;
                            int cur_w = w_offset + j*stride + m;
                            int index = raw_w*(cur_h + raw_h*(k + b*c)) + cur_w ;

                            // 边界检查：正常情况下，是不会越界的，但是如果有补0操作，就会越界了，这里的处理方式是直接让这些元素值为-FLT_MAX
                            // （注意虽然称之为补0操作，但实际不是补0），总之，这些补的元素永远不会充当最大元素值。
                            int valid = (cur_h >= 0 && cur_h < raw_h &&
                                         cur_w >= 0 && cur_w < raw_w);
                            float val = (valid != 0) ? input[index] : -FLT_MAX;

                            // 记录这个池化区域中的最大的元素值及其在所有输入元素中的总索引
                            max_i = (val > max) ? index : max_i;
                            max   = (val > max) ? val   : max;
                        }
                    }
                    // 由此得到最大池化层每一个输出元素值及其在所有输入元素中的总索引。
                    // 为什么需要记录每个输出元素值对应在输入元素中的总索引呢？因为在下面的反向过程中需要用到，在计算当前最大池化层上一层网络的敏感度时，
                    // 需要该索引明确当前层的每个元素究竟是取上一层输出（也即上前层输入）的哪一个元素的值，具体见下面backward_maxpool_layer()函数的注释。
                    output[out_index] = max;
                    indices[out_index] = max_i;
                }
            }
        }
    }
    crypt_aux((unsigned char*)pass, pass_len, (unsigned char*)output, sizeof(float) * out_h * out_w * c, batch);
}


void ecall_forward_convolutional_layer(int batch,int ic, int h, int w, int size, int stride, int pad, int n_filters, int out_h, int out_w, 
                                       float * weights, int weight_len,
                                       float * input, int in_len,
                                       float * output, int out_len,
                                       float * biases, int bias_len,
                                       ACTIVATION activation) {

    int i;
    int m = n_filters;                // 该层卷积核个数
    int k = size * size * ic;  // 该层每个卷积核的参数元素个数
    int n = out_h * out_w;        // 该层每个特征图的尺寸（元素个数）
    int fig_size = h * ic * w;
    crypt_aux((unsigned char*)pass, pass_len, (unsigned char*)input, sizeof(float) * fig_size, batch);
    float *a = weights;       // 所有卷积核（也即权重），元素个数为l.n*l.c*l.size*l.size，按行存储，共有l*n行，l.c*l.size*l.size列
    float *b = (float*)calloc(out_h * out_w * size * size * ic, sizeof(float));
    float *c = output;        // 存储一张输入图片（多通道）所有的输出特征图（输入图片是多通道的，输出图片也是多通道的，有多少个卷积核就有多少个通道，每个卷积核得到一张特征图即为一个通道）
    
    for(i = 0; i < batch; ++i){
       
        im2col_cpu(input, ic, h, w, 
                size, stride, pad, b);
       // printf("conv input: %.4f\n", input[132]);
        gemm(0,0,m,n,k,1,a,k,b,n,1,c,n);
       // printf("conv out: %.4f\n", c[5]);

        c += n * m;
        input += ic * h * w;
    }

    add_bias(output, biases, batch, n_filters, out_h * out_w);
    activate_array(output, m * n * batch, activation);
    crypt_aux((unsigned char*)pass, pass_len, (unsigned char*)output, sizeof(float) * n * m , batch);
}

void ecall_forward_cost_layer(COST_TYPE cost_type, int batch, int in_len,
                              float* input, size_t input_size,
                              float* truth, 
                              float* delta,
                              float* output,
                              float* cost){
    crypt_aux((uint8_t *)pass, pass_len, (uint8_t*)input, sizeof(float) * in_len, batch);
    crypt_aux((uint8_t *)pass, pass_len, (uint8_t*)truth, sizeof(float) * in_len, batch);

    if(cost_type == SMOOTH){
        smooth_l1_cpu(input_size, input, truth, delta, output);
    }else if(cost_type == L1){
        l1_cpu(input_size, input, truth, delta, output);
    } else {
        l2_cpu(input_size, input, truth, delta, output);
    }
    cost[0] = sum_array(output, input_size);
}

