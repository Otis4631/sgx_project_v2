#ifndef CRYPTO_H
#define CRYPTO_H

#ifdef SGX
#include <sgx_tcrypto.h>
#include <mbusafecrt.h>

class Crypto {
    public:
        uint8_t * password;
        size_t password_len;
        size_t key_len;
        uint8_t * keys;

        int decrypt(uint8_t * ciphertext);
        int generate_keys(const uint8_t* passwd, size_t passwd_len, size_t key_len);
        int decrypt(void * ciphertext, size_t cipher_len, int bytes_block, void* plaintext); 
        int encrypt(void* plaintext, size_t plaintext_len, int bytes_block, void* ciphertext);

        Crypto(void* passwd, size_t passwd_len, size_t keys_len); 
};

#else
#include <stdio.h>
#include <memory.h>
#include <openssl/aes.h>

#endif



#endif