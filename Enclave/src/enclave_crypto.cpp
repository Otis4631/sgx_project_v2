
#include <enclave.h>
#include <sgx_tcrypto.h>
#include <sgx_err.hpp>
#include <memory>
#include "enclave_crypto.h"
using namespace std;

template<typename T, typename... Ts>
unique_ptr<T> make_unique(Ts&&... params)
{
    return unique_ptr<T>(new T(forward<Ts>(params)...));
}

Crypto::Crypto(uint8_t* _public_key, int pk_len, int _sym_key_len, CIPHER type) :
            public_key(_public_key), sym_key_len(_sym_key_len), cipher_type(type) {

    public_key.reset(_public_key);
    public_key_len = pk_len;
    sym_key = make_unique<uint8_t>(sym_key_len);
    
    cipher_type = type;
    vector<uint8_t> random_bytes(32);
    gen_random_bytes(32, random_bytes);
    shared_ptr<uint8_t> sha256_res(new uint8_t[32]);
    sgx_sha256_hash_t p_hash;

    int round = sym_key_len / 32;
    int remainder = sym_key_len % 32;
    int current_key_len = 0;

    while(round) {
        run_sgx_function(sgx_sha256_msg, random_bytes.data(), random_bytes.size(), &p_hash);
        for(int i = 0; i < 32; i++) {
            random_bytes.push_back(p_hash[i]);
            sym_key.get()[i] = p_hash[i];
        }
        current_key_len += 32;
        round --;
    }
    
    if(remainder) {
        run_sgx_function(sgx_sha256_msg, random_bytes.data(), random_bytes.size(), &p_hash);
        memcpy(sym_key.get() + current_key_len, p_hash, remainder);
    }

    sym_key_encrypted_len = public_key_len;
    sym_key_encrypted = make_unique<uint8_t>(sym_key_encrypted_len);


    // encrypt sym key with public key
    run_sgx_function(sgx_rsa_pub_encrypt_sha256, (void*)public_key.get(), 
    (unsigned char*)sym_key_encrypted.get() , (size_t*)&sym_key_encrypted_len, 
    (const unsigned char*)sym_key.get(), (const size_t)sym_key_len);
}
