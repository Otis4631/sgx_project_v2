
#include <enclave.h>
#include <sgx_tcrypto.h>
#include <sgx_err.hpp>
#include <memory>
#include "enclave_crypto.h"
#include "Enclave_t.h"
#include <string>

using namespace std;

Crypto *crypto;

int Crypto::encrypt(uint8_t* out, size_t* out_len, uint8_t* in, size_t in_len) {
    if(cipher_type == AES128GCM){
        sgx_aes_gcm_128bit_key_t p_key;
        sgx_aes_gcm_128bit_tag_t mac;
        memcpy(p_key, sym_key, 16);
        sgx_status_t ret = sgx_rijndael128GCM_encrypt(
            &p_key, in, in_len, out, iv, iv_len, NULL, 0, &mac);
        if(ret != SGX_SUCCESS) {
            print_error_message(ret);
            abort();
        }
    }
}

int Crypto::decrypt(uint8_t* out, size_t* out_len, uint8_t* in, size_t in_len) {
    if(cipher_type == AES128GCM){
        sgx_aes_gcm_128bit_key_t p_key;
        sgx_aes_gcm_128bit_tag_t mac;
        memcpy(p_key, sym_key, 16);
        sgx_status_t ret = sgx_rijndael128GCM_decrypt(
            &p_key, in, in_len, out, iv, iv_len, NULL, 0, &mac);
        if(ret != SGX_SUCCESS) {
            print_error_message(ret);
            abort();
        }
    }
}

int Crypto::gen_sym_key() {
    vector<uint8_t> random_bytes(32);
    gen_random_bytes(32, random_bytes);

    sgx_sha256_hash_t p_hash;
    int round = (sym_key_len + iv_len) / 32;
    int remainder = (sym_key_len + iv_len) % 32;
    int current_key_len = 0;

    uint8_t* tmp = (new uint8_t[sym_key_len + iv_len]);
    if(!tmp) {
        printf("can not allow memory!\n");
        abort();
    } 
    while(round) {
        run_sgx_function(sgx_sha256_msg, random_bytes.data(), random_bytes.size(), &p_hash);
        for(int i = 0; i < 32; i++) {
            random_bytes.push_back(p_hash[i]);
            tmp[i] = p_hash[i];
        }
        current_key_len += 32;
        round --;
    }
    if(remainder) {
        run_sgx_function(sgx_sha256_msg, random_bytes.data(), random_bytes.size(), &p_hash);
        memcpy(tmp + current_key_len, p_hash, remainder);
    }
    sym_key_encrypted_len = public_key_len;
    memcpy(sym_key, tmp, sym_key_len);
    memcpy(iv, tmp + sym_key_len, iv_len);


    sym_key_encrypted = new uint8_t[sym_key_encrypted_len];
    if(!sym_key_encrypted) {
        printf("can not allow memory!\n");
        abort();
    }

    // encrypt sym key with public key
    run_sgx_function(sgx_rsa_pub_encrypt_sha256, (void*)public_key, 
    (unsigned char*)sym_key_encrypted , (size_t*)&sym_key_encrypted_len, 
    (const unsigned char*)sym_key, (const size_t)sym_key_len);

    delete[] tmp;
    return 0;
}
Crypto::Crypto(uint8_t* _public_key, int pk_len, int _sym_key_len, int _iv_len, CIPHER type):
        public_key(_public_key), public_key_len(pk_len), sym_key_len(_sym_key_len),iv_len(_iv_len), cipher_type(type) {
    sym_key = new uint8_t[sym_key_len];
    iv = new uint8_t[iv_len];
    if(!iv || !sym_key) {
        printf("can not allow memory!\n");
        abort();
    }
    gen_sym_key();
}

Crypto::~Crypto() {
    if(iv)                  delete[] iv;
    if(sym_key)             delete[] sym_key;
    if(sym_key_encrypted)   delete[] sym_key_encrypted;
    if(public_key)          delete[] public_key;
}

void init_crypto_ecall(uint8_t* n, size_t n_len, uint8_t* e, size_t e_len, uint8_t* out, uint8_t* iv_o, size_t iv_len) {
    uint8_t *pub_key;
    int remainder = e_len % sizeof(int);
    // std::string e_s(e, e+e_len);
    // for(int i = 0; i < sizeof(int) - remainder; i++) {
    //     e_s.insert(e_s.begin(), '\0');
    // }
    // e = (uint8_t*)e_s.c_str();
    e_len += (sizeof(int) - remainder);
    sgx_status_t ret = sgx_create_rsa_pub1_key(n_len, e_len , n, e, (void**)&pub_key);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        abort();
    }
    crypto = new Crypto(pub_key, n_len, 16, 12, AES128GCM);

    if(!crypto)
        abort();
    memcpy(out, crypto->sym_key_encrypted, n_len);
    memcpy(iv_o, crypto->iv, iv_len);
    #ifdef DEBUG
        printf("Enclave sym key: ");
        print_string2hex(crypto->sym_key, crypto->sym_key_len);
    #endif   
}

void test(uint8_t* p, size_t n) {
    memcpy(p, crypto->sym_key, 16);
}
