#pragma once

#include "App_c.h"

load_args data_preparation(network* net, list* data_options);


void train(char *datacfg, char *cfgfile, char *weightfile);
