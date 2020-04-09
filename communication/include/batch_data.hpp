#pragma once

#include <stddef.h>
#include "log.hpp"

#include <thread>
#include <fstream>
#include <boost/bind.hpp>

using namespace std;
enum DataType
{
    // describe data type, csv or image
    CSV,
    Image
};

class BatchData
{
    // expect binary file
    // read one batch, encrypt and send
public:
    BatchData(string _file_path, DataType _d, size_t _b, size_t _img_size, size_t _total_img, int _n_thread = thread::hardware_concurrency())
        : type(_d), batch(_b), img_size(_img_size), readed(0), total_img(_total_img), n_thread(_n_thread), file_path(_file_path)
    {
        // ifstream in_file();
        ifstream file(file_path, ios_base::binary);
        if (!file.is_open())
        {
            LOG_NAME("BatchData");
            LOG_ERROR(lg) << "failed to open file " << file_path;
            abort();
        }
        file.close();
        bin_size = batch * img_size * sizeof(float);
        data = make_shared<char>(bin_size);
    }

    void reset()
    {
        cur_pos = 0;
    }
    bool is_end()
    {
        return cur_pos >= total_img;
    }
    size_t get_next();
    size_t load_csv();

    void load_one_thread_csv(size_t start, size_t n, string path, char *out);

private:
    DataType type;
    size_t bin_size;
    shared_ptr<char> data;
    string file_path;
    size_t batch;
    size_t img_size;
    size_t total_img;
    size_t readed;
    int n_thread;
    int cur_pos = 0;
    log_t lg;
};

DataType to_datatype(string s);