#ifndef TYPES
#define TYPES
#include <stdint.h>
typedef enum{
    LOGISTIC, RELU, RELIE, LINEAR, RAMP, TANH, PLSE, LEAKY, ELU, LOGGY, STAIR, HARDTAN, LHTAN
}ACTIVATION;

typedef enum{
    SSE, MASKED, L1, SMOOTH, CE
} COST_TYPE;


typedef enum {
    CONVOLUTIONAL,
    DECONVOLUTIONAL,
    CONNECTED,
    MAXPOOL,
    SOFTMAX,
    DETECTION,
    DROPOUT,
    CROP,
    ROUTE,
    COST,
    NORMALIZATION,
    AVGPOOL,
    LOCAL,
    SHORTCUT,
    ACTIVE,
    RNN,
    GRU,
    CRNN,
    BATCHNORM,
    NETWORK,
    XNOR,
    REGION,
    REORG,
    BLANK               // 表示未识别的网络层名称
} LAYER_TYPE;

typedef struct {
         int64_t batch;
         int64_t inputs;
         int64_t outputs;
         int64_t h;
         int64_t w;
         int64_t ic;
         int64_t out_h;
         int64_t out_w;
         int64_t oc;

         int64_t size;
         int64_t stride;
         int64_t pad;

         float* input;
         float* biases;
         float* weights;
         float* output;

         int batch_normalize;
         ACTIVATION a;
} dnnl_transfer_layer_data;
#endif