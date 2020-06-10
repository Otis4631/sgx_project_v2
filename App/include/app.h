#ifndef __APP__
#define __APP__

#include <stdio.h>
#include <time.h>
#include <iostream>

#include <vector>
// #include "openssl_crypto.h"


using namespace std;


const int TRAIN = 0;
const int INFERENCE = 1;



class Classifier
{
    public:
    Classifier();

    ~Classifier();
    void init_crypto(uint8_t* n, size_t n_len, uint8_t* e, size_t e_len, uint8_t* out, uint8_t* iv, size_t iv_len);
    void start();
    uint64_t eid;
    int work_mode = 0;
    string network_path;
    string weights_path;
    string data_path;
    char* iv_b;
    char* key_b;
};


#endif