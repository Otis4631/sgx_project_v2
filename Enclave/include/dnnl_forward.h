#ifndef DNNL_FORWAD
#define DNNL_FORWAD
#include "dnnl.hpp"
#include "dnnl_utils.hpp"
#include "types.h"
using namespace dnnl;

void dnnl_conv_forward(dnnl_transfer_layer_data *data);
void dnnl_connected_forward(dnnl_transfer_layer_data *data);



void run_dnnl_function(std::function<void(dnnl_transfer_layer_data *)> func, dnnl_transfer_layer_data *data);

#endif