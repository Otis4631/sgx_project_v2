#ifndef E_BACKWARD
#define E_BACKWARD
#include "layer.h"
#include "network.h"

void e_backward_connected_layer(layer l, network net);
void e_backward_cost_layer(layer l, network net);
void e_backward_dropout_layer(layer l, network net);
#endif