#pragma once
#include <sgx_error.h>
#include "sgx_err.h"
typedef struct _sgx_errlist_t {
    sgx_status_t err;
    const char * msg;
} sgx_errlist_t;


static sgx_errlist_t sgx_errlist[] = {
    {SGX_ERROR_UNEXPECTED,               "Unexpected error occurred."},
    {SGX_ERROR_INVALID_PARAMETER,        "Invalid parameter."},
    {SGX_ERROR_OUT_OF_MEMORY,            "Out of memory."},
    {SGX_ERROR_ENCLAVE_LOST,             "Power transition occurred."},
    {SGX_ERROR_INVALID_ENCLAVE,          "Invalid enclave image."},
    {SGX_ERROR_INVALID_ENCLAVE_ID,       "Invalid enclave identification."},
    {SGX_ERROR_INVALID_SIGNATURE,        "Invalid enclave signature."},
    {SGX_ERROR_OUT_OF_EPC,               "Out of EPC memory."},
    {SGX_ERROR_NO_DEVICE,                "Invalid SGX device."},
    {SGX_ERROR_MEMORY_MAP_CONFLICT,      "Memory map conflicted."},
    {SGX_ERROR_INVALID_METADATA,         "Invalid encalve metadata."},
    {SGX_ERROR_DEVICE_BUSY,              "SGX device is busy."},
    {SGX_ERROR_INVALID_VERSION,          "Enclave metadata version is invalid."},
    {SGX_ERROR_ENCLAVE_FILE_ACCESS,      "Can't open enclave file."},

    {SGX_ERROR_INVALID_FUNCTION,         "Invalid function name."},
    {SGX_ERROR_OUT_OF_TCS,               "Out of TCS."},
    {SGX_ERROR_ENCLAVE_CRASHED,          "The enclave is crashed."},

    {SGX_ERROR_MAC_MISMATCH,             "Report varification error occurred."},
    {SGX_ERROR_INVALID_ATTRIBUTE,        "The enclave is not authorized."},
    {SGX_ERROR_INVALID_CPUSVN,           "Invalid CPUSVN."},
    {SGX_ERROR_INVALID_ISVSVN,           "Invalid ISVSVN."},
    {SGX_ERROR_INVALID_KEYNAME,          "The requested key name is invalid."},

    {SGX_ERROR_SERVICE_UNAVAILABLE,          "AESM service is not responsive."},
    {SGX_ERROR_SERVICE_TIMEOUT,              "Request to AESM is time out."},
    {SGX_ERROR_SERVICE_INVALID_PRIVILEGE,    "Error occurred while getting launch token."},
};


void print_error_message(sgx_status_t ret)
{
    size_t idx = 0;
    size_t ttl = sizeof sgx_errlist/sizeof sgx_errlist[0];

    for (idx = 0; idx < ttl; idx++) {
        if(ret == sgx_errlist[idx].err) {
            printf("Error: %s\n", sgx_errlist[idx].msg);
            break;
        }
    }    
    if (idx == ttl)
        printf("Error: Unexpected error occurred.\n");
    return;
}
 
#ifdef __cplusplus
#include <functional>
#include <sgx_tcrypto.h>

inline void run_sgx_function(std::function<sgx_status_t()> func) {
    sgx_status_t ret = func();
    if(ret != SGX_SUCCESS) {
        print_error_message(ret);
        abort();
    }
}

// rsa encrypt
inline void run_sgx_function(std::function<sgx_status_t(void* , unsigned char* , size_t* , const unsigned char* , const size_t)> func,
    void* rsa_key, unsigned char* pout_data, size_t* pout_len, const unsigned char* pin_data, const size_t pin_len) {
        run_sgx_function([&]() -> sgx_status_t {return func(rsa_key, pout_data, pout_len, pin_data, pin_len); });
    }

// sha256
inline void run_sgx_function(std::function<sgx_status_t(const uint8_t *p_src, uint32_t src_len, sgx_sha256_hash_t *p_hash)> func,
    const uint8_t *p_src, uint32_t src_len, sgx_sha256_hash_t *p_hash) {
    run_sgx_function([&]() {return func(p_src, src_len, p_hash);});
}

#endif
