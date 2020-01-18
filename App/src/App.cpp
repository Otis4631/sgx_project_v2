#include "Enclave_u.h"
#include "sgx_urts.h"
#include <stdio.h>
#include <time.h>
#include <iostream>
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
}


int initialize_enclave(void)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    
    /* Call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_create_enclave(ENCLAVE_FILENAME, 1, NULL, NULL, &EID, NULL);
    if (ret != SGX_SUCCESS) {
        printf("error when init enclave\n");
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
    encrypt_csv("data/conv_test.csv", "data/e_conv_test.csv",(unsigned char *)passwd,sizeof(passwd));
    //decrypt_csv("data/e_conv_test.csv",(unsigned char *)passwd,sizeof(passwd));
}

int main(){
    time_t t = clock();
    initialize_enclave();
    printf("initialize enclave successfully using %.3fs\n", (double)(clock() - t) / CLOCKS_PER_SEC);
   // predict("data/conv_test.cfg", "cfg/mynet.cfg", NULL);
   train("data/train.cfg", "cfg/mynet.cfg", NULL);
    destory_enclave();

}
