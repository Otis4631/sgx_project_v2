#include "cost_layer.h"
#include "utils.h"
#include "cuda.h"
#include "blas.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "e_forward.h"
#include "e_backward.h"
/*
** 支持四种cost functions
** L1    即误差的绝对值之和
** SSE   即L2，误差的平方和（可以查看blas.c中的l2_cpu()函数，没有乘以1/2），为绝大部分网络采用,
         全称应该是the sum of squares due to error（参考的：http://blog.sina.com.cn/s/blog_628033fa0100kjjy.html）
** MASKED 目前只发现在darknet9000.cfg中使用
** SMOOTH 
*/
COST_TYPE get_cost_type(char *s)
{
    if (strcmp(s, "sse")==0) return SSE;
    if (strcmp(s, "masked")==0) return MASKED;
    if (strcmp(s, "smooth")==0) return SMOOTH;
    if (strcmp(s, "L1")==0) return L1;
    if (strcmp(s, "ce") == 0) return CE;
    fprintf(stderr, "Couldn't find cost type %s, going with SSE\n", s);
    return SSE;
}

char *get_cost_string(COST_TYPE a)
{
    switch(a){
        case SSE:
            return "sse";
        case MASKED:
            return "masked";
        case SMOOTH:
            return "smooth";
        case L1:
            return "L1";
        case CE:
            return "ce";
    }
    return "sse";
}

cost_layer make_cost_layer(int batch, int inputs, COST_TYPE cost_type, float scale, int sgx)
{
    fprintf(stderr, "cost                                           %4d\n",  inputs);
    cost_layer l = {0};
    l.type = COST;

    l.scale = scale;
    l.batch = batch;
    l.inputs = inputs;
    l.outputs = inputs;
    l.cost_type = cost_type;
    l.delta = calloc(inputs * batch, sizeof(float));
    l.output = calloc(inputs * batch, sizeof(float));
    l.cost = calloc(1, sizeof(float));
    l.sgx = sgx;
    l.forward = forward_cost_layer;
    l.backward = backward_cost_layer;
    if(l.sgx){
        l.backward = e_backward_cost_layer;
        l.forward = e_forward_cost_layer;
    }
    #ifdef GPU
    l.forward_gpu = forward_cost_layer_gpu;
    l.backward_gpu = backward_cost_layer_gpu;

    l.delta_gpu = cuda_make_array(l.output, inputs*batch);
    l.output_gpu = cuda_make_array(l.delta, inputs*batch);
    #endif
    return l;
}

void resize_cost_layer(cost_layer *l, int inputs)
{
    l->inputs = inputs;
    l->outputs = inputs;
    l->delta = realloc(l->delta, inputs*l->batch*sizeof(float));
    l->output = realloc(l->output, inputs*l->batch*sizeof(float));
#ifdef GPU
    cuda_free(l->delta_gpu);
    cuda_free(l->output_gpu);
    l->delta_gpu = cuda_make_array(l->delta, inputs*l->batch);
    l->output_gpu = cuda_make_array(l->output, inputs*l->batch);
#endif
}

float singe_to_oh(int classes, int label, float* oh){
    for(int i = 0; i < classes; i++) {
        if(i == label)
            oh[i] = 1;
        else
            oh[i] = 0;
    }
}

void ce_forward(int batch, int classes, float *pred, float *truth, float *delta, float *error)
{

    softmax_cpu(pred, classes, batch, classes, 1, 0, 1, 1, delta); //delta 中暂存softmax的值
    size_t index = 0;
    for(int i = 0; i < batch; i++) {
        index = i * classes + (int)truth[i];
        float a = delta[index];
        error[i] = -log(a);
        delta[index] -= 1;
    }
}

void forward_cost_layer(cost_layer l, network net)
{
    if (!net.truth) return;
    if(l.cost_type == MASKED){
        int i;
        for(i = 0; i < l.batch*l.inputs; ++i){
            if(net.truth[i] == SECRET_NUM) net.input[i] = SECRET_NUM;
        }
    }
    if(l.cost_type == SMOOTH){
        smooth_l1_cpu(l.batch*l.inputs, net.input, net.truth, l.delta, l.output);
    } else if(l.cost_type == L1){
        l1_cpu(l.batch*l.inputs, net.input, net.truth, l.delta, l.output);
    } else if(l.cost_type == CE){
        ce_forward(l.batch, l.inputs, net.input, net.truth, l.delta, l.output);
    } else {
        l2_cpu(l.batch*l.inputs, net.input, net.truth, l.delta, l.output);
    }
    l.cost[0] = sum_array(l.output, l.batch*l.inputs);
}

void backward_cost_layer(const cost_layer l, network net)
{
    axpy_cpu(l.batch*l.inputs, l.scale, l.delta, 1, net.delta, 1);
}

#ifdef GPU

void pull_cost_layer(cost_layer l)
{
    cuda_pull_array(l.delta_gpu, l.delta, l.batch*l.inputs);
}

void push_cost_layer(cost_layer l)
{
    cuda_push_array(l.delta_gpu, l.delta, l.batch*l.inputs);
}

int float_abs_compare (const void * a, const void * b)
{
    float fa = *(const float*) a;
    if(fa < 0) fa = -fa;
    float fb = *(const float*) b;
    if(fb < 0) fb = -fb;
    return (fa > fb) - (fa < fb);
}

void forward_cost_layer_gpu(cost_layer l, network net)
{
    if (!net.truth) return;
    if(l.smooth){
        scal_ongpu(l.batch*l.inputs, (1-l.smooth), net.truth_gpu, 1);
        add_ongpu(l.batch*l.inputs, l.smooth * 1./l.inputs, net.truth_gpu, 1);
    }
    if (l.cost_type == MASKED) {
        mask_ongpu(l.batch*l.inputs, net.input_gpu, SECRET_NUM, net.truth_gpu);
    }

    if(l.cost_type == SMOOTH){
        smooth_l1_gpu(l.batch*l.inputs, net.input_gpu, net.truth_gpu, l.delta_gpu, l.output_gpu);
    } else if (l.cost_type == L1){
        l1_gpu(l.batch*l.inputs, net.input_gpu, net.truth_gpu, l.delta_gpu, l.output_gpu);
    } else {
        l2_gpu(l.batch*l.inputs, net.input_gpu, net.truth_gpu, l.delta_gpu, l.output_gpu);
    }

    if(l.ratio){
        cuda_pull_array(l.delta_gpu, l.delta, l.batch*l.inputs);
        qsort(l.delta, l.batch*l.inputs, sizeof(float), float_abs_compare);
        int n = (1-l.ratio) * l.batch*l.inputs;
        float thresh = l.delta[n];
        thresh = 0;
        printf("%f\n", thresh);
        supp_ongpu(l.batch*l.inputs, thresh, l.delta_gpu, 1);
    }

    if(l.thresh){
        supp_ongpu(l.batch*l.inputs, l.thresh*1./l.inputs, l.delta_gpu, 1);
    }

    cuda_pull_array(l.output_gpu, l.output, l.batch*l.inputs);
    l.cost[0] = sum_array(l.output, l.batch*l.inputs);
}

void backward_cost_layer_gpu(const cost_layer l, network net)
{
    axpy_ongpu(l.batch*l.inputs, l.scale, l.delta_gpu, 1, net.delta_gpu, 1);
}
#endif

