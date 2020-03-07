#include "Enclave_u.h"
#include "sgx_urts.h"
#include <stdio.h>
#include <time.h>
#include <iostream>
#include <sgx_uswitchless.h>
#include "sgx_err.h"
#include "classifier.h"
#include "encrypt_file.h"
#include "openssl_crypto.h"
extern "C" {
    #include "utils.h"
}

using namespace std;

#define ENCLAVE_FILENAME "enclave.signed.so"
void ocall_print_string(const char *str)
{
    printf("%s", str);
}

void ocall_print_string2hex(const char *str, int n)
{
    print_string2hex((uint8_t*)str, n);
}


extern "C"{
    sgx_enclave_id_t EID = 0;
    extern void predict(char *datacfg, char *cfgfile, char *weightfile);
    extern void train(char *datacfg, char *cfgfile, char *weightfile);
    #include "base64.h"
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

// int main(int argc, char ** argv){
//     if(argc < 3){
//         printf("Usage ./%s [train/predict] [data cfg] [network cfg] [weights cfg(optional)] ", argv[0]);
//         return -1;
//     }
//     /* Configuration for Switchless SGX */
//     #ifdef OPENMP
//         printf("OPENMP!\n");
//     #endif
//     time_t t = clock();
//     if(initialize_enclave(NULL) == 0)
//         printf("initialize enclave successfully using %.3fs\n", (double)(clock() - t) / CLOCKS_PER_SEC);
//     else
//         return -1;
//     char * weights_path = NULL;
//     if(argc > 4)
//         weights_path = argv[4];
//     if(0 == strcmp(argv[1], "train"))
//         train(argv[2], argv[3], weights_path);
//     else if(0 == strcmp(argv[1], "predict"))
//         predict(argv[2], argv[3], weights_path);
//     else
//     {
//         printf("Unknown args\n");
//     }
//     destory_enclave();
// }

int endian_swap(uint8_t* in, size_t size){
    for(int i = 0; i < size / 2; i++) {
        swap(in[i], in[size - i  - 1]);
    }
}


int main()
{
    initialize_enclave(NULL);
    OpenSSLCrypto enc(RSA_OAEP);
    ("%d\n", enc.open_public_key("/data/lz/sgx_project_v2/Enclave/public.pem"));
    size_t n_len = 0;
    size_t e_len = 0;
    enc.get_raw_public_key(NULL, &n_len, NULL, &e_len);
    uint8_t* n = new uint8_t[n_len];
    uint8_t* e = new uint8_t[e_len];
    uint8_t* ciphertxt = new uint8_t[n_len];
    enc.get_raw_public_key(n, &n_len, e, &e_len);
    endian_swap(n, n_len);
    endian_swap(e, e_len);
    test(EID, n, n_len, e, e_len, ciphertxt);
    OpenSSLCrypto d(RSA_OAEP | MODE_DECRYPT);
    d.open_private_key("/data/lz/sgx_project_v2/Enclave/private.pem", NULL);

    uint8_t* decryptxt = new uint8_t[384];
    size_t out_len = 384;

    d.crypt(decryptxt, &out_len, ciphertxt, n_len);

    printf("Received sym key: ");
    print_string2hex(decryptxt, out_len);

    OpenSSLCrypto aes_gcm(AES_128_GCM | MODE_DECRYPT);
    aes_gcm.AES_init(,);

}