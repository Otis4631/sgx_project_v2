
#ifndef E_FORWARD
#define E_FORWARD
#include "layer.h"
#include "network.h"

int e_gemm(int TA, int TB, int M, int N, int K, float ALPHA, 
        float *A, int lda, 
        float *B, int ldb,
        float BETA,
        float *C, int ldc);

int e_forward_connected_layer(layer l, network net);
int e_forward_maxpool_layer(layer l, network net);
int e_forward_convolutional_layer(layer l, network net);
int e_forward_cost_layer(const layer l, network net);
#endif