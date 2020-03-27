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


void validate_classifier_crop(char *datacfg, char *filename, char *weightfile)
{
    int i = 0;
    network net = parse_network_cfg(filename);
    if(weightfile){
        load_weights(&net, weightfile);
    }
    srand(time(0));

    list *options = read_data_cfg(datacfg);

    char *label_list = option_find_str(options, "labels", "data/labels.list");
    char *valid_list = option_find_str(options, "valid", "data/train.list");
    int classes = option_find_int(options, "classes", 2);
    int topk = option_find_int(options, "top", 1);

    char **labels = get_labels(label_list);
    list *plist = get_paths(valid_list);

    char **paths = (char **)list_to_array(plist);
    int m = plist->size;
    free_list(plist);

    clock_t time;
    float avg_acc = 0;
    float avg_topk = 0;
    int splits = m/1000;
    int num = (i+1)*m/splits - i*m/splits;

    data val, buffer;

    load_args args = {0};
    args.w = net.w;
    args.h = net.h;

    args.paths = paths;
    args.classes = classes;
    args.n = num;
    args.m = 0;
    args.labels = labels;
    args.d = &buffer;
    args.type = OLD_CLASSIFICATION_DATA;

    pthread_t load_thread = load_data_in_thread(args);
    for(i = 1; i <= splits; ++i){
        time=clock();

        pthread_join(load_thread, 0);
        val = buffer;

        num = (i+1)*m/splits - i*m/splits;
        char **part = paths+(i*m/splits);
        if(i != splits){
            args.paths = part;
            load_thread = load_data_in_thread(args);
        }
        printf("Loaded: %d images in %lf seconds\n", val.X.rows, sec(clock()-time));

        time=clock();
        float *acc = network_accuracies(net, val, topk);
        avg_acc += acc[0];
        avg_topk += acc[1];
        printf("%d: top 1: %f, top %d: %f, %lf seconds, %d images\n", i, avg_acc/i, topk, avg_topk/i, sec(clock()-time), val.X.rows);
        free_data(val);
    }
}

void validate_classifier_10(char *datacfg, char *filename, char *weightfile)
{
    int i, j;
    network net = parse_network_cfg(filename);
    set_batch_network(&net, 1);
    if(weightfile){
        load_weights(&net, weightfile);
    }
    srand(time(0));

    list *options = read_data_cfg(datacfg);

    char *label_list = option_find_str(options, "labels", "data/labels.list");
    char *valid_list = option_find_str(options, "valid", "data/train.list");
    int classes = option_find_int(options, "classes", 2);
    int topk = option_find_int(options, "top", 1);

    char **labels = get_labels(label_list);
    list *plist = get_paths(valid_list);

    char **paths = (char **)list_to_array(plist);
    int m = plist->size;
    free_list(plist);

    float avg_acc = 0;
    float avg_topk = 0;
    int *indexes = calloc(topk, sizeof(int));

    for(i = 0; i < m; ++i){
        int class = -1;
        char *path = paths[i];
        for(j = 0; j < classes; ++j){
            if(strstr(path, labels[j])){
                class = j;
                break;
            }
        }
        int w = net.w;
        int h = net.h;
        int shift = 32;
        image im = load_image_color(paths[i], w+shift, h+shift);
        image images[10];
        images[0] = crop_image(im, -shift, -shift, w, h);
        images[1] = crop_image(im, shift, -shift, w, h);
        images[2] = crop_image(im, 0, 0, w, h);
        images[3] = crop_image(im, -shift, shift, w, h);
        images[4] = crop_image(im, shift, shift, w, h);
        flip_image(im);
        images[5] = crop_image(im, -shift, -shift, w, h);
        images[6] = crop_image(im, shift, -shift, w, h);
        images[7] = crop_image(im, 0, 0, w, h);
        images[8] = crop_image(im, -shift, shift, w, h);
        images[9] = crop_image(im, shift, shift, w, h);
        float *pred = calloc(classes, sizeof(float));
        for(j = 0; j < 10; ++j){
            float *p = network_predict(net, images[j].data);
            if(net.hierarchy) hierarchy_predictions(p, net.outputs, net.hierarchy, 1, 1);
            axpy_cpu(classes, 1, p, 1, pred, 1);
            free_image(images[j]);
        }
        free_image(im);
        top_k(pred, classes, topk, indexes);
        free(pred);
        if(indexes[0] == class) avg_acc += 1;
        for(j = 0; j < topk; ++j){
            if(indexes[j] == class) avg_topk += 1;
        }

        printf("%d: top 1: %f, top %d: %f\n", i, avg_acc/(i+1), topk, avg_topk/(i+1));
    }
}




void validate_classifier_single(char *datacfg, char *filename, char *weightfile)
{
    int i, j;
    network net = parse_network_cfg(filename);
    if(weightfile){
        load_weights(&net, weightfile);
    }
    set_batch_network(&net, 1);
    srand(time(0));

    list *options = read_data_cfg(datacfg);

    char *label_list = option_find_str(options, "labels", "data/labels.list");
    char *leaf_list = option_find_str(options, "leaves", 0);
    if(leaf_list) change_leaves(net.hierarchy, leaf_list);
    char *valid_list = option_find_str(options, "valid", "data/train.list");
    int classes = option_find_int(options, "classes", 2);
    int topk = option_find_int(options, "top", 1);

    char **labels = get_labels(label_list);
    list *plist = get_paths(valid_list);

    char **paths = (char **)list_to_array(plist);
    int m = plist->size;
    free_list(plist);

    float avg_acc = 0;
    float avg_topk = 0;
    int *indexes = calloc(topk, sizeof(int));

    for(i = 0; i < m; ++i){
        int class = -1;
        char *path = paths[i];
        for(j = 0; j < classes; ++j){
            if(strstr(path, labels[j])){
                class = j;
                break;
            }
        }
        image im = load_image_color(paths[i], 0, 0);
        image resized = resize_min(im, net.w);
        image crop = crop_image(resized, (resized.w - net.w)/2, (resized.h - net.h)/2, net.w, net.h);
        //show_image(im, "orig");
        //show_image(crop, "cropped");
        //cvWaitKey(0);
        float *pred = network_predict(net, crop.data);
        if(net.hierarchy) hierarchy_predictions(pred, net.outputs, net.hierarchy, 1, 1);

        if(resized.data != im.data) free_image(resized);
        free_image(im);
        free_image(crop);
        top_k(pred, classes, topk, indexes);

        if(indexes[0] == class) avg_acc += 1;
        for(j = 0; j < topk; ++j){
            if(indexes[j] == class) avg_topk += 1;
        }

        printf("%d: top 1: %f, top %d: %f\n", i, avg_acc/(i+1), topk, avg_topk/(i+1));
    }
}

void try_classifier(char *datacfg, char *cfgfile, char *weightfile, char *filename, int layer_num)
{
    network net = parse_network_cfg(cfgfile);
    if(weightfile){
        load_weights(&net, weightfile);
    }
    set_batch_network(&net, 1);
    srand(2222222);

    list *options = read_data_cfg(datacfg);

    char *name_list = option_find_str(options, "names", 0);
    if(!name_list) name_list = option_find_str(options, "labels", "data/labels.list");
    int top = option_find_int(options, "top", 1);

    int i = 0;
    char **names = get_labels(name_list);
    clock_t time;
    int *indexes = calloc(top, sizeof(int));
    char buff[256];
    char *input = buff;
    while(1){
        if(filename){
            strncpy(input, filename, 256);
        }else{
            printf("Enter Image Path: ");
            fflush(stdout);
            input = fgets(input, 256, stdin);
            if(!input) return;
            strtok(input, "\n");
        }
        image orig = load_image_color(input, 0, 0);
        image r = resize_min(orig, 256);
        image im = crop_image(r, (r.w - 224 - 1)/2 + 1, (r.h - 224 - 1)/2 + 1, 224, 224);
        float mean[] = {0.48263312050943, 0.45230225481413, 0.40099074308742};
        float std[] = {0.22590347483426, 0.22120921437787, 0.22103996251583};
        float var[3];
        var[0] = std[0]*std[0];
        var[1] = std[1]*std[1];
        var[2] = std[2]*std[2];

        normalize_cpu(im.data, mean, var, 1, 3, im.w*im.h);

        float *X = im.data;
        time=clock();
        float *predictions = network_predict(net, X);

        layer l = net.layers[layer_num];
        for(i = 0; i < l.c; ++i){
            if(l.rolling_mean) printf("%f %f %f\n", l.rolling_mean[i], l.rolling_variance[i], l.scales[i]);
        }
#ifdef GPU
        cuda_pull_array(l.output_gpu, l.output, l.outputs);
#endif
        for(i = 0; i < l.outputs; ++i){
            printf("%f\n", l.output[i]);
        }
        /*

           printf("\n\nWeights\n");
           for(i = 0; i < l.n*l.size*l.size*l.c; ++i){
           printf("%f\n", l.filters[i]);
           }

           printf("\n\nBiases\n");
           for(i = 0; i < l.n; ++i){
           printf("%f\n", l.biases[i]);
           }
         */

        top_predictions(net, top, indexes);
        printf("%s: Predicted in %f seconds.\n", input, sec(clock()-time));
        for(i = 0; i < top; ++i){
            int index = indexes[i];
            printf("%s: %f\n", names[index], predictions[index]);
        }
        free_image(im);
        if (filename) break;
    }
}






void predict_classifier(char *datacfg, char *cfgfile, char *weightfile, char *filename, int top)
{
    network net = parse_network_cfg(cfgfile);
    if(weightfile){
        load_weights(&net, weightfile);
    }
    set_batch_network(&net, 1);
    srand(2222222);

    list *options = read_data_cfg(datacfg);

    char *name_list = option_find_str(options, "names", 0);
    if(!name_list) name_list = option_find_str(options, "labels", "data/labels.list");
    if(top == 0) top = option_find_int(options, "top", 1);

    int i = 0;
    char **names = get_labels(name_list);
    clock_t time;
    int *indexes = calloc(top, sizeof(int));
    char buff[256];
    char *input = buff;
    int size = net.w;
    while(1){
        if(filename){
            strncpy(input, filename, 256);
        }else{
            printf("Enter Image Path: ");
            fflush(stdout);
            input = fgets(input, 256, stdin);
            if(!input) return;
            strtok(input, "\n");
        }
        image im = load_image_color(input, 0, 0);
        image r = resize_min(im, size);
        resize_network(&net, r.w, r.h);
        //printf("%d %d\n", r.w, r.h);

        float *X = r.data;
        time=clock();
        float *predictions = network_predict(net, X);
        if(net.hierarchy) hierarchy_predictions(predictions, net.outputs, net.hierarchy, 1, 1);
        top_k(predictions, net.outputs, top, indexes);
        printf("%s: Predicted in %f seconds.\n", input, sec(clock()-time));
        for(i = 0; i < top; ++i){
            int index = indexes[i];
            //if(net.hierarchy) printf("%d, %s: %f, parent: %s \n",index, names[index], predictions[index], (net.hierarchy->parent[index] >= 0) ? names[net.hierarchy->parent[index]] : "Root");
            //else printf("%s: %f\n",names[index], predictions[index]);
            printf("%5.2f%%: %s\n", predictions[index]*100, names[index]);
        }
        if(r.data != im.data) free_image(r);
        free_image(im);
        if (filename) break;
    }
}


void label_classifier(char *datacfg, char *filename, char *weightfile)
{
    int i;
    network net = parse_network_cfg(filename);
    set_batch_network(&net, 1);
    if(weightfile){
        load_weights(&net, weightfile);
    }
    srand(time(0));

    list *options = read_data_cfg(datacfg);

    char *label_list = option_find_str(options, "names", "data/labels.list");
    char *test_list = option_find_str(options, "test", "data/train.list");
    int classes = option_find_int(options, "classes", 2);

    char **labels = get_labels(label_list);
    list *plist = get_paths(test_list);

    char **paths = (char **)list_to_array(plist);
    int m = plist->size;
    free_list(plist);

    for(i = 0; i < m; ++i){
        image im = load_image_color(paths[i], 0, 0);
        image resized = resize_min(im, net.w);
        image crop = crop_image(resized, (resized.w - net.w)/2, (resized.h - net.h)/2, net.w, net.h);
        float *pred = network_predict(net, crop.data);

        if(resized.data != im.data) free_image(resized);
        free_image(im);
        free_image(crop);
        int ind = max_index(pred, classes);

        printf("%s\n", labels[ind]);
    }
}


void test_classifier(char *datacfg, char *cfgfile, char *weightfile, int target_layer)
{
    int curr = 0;
    network net = parse_network_cfg(cfgfile);
    if(weightfile){
        load_weights(&net, weightfile);
    }
    srand(time(0));

    list *options = read_data_cfg(datacfg);

    char *test_list = option_find_str(options, "test", "data/test.list");
    int classes = option_find_int(options, "classes", 2);

    list *plist = get_paths(test_list);

    char **paths = (char **)list_to_array(plist);
    int m = plist->size;
    free_list(plist);

    clock_t time;

    data val, buffer;

    load_args args = {0};
    args.w = net.w;
    args.h = net.h;
    args.paths = paths;
    args.classes = classes;
    args.n = net.batch;
    args.m = 0;
    args.labels = 0;
    args.d = &buffer;
    args.type = OLD_CLASSIFICATION_DATA;

    pthread_t load_thread = load_data_in_thread(args);
    for(curr = net.batch; curr < m; curr += net.batch){
        time=clock();

        pthread_join(load_thread, 0);
        val = buffer;

        if(curr < m){
            args.paths = paths + curr;
            if (curr + net.batch > m) args.n = m - curr;
            load_thread = load_data_in_thread(args);
        }
        fprintf(stderr, "Loaded: %d images in %lf seconds\n", val.X.rows, sec(clock()-time));

        time=clock();
        matrix pred = network_predict_data(net, val);

        int i, j;
        if (target_layer >= 0){
            //layer l = net.layers[target_layer];
        }

        for(i = 0; i < pred.rows; ++i){
            printf("%s", paths[curr-net.batch+i]);
            for(j = 0; j < pred.cols; ++j){
                printf("\t%g", pred.vals[i][j]);
            }
            printf("\n");
        }

        free_matrix(pred);

        fprintf(stderr, "%lf seconds, %d images, %d total\n", sec(clock()-time), val.X.rows, curr);
        free_data(val);
    }
}




void run_classifier(int argc, char **argv)
{
    if(argc < 4){
        fprintf(stderr, "usage: %s %s [train/test/valid] [cfg] [weights (optional)]\n", argv[0], argv[1]);
        return;
    }

    int cam_index = find_int_arg(argc, argv, "-c", 0);
    int top = find_int_arg(argc, argv, "-t", 0);
    int clear = find_arg(argc, argv, "-clear");
    char *data = argv[3];
    char *cfg = argv[4];
    char *weights = (argc > 5) ? argv[5] : 0;
    char *filename = (argc > 6) ? argv[6]: 0;
    char *layer_s = (argc > 7) ? argv[7]: 0;
    int layer = layer_s ? atoi(layer_s) : -1;
    if(0==strcmp(argv[2], "predict")) predict_classifier(data, cfg, weights, filename, top);
    //else if(0==strcmp(argv[2], "train")) train_classifier(data, cfg, weights, NULL, 0, clear);
}


