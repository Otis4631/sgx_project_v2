#include "enclave.h"

void ecall_backward_dropout_layer( int train, size_t batch, size_t inputs, float probability, float scale, size_t,
                                    float* rand, float* input, float* ndelta)
{
    int i;
    crypt_aux((unsigned char*)pass, pass_len, (unsigned char*)input, sizeof(float) * inputs, batch);
     if(!ndelta) return;
    for(i = 0; i < batch * inputs; ++i){
        float r = rand[i];
        if(r < probability) ndelta[i] = 0;
        else ndelta[i] *= scale;
    }
    
}

void ecall_backward_connected_layer(int bn, size_t out_c, size_t out_w, size_t out_h, 
                                    int batch, int outputs, int inputs, ACTIVATION activation, size_t a_len, size_t b_len, size_t c_len, size_t n_delta_len,
                                    float* output, 
                                    float* input,
                                    float* delta,
                                    float* n_delta,
                                    float* weights,
                                    float* bias_updates,
                                    float* weight_updates,
                                    
                                    float* scale_updates, float* x, float *x_norm,
                                    float* mean, float* variance, float* mean_delta,
                                    float* variance_delta, float* scales)
{
    //a_len: outputs*batch
    //b_len: batch*inputs
    //c_len: outputs*inputs

    crypt_aux((uint8_t *)pass, pass_len, (unsigned char*)input, sizeof(float) * inputs, batch);
    crypt_aux((uint8_t *)pass, pass_len, (unsigned char*)output, sizeof(float) * outputs, batch);



    int i;

    gradient_array(output, a_len, activation, delta);
    backward_bias(bias_updates, delta, batch, out_c, out_w*out_h);
    if(bn){
        crypt_aux((unsigned char*)pass, pass_len, (unsigned char*)x, sizeof(float) * outputs, batch);
        crypt_aux((unsigned char*)pass, pass_len, (unsigned char*)x_norm, sizeof(float) * outputs, batch);
        backward_batchnorm_layer(batch, out_c, out_w, out_h,
                              bias_updates, delta, scale_updates, x_norm, x,
                              mean, variance, mean_delta, variance_delta, scales);   
    }
    // 计算当前全连接层的权重更新值
    int m = outputs;
    int k = batch;
    int n = inputs;
    float *a = delta;
    float *b = input;
    float *c = weight_updates;

    gemm(1,0,m,n,k,1,a,m,b,n,1,c,n);

    // 由当前全连接层计算上一层的敏感度图（完成绝大部分计算：当前全连接层敏感度图乘以当前层还未更新的权重）
    m = batch;
    k = outputs;
    n = inputs;

    a = delta;
    b = weights;
    c = n_delta;

    if(c) gemm(0,0,m,n,k,1,a,k,b,n,1,c,n);
}


void ecall_backward_cost_layer(size_t input_size, int scale, 
                                float* delta,
                                float* n_delta)
{
    axpy_cpu(input_size, scale, delta, 1, n_delta, 1);
}


void ecall_backward_convolutional_layer(size_t batch ,size_t m /*卷积核个数*/, size_t size, size_t ic, size_t out_h, size_t out_w, size_t h, size_t w,
                                        size_t stride, size_t pad,  size_t bias_len, 
                                        size_t output_len, size_t input_len, size_t weight_len, ACTIVATION activation, 
                                        float* output,
                                        float* input,
                                        float* delta,
                                        float* ndelta,
                                        float* weights,
                                        float* bias_updates,
                                        float* weight_updates,
                                        // For Batch Normalization 
                                        int bn,
                                        float* scale_updates, float* x, float *x_norm,
                                        float* mean, float* variance, float* mean_delta,
                                        float* variance_delta, float* scales
                                        ) 
{

    crypt_aux(pass, pass_len, (unsigned char*)input, (sizeof(float) * input_len / batch), batch);
    crypt_aux(pass, pass_len, (unsigned char*)output, (sizeof(float) * output_len / batch), batch);

    int i;
   // int m = n;                // 卷积核个数
    // 每一个卷积核元素个数（包括l.c（l.c为该层网络接受的输入图片的通道数）个通道上的卷积核元素个数总数，比如卷积核尺寸为3*3,
    // 输入图片有3个通道，因为要同时作用于输入的3个通道上，所以实际上这个卷积核是一个立体的，共有3*3*3=27个元素，这些元素都是要训练的参数）
    int n = size * size * ic;
    int k = out_w * out_h;    // 每张输出特征图的元素个数：out_w，out_h是输出特征图的宽高
    float *workspace = (float*)calloc(out_h * out_w * n, sizeof(float));
    gradient_array(output, output_len, activation, delta);
    if(bn){
            size_t outputs = m * out_w * out_h;
            crypt_aux((unsigned char*)pass, pass_len, (unsigned char*)x, sizeof(float) * outputs, batch);
            crypt_aux((unsigned char*)pass, pass_len, (unsigned char*)x_norm, sizeof(float) * outputs, batch);
            backward_batchnorm_layer(batch, m, out_w, out_h,
                                bias_updates, delta, scale_updates, x_norm, x,
                                mean, variance, mean_delta, variance_delta, scales);   
        }else {
        backward_bias(bias_updates, delta, batch, m, k);
    }
    for(i = 0; i < batch; ++i){
        float *a = delta + i * m * k;
        // net.workspace的元素个数为所有层中最大的l.workspace_size（在make_convolutional_layer()计算得到workspace_size的大小，在parse_network_cfg()中动态分配内存，此值对应未使用gpu时的情况）,
        // net.workspace充当一个临时工作空间的作用，存储临时所需要的计算参数，比如每层单张图片重排后的结果（这些参数马上就会参与卷积运算），一旦用完，就会被马上更新（因此该变量的值的更新频率比较大）
        float *b = workspace;
        float *c = weight_updates;

        // 进入本函数之前，在backward_network()函数中，已经将net.input赋值为prev.output，也即若当前层为第l层，net.input此时已经是第l-1层的输出
        float *im = input + i * ic * h * w;

        // 下面两步：im2col_cpu()与gemm()是为了计算当前层的权重更新值（其实也就是误差函数对当前成权重的导数）
        // 将多通道二维图像net.input变成按一定存储规则排列的数组b，以方便、高效地进行矩阵（卷积）计算，详细查看该函数注释（比较复杂），
        // im2col_cput每次仅处理net.input（包含整个batch）中的一张输入图片（对于第一层，则就是读入的图片，对于之后的层，这些图片都是上一层的输出，通道数等于上一层卷积核个数）。
        // 最终重排的b为l.c * l.size * l.size行，l.out_h * l.out_w列。
        // 你会发现在前向forward_convolutional_layer()函数中，也为每层的输入进行了重排，但是很遗憾的是，并没有一个l.workspace把每一层的重排结果保存下来，而是统一存储到net.workspace中，
        // 并被不断擦除更新，那为什么不保存呢？保存下来不是省掉一大笔额外重复计算开销？原因有两个：1）net.workspace中只存储了一张输入图片的重排结果，所以重排下张图片时，马上就会被擦除，
        // 当然你可能会想，那为什么不弄一个l.worspaces将每层所有输入图片的结果保存呢？这引出第二个原因；2）计算成本是降低了，但存储空间需求急剧增加，想想每一层都有l.batch张图，且每张都是多通道的，
        // 重排后其元素个数还会增多，这个存储量搁谁都受不了，如果一个batch有128张图，输入图片尺寸为400*400，3通道，网络有16层（假设每层输入输出尺寸及通道数都一样），那么单单为了存储这些重排结果，
        // 就需要128*400*400*3*16*4/1024/1024/1024 = 3.66G，所以为了权衡，只能重复计算！
        im2col_cpu(im, ic, h, w, 
                size, stride, pad, b);
        gemm(0,1,m,n,k,1,a,k,b,k,1,c,n);
         if(ndelta){
            // 当前层还未更新的权重
            a = weights;
            // 每次循环仅处理一张输入图，注意移位（l.delta的维度为l.batch * l.out_c * l.out_w * l.out_h）（注意l.n = l.out_c，另外提一下，对整个网络来说，每一层的l.batch其实都是一样的）
            b = delta + i * m * k;

            // net.workspace和上面一样，还是一张输入图片的重排，不同的是，此处我们只需要这个容器，而里面存储的值我们并不需要，在后面的处理过程中，
            // 会将其中存储的值一一覆盖掉（尺寸维持不变，还是(l.c * l.size * l.size) * (l.out_h * l.out_w）
            c = workspace;

            // 相比上一个gemm，此处的a对应上一个的c,b对应上一个的a，c对应上一个的b，即此处a,b,c的行列分别为：
            // a: (l.n) * (l.c*l.size*l.size)，表示当前层所有权重系数
            // b: (l.out_c) * (l.out_h*l*out_w)（注意：l.n = l.out_c），表示当前层的敏感度图
            // c: (l.c * l.size * l.size) * (l.out_h * l.out_w)，表示上一层的敏感度图（其元素个数等于上一层网络单张输入图片的所有输出元素个数），
            // 此时要完成a * b + c计算，必须对a进行转置（否则行列不匹配），因故调用gemm_tn()函数。
            // 此操作含义是用：用当前层还未更新的权重值对敏感度图做卷积，得到包含上一层所有敏感度信息的矩阵，但这不是上一层最终的敏感度图，
            // 因为此时的c，也即net.workspace的尺寸为(l.c * l.size * l.size) * (l.out_h * l.out_w)，明显不是上一层的输出尺寸l.c*l.w*l.h，
            // 接下来还需要调用col2im_cpu()函数将其恢复至l.c*l.w*l.h（可视为l.c行，l.w*l.h列），这才是上一层的敏感度图（实际还差一个环节，
            // 这个环节需要等到下一次调用backward_convolutional_layer()才完成：将net.delta中每个元素乘以激活函数对加权输入的导数值）。
            // 完成gemm这一步，如col2im_cpu()中注释，是考虑了多个卷积核导致的一对多关系（上一层的一个输出元素会流入到下一层多个输出元素中），
            // 接下来调用col2im_cpu()则是考虑卷积核重叠（步长较小）导致的一对多关系。
            gemm(1,0,n,k,m,1,a,n,b,k,0,c,k);

            // 对c也即net.workspace进行重排，得到的结果存储在net.delta中，每次循环只会处理一张输入图片，因此，此处只会得到一张输入图产生的敏感图（注意net.delta的移位）,
            // 整个循环结束后，net.delta的总尺寸为l.batch * l.h * l.w * l.c，这就是上一层网络整个batch的敏感度图，可视为有l.batch行，l.h*l.w*l.c列，
            // 每行存储了一张输入图片所有输出特征图的敏感度
            // col2im_cpu()函数中会调用col2im_add_pixel()函数，该函数中使用了+=运算符，也即该函数要求输入的net.delta的初始值为0,而在gradient_array()中注释到l.delta的元素是不为0（也不能为0）的，
            // 看上去是矛盾的，实则不然，gradient_array()使用的l.delta是当前层的敏感度图，而在col2im_cpu()使用的net.delta是上一层的敏感度图，正如gradient_array()中所注释的，
            // 当前层l.delta之所以不为0,是因为从后面层反向传播过来的，对于上一层，显然还没有反向传播到那，因此net.delta的初始值都是为0的（注意，每一层在构建时，就为其delta动态分配了内存，
            // 且在前向传播时，为每一层的delta都赋值为0,可以参考network.c中forward_network()函数）
            col2im_cpu(workspace, ic, h, w, size, stride, pad, ndelta + i * ic * h * w);
        }
     }
}

