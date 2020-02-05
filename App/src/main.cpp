#include "Enclave_u.h"
#include "sgx_urts.h"
#include <stdio.h>
#include <time.h>
#include <iostream>
#include <sgx_uswitchless.h>

#include "classifier.h"
#include "encrypt_file.h"


using namespace std;

#define ENCLAVE_FILENAME "enclave.signed.so"
void ocall_print_string(const char *str)
{
    /* Proxy/Bridge will check the length and null-terminate 
     * the input string to prevent buffer overflow. 
     */
    printf("%s", str);
}
extern "C"{
    sgx_enclave_id_t EID = 0;
    extern void predict(char *datacfg, char *cfgfile, char *weightfile);
    extern void train(char *datacfg, char *cfgfile, char *weightfile);
    #include "base64.h"
    #include "sgx_err.h"
}


int initialize_enclave(const sgx_uswitchless_config_t* us_config)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;

    /* Call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */

    const void* enclave_ex_p[32] = { 0 };

    // enclave_ex_p[SGX_CREATE_ENCLAVE_EX_SWITCHLESS_BIT_IDX] = (const void*)us_config;
    ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL, &EID, NULL);
   // ret = sgx_create_enclave_ex(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL, &EID, NULL, SGX_CREATE_ENCLAVE_EX_SWITCHLESS, enclave_ex_p);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }

    return 0;
}

int destory_enclave(void)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    
    /* Call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_destroy_enclave(EID);
    if (ret != SGX_SUCCESS) {
        printf("error when destory enclave\n");
        return -1;
    }

    return 0;
}

void encrypt() {
    unsigned char passwd[] = "lizheng";
    encrypt_csv("data/train.csv", "data/e_train.csv",(unsigned char *)passwd, sizeof(passwd));
    decrypt_csv("data/e_train.csv",(unsigned char *)passwd,sizeof(passwd));
}

int main(int argc, char ** argv){
    if(argc < 2){
        printf("No args\n");
        return -1;
    }
    /* Configuration for Switchless SGX */
    sgx_uswitchless_config_t us_config = SGX_USWITCHLESS_CONFIG_INITIALIZER;
    us_config.num_uworkers = 4;
    us_config.num_tworkers = 4;
    time_t t = clock();
    if(initialize_enclave(&us_config) == 0)
        printf("initialize enclave successfully using %.3fs\n", (double)(clock() - t) / CLOCKS_PER_SEC);
    else
        return -1;
   // predict("data/conv_test.cfg", "cfg/mynet.cfg", NULL);
    if(0 == strcmp(argv[1], "t"))
        train("data/train.cfg", "cfg/mynet.cfg", NULL);
    else if(0 == strcmp(argv[1], "e"))
        encrypt();
    else
    {
        printf("Unknown args\n");
    }
    destory_enclave();
}