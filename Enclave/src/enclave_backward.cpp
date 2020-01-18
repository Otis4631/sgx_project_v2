#include "enclave.h"


// struct connect_backward_struct {
//     int batch, int outputs, int inputs, ACTIVATION a, 
//     [in, count=outputs * batch] float* output, size_t out_len,
//     [in, out, count=outputs * batch]  float* delta,
//     [in, out, count=] float* bias
// };

void ecall_backward_connected_layer(int batch, int outputs, int inputs, ACTIVATION activation, size_t a_len, size_t b_len, size_t c_len,
                                    float* output, 
                                    float* input,
                                    float* delta,
                                    float* n_delta,

                                    float* bias,
                                    //float* bias_updates,

                                    float* weights)
{
    //a_len: outputs*batch
    //b_len: batch*inputs
    //c_len: outputs*inputs

    float *bias_updates = new float[outputs]();
    float *weight_updates = new float[c_len]();
    int i;
    // 完成当前层敏感度图的计算：当前全连接层下一层不管是什么类型的网络，都会完成当前层敏感度图的绝大部分计算（上一层敏感度乘以上一层与当前层之间的权重）
    // （注意是反向传播），此处只需要再将l.delta中的每一个元素乘以激活函数对加权输入的导数即可
    // gradient_array()函数完成激活函数对加权输入的导数，并乘以之前得到的l.delta，得到当前层最终的l.delta（误差函数对加权输入的导数）
    gradient_array(output, a_len, activation, delta);

    // 计算当前全连接层的偏置更新值
    // 相比于卷积层的偏置更新值，此处更为简单（卷积层中有专门的偏置更新值计算函数，主要原因是卷积核在图像上做卷积即权值共享增加了复杂度，而全连接层没有权值共享），
    // 只需调用axpy_cpu()函数就可以完成。误差函数对偏置的导数实际就等于以上刚求完的敏感度值，因为有多张图片，需要将多张图片的效果叠加，故而循环调用axpy_cpu()函数，
    // 不同于卷积层每个卷积核才有一个偏置参数，全连接层是每个输出元素就对应有一个偏置参数，共有l.outputs个，每次循环将求完一张图片所有输出的偏置更新值。
    // l.bias_updates虽然没有明显的初始化操作，但其在make_connected_layer()中是用calloc()动态分配内存的，因此其已经全部初始化为0值。
    // 循环结束后，最终会把每一张图的偏置更新值叠加，因此，最终l.bias_updates中每一个元素的值是batch中所有图片对应输出元素偏置更新值的叠加。
    for(i = 0; i < batch; ++i){
       // y= alpha*x + y
       // void axpy_cpu(int N, float ALPHA, float *X, int INCX, float *Y, int INCY);

        axpy_cpu(outputs, 1, delta + i*outputs, 1, bias_updates, 1);
    }
    // if(l.batch_normalize){
    //     backward_scale_cpu(l.x_norm, l.delta, l.batch, l.outputs, 1, l.scale_updates);

    //     scale_bias(l.delta, l.scales, l.batch, l.outputs, 1);

    //     mean_delta_cpu(l.delta, l.variance, l.batch, l.outputs, 1, l.mean_delta);
    //     variance_delta_cpu(l.x, l.delta, l.mean, l.variance, l.batch, l.outputs, 1, l.variance_delta);
    //     normalize_delta_cpu(l.x, l.mean, l.variance, l.mean_delta, l.variance_delta, l.batch, l.outputs, 1, l.delta);
    // }

    // 计算当前全连接层的权重更新值
    int m = outputs;
    int k = batch;
    int n = inputs;
    float *a = delta;
    float *b = input;
    float *c = weight_updates;

    // a：当前全连接层敏感度图，维度为l.batch*l.outputs
    // b：当前全连接层所有输入，维度为l.batch*l.inputs
    // c：当前全连接层权重更新值，维度为l.outputs*l.inputs（权重个数）
    // 由行列匹配规则可知，需要将a转置，故而调用gemm_tn()函数，转置a实际上是想把batch中所有图片的影响叠加。
    // 全连接层的权重更新值的计算也相对简单，简单的矩阵乘法即可完成：当前全连接层的敏感度图乘以当前层的输入即可得到当前全连接层的权重更新值，
    // （当前层的敏感度是误差函数对于加权输入的导数，所以再乘以对应输入值即可得到权重更新值）
    // m：a'的行，值为l.outputs，含义为每张图片输出的元素个数
    // n：b的列数，值为l.inputs，含义为每张输入图片的元素个数
    // k：a’的列数，值为l.batch，含义为一个batch中含有的图片张数
    // 最终得到的c维度为l.outputs*l.inputs，对应所有权重的更新值
    gemm(1,0,m,n,k,1,a,m,b,n,1,c,n);

    // 由当前全连接层计算上一层的敏感度图（完成绝大部分计算：当前全连接层敏感度图乘以当前层还未更新的权重）
    m = batch;
    k = outputs;
    n = inputs;

    a = delta;
    b = weights;
    c = n_delta;

    // 一定注意此时的c等于net.delta，已经在network.c中的backward_network()函数中赋值为上一层的delta
    // a：当前全连接层敏感度图，维度为l.batch*l.outputs
    // b：当前层权重（连接当前层与上一层），维度为l.outputs*l.inputs
    // c：上一层敏感度图（包含整个batch），维度为l.batch*l.inputs
    // 由行列匹配规则可知，不需要转置。由全连接层敏感度图计算上一层的敏感度图也很简单，直接利用矩阵相乘，将当前层l.delta与当前层权重相乘就可以了，
    // 只需要注意要不要转置，拿捏好就可以，不需要像卷积层一样，需要对权重或者输入重排！
    // m：a的行，值为l.batch，含义为一个batch中含有的图片张数
    // n：b的列数，值为l.inputs，含义为每张输入图片的元素个数
    // k：a的列数，值为l.outputs，含义为每张图片输出的元素个数
    // 最终得到的c维度为l.bacth*l.inputs（包含所有batch）
    if(c) gemm(0,0,m,n,k,1,a,k,b,n,1,c,n);
}
