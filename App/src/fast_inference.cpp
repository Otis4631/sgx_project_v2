#include <sys/time.h>
extern "C" {
#include "App_c.h"
    
}


void fast_inference(network* net, list* data_options) {
    char *result_path = option_find_str(data_options, "result_path", "./res");
    FILE* res_fp = fopen(result_path, "w");
    pthread_t load_thread;
    load_args args = data_preparation(net, data_options);
    long N = args.m;
    data train;
    struct timeval t0, t1, t2;
    double timeuse, load_data_time;
    time_t tt;
    double ttused;
    double time_sum = 0;
    while(*net->seen < N) {
        load_thread = load_data(args);
        gettimeofday(&t0, NULL);
        pthread_join(load_thread, 0);
        train = *args.d;
        gettimeofday(&t1, NULL);
        load_data_time = t1.tv_sec - t0.tv_sec + (t1.tv_usec - t0.tv_usec)/1000000.0;
        tt = clock();
        // ecall_predict_network();
    }    
}