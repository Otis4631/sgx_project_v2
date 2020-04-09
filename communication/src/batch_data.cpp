#include "batch_data.hpp"

void BatchData::load_one_thread_csv(size_t start, size_t n, string path, char* out) {
    n *= img_size * sizeof(float);
    ifstream file(path, ios_base::binary);
    file.seekg(start * img_size * sizeof(float));
    file.read(out, n);
}


size_t BatchData::load_csv() {
    // batch 1024
    // 40
    size_t n = (total_img - cur_pos) > batch ? batch : (total_img - cur_pos);

    int img_per_thread = n / n_thread;
    thread t_arr[n_thread];
    for(int i = 0; i < n_thread; i++) {
        t_arr[i] = thread(boost::bind(&BatchData::load_one_thread_csv, this, cur_pos, img_per_thread, file_path, data.get() + cur_pos * img_size * sizeof(float)));
        cur_pos += img_per_thread;
    }
    int remain = n % n_thread;
    load_one_thread_csv(cur_pos, remain, file_path, data.get() + cur_pos * img_size * sizeof(float));
    
    for(int i = 0; i < n_thread; i++)
        t_arr[i].join();
}


size_t BatchData::get_next() {
    if(cur_pos > total_img) {
        LOG_NOTICE(lg) << "there are no more data, call reset() if want to read again";
        return 0;
    }
    if(type == CSV) {
        load_csv();
    }
}


DataType to_datatype(string s) {
    if(s == "csv")
        return CSV;
    if(s == "img")
        return Image;
}