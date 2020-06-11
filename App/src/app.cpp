#include "app.h"
extern "C"
{
    #include "App_c.h"
}
#define ENCLAVE_FILENAME "enclave.signed.so"

void ocall_print_string(const char *str)
{
    printf("%s", str);
}

void ocall_print_string2hex(const char *str, int n)
{
    print_string2hex((uint8_t *)str, n);
}
extern void encrypt_csv(char* filename_plaintext, char* filename_ciphertext, unsigned char* passwd, size_t passwd_len) ;
void decrypt_csv(char* filename_ciphertext, unsigned char* passwd, size_t passwd_len, int times = 4);
extern "C"
{
    sgx_enclave_id_t EID = 0;
    extern void predict(char *datacfg, char *cfgfile, char *weightfile);
    extern void train(char *datacfg, char *cfgfile, char *weightfile);
#include "base64.h"
}

int initialize_enclave(sgx_enclave_id_t *eid)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;

    /* Call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */

    const void *enclave_ex_p[32] = {0};

    // enclave_ex_p[SGX_CREATE_ENCLAVE_EX_SWITCHLESS_BIT_IDX] = (const void*)us_config;
    ret = sgx_create_enclave("/data/lz/sgx_project_v2/build/enclave.signed.so", SGX_DEBUG_FLAG, NULL, NULL, eid, NULL);
    // ret = sgx_create_enclave_ex(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL, &EID, NULL, SGX_CREATE_ENCLAVE_EX_SWITCHLESS, enclave_ex_p);
    if (ret != SGX_SUCCESS)
    {
        print_error_message(ret);
        return -1;
    }

    return 0;
}

int destory_enclave(sgx_enclave_id_t *eid)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;

    /* Call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_destroy_enclave(*eid);
    if (ret != SGX_SUCCESS)
    {
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

// int main(int argc, char **argv)
// {
//     if (argc < 3)
//     {
//         printf("Usage ./%s [train/predict] [data cfg] [network cfg] [weights cfg(optional)] \n", argv[0]);
//         return -1;
//     }

// #ifdef OPENMP
//     printf("OPENMP!\n");
// #endif
//     time_t t = clock();
//     if (initialize_enclave() == 0)
//         printf("initialize enclave successfully using %.3fs\n", (double)(clock() - t) / CLOCKS_PER_SEC);
//     else
//     {
//         printf("Initialize enclave failed\n");
//         return -1;
//     }

//     char *weights_path = NULL;
//     if (argc > 4)
//         weights_path = argv[4];
//     if (0 == strcmp(argv[1], "train"))
//         train(argv[2], argv[3], weights_path);
//     else if (0 == strcmp(argv[1], "predict"))
//         predict(argv[2], argv[3], weights_path);
//     else
//     {
//         printf("Unknown args\n");
//     }
//     destory_enclave();
// }

int endian_swap(uint8_t *in, size_t size)
{
    for (int i = 0; i < size / 2; i++)
    {
        swap(in[i], in[size - i - 1]);
    }
}

Classifier::Classifier()
{
    time_t t = clock();
    if (initialize_enclave(&eid) == 0)
        printf("initialize enclave successfully using %.3fs\n", (double)(clock() - t) / CLOCKS_PER_SEC);
    else
    {
        printf("Initialize enclave failed\n");
        abort();
    }
};

Classifier::~Classifier()
{
    destory_enclave(&eid);
}

void Classifier::init_crypto(uint8_t *n, size_t n_len, uint8_t *e, size_t e_len, uint8_t *out, uint8_t *iv, size_t iv_len)
{
    iv_b = new char[iv_len];
    key_b = new char[n_len];
    endian_swap(n, n_len);
    endian_swap(e, e_len);
    sgx_status_t ret = init_crypto_ecall(eid, n, n_len, e, e_len, out, iv, iv_len);
    if (ret != 0)
    {
        print_error_message(ret);
        abort();
    }
}
void Classifier::start() {
    encrypt();
    EID = eid;

    const char datacfg[] = "/data/lz/sgx_project_v2/data/e_mnist.cfg";
    
    const char cfgfile[] ="/data/lz/sgx_project_v2/cfg/e_mynet.cfg";
    train((char*)datacfg, (char*)cfgfile, NULL);

}
