#include "network.h"
#include "utils.h"
#include "parser.h"
#include "option_list.h"
#include "blas.h"
#include "assert.h"
#include "classifier.h"
#include "cuda.h"
#include <sys/time.h>
#include "rc4.h"

#define US 1000000 

float *get_regression_values(char **labels, int n)
{
    float *v = calloc(n, sizeof(float));
    int i;
    for(i = 0; i < n; ++i){
        char *p = strchr(labels[i], ' ');
        *p = 0;
        v[i] = atof(p+1);
    }
    return v;
}

char pass[] = "lizheng";
int pass_len = sizeof(pass);
void predict_csv(network* net, list* data_options) {
    /**
     * 推理以csv格式存放的密文图片 
     * @param:
     * net:             由cfg文件解析完成的network
     * data_options     由cfg文件解析完成的data option 
    */
    int x_total = option_find_int_quiet(data_options, "size", 1);
    char *filename = option_find_str(data_options, "predict", "data/predict.csv");
    int w = option_find_int(data_options, "width", 0);
    int h = option_find_int(data_options, "height", 0);
    int c = option_find_int(data_options, "channels", 0);
    int encrypt = option_find_int_quiet(data_options, "encrypt", 1);
    //int batch = option_find_int_quiet(data_options, "batch", )
    int N = x_total;
    char* line;
    int i = 0;
    FILE *fp = fopen(filename, "r");
    if(!fp) {
        file_error(filename);
        printf("error fp\n");
    }
    int fields, cur_batch = 0;
    int batch_size = net->batch;
    float* input = NULL;
    float* p;
    int input_offset = 0;
    int cnt = 0;
    while((line = fgetl(fp))) {
        cnt ++;
        if(!encrypt) {
            fields = count_fields(line);
            if(input == NULL)
                input = (float*)calloc(fields * net->batch, sizeof(float));
            parse_fields(line, fields, input + input_offset);
        }
        else {
            fields = count_from_base64(line) / 4;
            if(input == NULL)
                input = (float*)calloc(fields * net->batch, sizeof(float));
            read_from_base64(line, input + input_offset);
        }
        input_offset += fields;
        cur_batch ++;
        free(line);
        if(cur_batch < batch_size && cnt < N) {
            continue;
        }
       else { // predict batch
            cur_batch = 0;
            input_offset = 0;
            time_t t = clock();
            float* out = network_predict(*net, input);
            free(input);
            input = NULL;
            printf("batch predict: using time: %.4fs\n", (double)(clock() - t ) / CLOCKS_PER_SEC);
            char pass[] = "lizheng";
            if(encrypt)
                crypt_aux(pass, 8, (unsigned char*)out, 4 * net->outputs, net->batch);
            for(int i=0; i < net->outputs * net->batch; i ++)
                printf("%f ",out[i]);
            printf("\n");
       }
       if(cnt > N) break;
    }
    fclose(fp);
}


load_args data_preparation(network* net, list* data_options) {
    load_args arg = {0};
    long N = option_find_long(data_options, "size", 0);

    char *label_filename = option_find_str(data_options, "labels", "data/labels.list");
    char *train_filename = option_find_str(data_options, "data_path", "data/train.list");
    int normalize = option_find_int_quiet(data_options, "normalize", 1);
    int classes = option_find_int(data_options, "classes", 2);
    int encrypt = option_find_int(data_options, "encrypt", 0);
    int random = option_find_int_quiet(data_options, "random", 0);
    load_args args = {0};
    args.fp = fopen(train_filename, "r");
    args.csv_path = train_filename;
    args.w = net->w;
    args.h = net->h;
    args.n = net->batch;
    args.m = N;
    args.normalize = normalize;
    args.encrypt = encrypt;
    args.threads = 1;
    args.type = CSV_DATA;
    args.c = net->c;
    args.random = random;
    data *buffer = calloc(1, sizeof(data));
    args.d = buffer;
    return args;
}

void train_csv(network* net, list* data_options) {
    char *base = "mynet";
    char *backup_directory = option_find_str(data_options, "backup", "backup");
    pthread_t load_thread;
    load_args args = data_preparation(net, data_options);
    long N = args.m;
    data train;
    struct timeval t0, t1, t2;
    double timeuse, load_data_time;
    time_t tt;
    double ttused;
    double time_sum = 0;
    int epoch = (*net->seen) / N; // 当前处理的图片数 除以 总图片数
    while(get_current_batch(net) < net->max_batches){
        load_thread = load_data(args);
        gettimeofday(&t0, NULL);
        pthread_join(load_thread, 0);
        train = *args.d;
        gettimeofday(&t1, NULL);
        load_data_time = t1.tv_sec - t0.tv_sec + (t1.tv_usec - t0.tv_usec)/1000000.0;
        tt = clock();
        float loss = 0;
        loss = train_network(net, train);
        gettimeofday(&t2, NULL);
        timeuse = t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec)/1000000.0;
        ttused = sec(clock() - tt);
        time_sum += timeuse;
        printf("Epoch: %d, Batch: %d, Loss: %g, Learning Rate: %g, Time used of Data Loading %.3fs, Time used of Training %.3fs/%.3fs, Image Processed: %d\n",
        epoch, get_current_batch(net), loss, get_current_rate(net), load_data_time, timeuse, ttused,  *net->seen);
        
        free_data(train);
        
        if(*net->seen / N > epoch){
            epoch = *net->seen / N;
            char buff[256];
            sprintf(buff, "%s/%s_%d.weights",backup_directory, base, epoch);
            save_weights(*net, buff);
        }
    }

    char buff[256];
    sprintf(buff, "%s/%s.weights", backup_directory, base);
    printf("Average time used: %.3fs\n", time_sum / get_current_batch(net));
    save_weights(*net, buff);
    free_network(*net);
    free_list(data_options);
    fclose(args.fp);
    
    //free(base);

}


void train(char *datacfg, char *cfgfile, char *weightfile) {
    network net = load_network(cfgfile, weightfile, 1);
    printf("Learning Rate: %g, Momentum: %g, Decay: %g\n", net.learning_rate, net.momentum, net.batch, net.decay);

    list *data_options = read_data_cfg(datacfg);
    int is_csv_format = option_find_int(data_options, "csv", 0);

    if(is_csv_format) {
        train_csv(&net, data_options);
    }

}


void predict(char *datacfg, char *cfgfile, char *weightfile) {
    network net = load_network(cfgfile, weightfile, 1);
    list *data_options = read_data_cfg(datacfg);
    int is_csv_format = option_find_int(data_options, "csv", 0);
    
    if(is_csv_format) {
        predict_csv(&net, data_options);
     }
    else {
        // TODO 图片格式的推理

    }

}







