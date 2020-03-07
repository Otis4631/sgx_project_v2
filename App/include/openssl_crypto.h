#pragma once
#include <string>

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/md5.h>
#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/err.h>

using namespace std;

class OpenSSLCrypto
{
public:
    OpenSSLCrypto();
    OpenSSLCrypto(int _mode);

    void err_handle();

    int init_status();
    int AES_init(uint8_t *key, uint8_t *iv);

    int set_RSA_padding();

    int open_public_key(const char *pub_key_file);
    int open_private_key(const char *priv_key_file, const char *passwd);

    int AES_update(uint8_t *_out, int *_out_len, uint8_t *_in, int _in_len);
    int AES_final(uint8_t *_out, int *_out_len);

    size_t get_key_length();
    size_t get_iv_length();

    int crypt(uint8_t *_out, size_t *_out_len, uint8_t *_in, size_t _in_len);
    int file_crypt(FILE *in, FILE *out);

    int get_raw_private_key(uint8_t *priv, size_t *len);
    int get_raw_public_key(uint8_t *n_c, size_t *n_len, uint8_t *e_c, size_t *e_len);

    int set_mode(int _mode);
    void clean_up();

    ~OpenSSLCrypto();

private:
    int RSA_decrypt();
    int RSA_encrypt();
    int AES_crypt();

    int mode;
    EVP_PKEY *pkey = NULL;
    EVP_PKEY_CTX *pk_ctx = NULL;
    EVP_CIPHER_CTX *sym_ctx = NULL;

    RSA *rsa = NULL;
    ENGINE *eng = NULL;
    FILE *fp = NULL;
    BIO *bio = NULL;

    uint8_t *key = NULL;
    uint8_t *iv = NULL;

    uint8_t *in = NULL;
    uint8_t *out = NULL;

    size_t in_len;
    size_t *out_len = NULL;

    unsigned long e;
    int enc = -1;

    const string pkcs1_header = "-----BEGIN RSA P";
    const string pkcs8_header = "-----BEGIN P";
};


// CIPHER   PARAM       MODE
// 0000     00000000    0000
#define CIPHER                 0xF000   // 1111 0000 0000 0000
#define CIPHER_RSA             0x8000   // 1000 0000 0000 0000
#define CIPHER_AES             0x4000   // 0100 0000 0000 0000

#define PARAM                  0x0FF0   // 0000 1111 1111 0000
#define PARAM_GCM_128          0x0010   // 0000 0000 0001 0000
#define PARAM_OAEP             0x0020   // 0000 0000 0010 0000 
#define PARAM_COMMON           0x0000   // 0000 0000 0000 0000


#define MODE                   0x000F   // 0000 0000 0000 1111
#define MODE_ENCRYPT           0x0001   // 0000 0000 0000 0001 
#define MODE_DECRYPT           0x0002   // 0000 0000 0000 0010





#define AES_128_GCM    (CIPHER_AES | PARAM_GCM_128)
#define RSA_OAEP       (CIPHER_RSA | PARAM_OAEP)

// Error
#define ERR_OPEN_FAILED     -1
#define ERR_BAD_MODE        -2
#define ERR_NEW_FAILED      -3
#define ERR_RUN_FAILED      -4
#define ERR_READ_FAILED     -5
#define ERR_STATUS_UNINIT   -6
