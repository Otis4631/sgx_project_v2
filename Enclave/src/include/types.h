#ifndef TYPES
#define TYPES
typedef unsigned long size_t;

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
         size_t batch;
         size_t inputs;
         size_t outputs;
         size_t h;
         size_t w;
         size_t ic;
         size_t out_h;
         size_t out_w;
         size_t oc;

         size_t size;
         size_t stride;
         size_t pad;

         float* input;
         float* biases;
         float* weights;
         float* output;

         int batch_normalize;
         ACTIVATION a;
} dnnl_transfer_layer_data;
#endif