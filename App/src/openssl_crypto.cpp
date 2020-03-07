

#include <iostream>
#include <cstring>
#include <stdint.h>
#include <assert.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <string.h>

#include "openssl_crypto.h"


#define BUF_SIZE 1024

using namespace std;

int md5_encrypt(const void *data, size_t len, unsigned char *md5)
{
    if (data == NULL || len <= 0 || md5 == NULL)
    {
        printf("Input param invalid!\n");
        return -1;
    }
    MD5_CTX pk_ctx;
    MD5_Init(&pk_ctx);
    MD5_Update(&pk_ctx, data, len);
    MD5_Final(md5, &pk_ctx);
    return 0;
}


int OpenSSLCrypto::set_mode(int _mode) {
    int cipher = mode & CIPHER;
    int param = mode & PARAM;
    mode = _mode;
    if(mode & MODE_ENCRYPT) {
        enc = 1;
    }
    else if(mode & MODE_DECRYPT)
        enc = 0;
}

void OpenSSLCrypto::clean_up() {
    if (pk_ctx)
        EVP_PKEY_CTX_free(pk_ctx);
    if (pkey)
        EVP_PKEY_free(pkey);
    if (fp)
        fclose(fp);
    if (sym_ctx)
        EVP_CIPHER_CTX_free(sym_ctx);
    if (rsa)
        RSA_free(rsa);
    if (key)
        delete[] key;

    if (iv)
        delete[] iv;
        

    pk_ctx = NULL, pkey = NULL, fp = NULL, sym_ctx = NULL, rsa = NULL, key = NULL, iv = NULL;
}

int OpenSSLCrypto::init_status()
{
    pkey = EVP_PKEY_new();
    if (NULL == pkey)
    {
        err_handle();
        return ERR_NEW_FAILED;
    }

    if (mode & CIPHER_AES)
    {
        sym_ctx = EVP_CIPHER_CTX_new();
        if (sym_ctx == NULL)
        {
            err_handle();
            return -1;
        }
        const EVP_CIPHER *tmp_cipher = NULL;

        if (mode & PARAM_GCM_128)
            tmp_cipher = EVP_aes_128_gcm();

        if (mode & MODE_ENCRYPT)
            enc = 1;
        else if (mode & MODE_DECRYPT)
            enc = 0;
        else
        {
            printf("Mode Error!\n");
            return ERR_BAD_MODE;
        }
        if (EVP_CipherInit(sym_ctx, tmp_cipher, NULL, NULL, enc) <= 0)
        {
            err_handle();
            return -1;
        }
    }

    return 0;
}

int OpenSSLCrypto::set_RSA_padding()
{
    if (mode & RSA_OAEP)
    {
        if (EVP_PKEY_CTX_set_rsa_padding(pk_ctx, RSA_PKCS1_OAEP_PADDING) <= 0 
            || !EVP_PKEY_CTX_set_rsa_oaep_md(pk_ctx, EVP_sha256()))
        {
            err_handle();
            return ERR_RUN_FAILED;
        }
    }

    else
        return ERR_BAD_MODE;
    return 0;
}

OpenSSLCrypto::OpenSSLCrypto()
{
    init_status();
}

OpenSSLCrypto::OpenSSLCrypto(int _mode) : mode(_mode)
{
    init_status();
}

void OpenSSLCrypto::err_handle()
{
    e = ERR_peek_last_error();
    char *c_err_str = (char *)ERR_reason_error_string(e);
    if (c_err_str)
        printf("%s\n", c_err_str);
    ERR_print_errors_fp(stderr);
    ERR_clear_error();
}

int OpenSSLCrypto::open_public_key(const char *pub_key_file)
{
    if (!(mode & CIPHER_RSA))
        return ERR_BAD_MODE;
    fp = fopen(pub_key_file, "r");
    if (NULL == fp)
    {
        printf("open_public_key: error on reading file!\n");
        return ERR_OPEN_FAILED;
    }

    char *header = new char[1024]();
    if (!fgets(header, 1024, fp))
    {
        fclose(fp);
        fp = NULL;
        delete[] header;
        return ERR_READ_FAILED;
    }
    fseek(fp, 0, 0);
    if (0 == strncmp(header, pkcs8_header.c_str(), pkcs8_header.size()))
    {
        rsa = PEM_read_RSA_PUBKEY(fp, NULL, NULL, NULL);
    }
    else if (0 == strncmp(header, pkcs1_header.c_str(), pkcs1_header.size()))
    {
        rsa = PEM_read_RSAPublicKey(fp, NULL, NULL, NULL);
    }
    delete[] header;
    fclose(fp);
    fp = NULL;
    if (rsa == NULL)
    {
        err_handle();
        return ERR_READ_FAILED;
    }
    EVP_PKEY_set1_RSA(pkey, rsa);
    RSA_print_fp(stdout, rsa, 0);
    pk_ctx = EVP_PKEY_CTX_new(pkey, eng);
    if (NULL == pk_ctx)
    {
        err_handle();
        return ERR_NEW_FAILED;
    }
    if (EVP_PKEY_encrypt_init(pk_ctx) <= 0)
    {
        err_handle();
        return ERR_RUN_FAILED;
    }
    if (set_RSA_padding() < 0)
        return ERR_RUN_FAILED;
    return 0;
}


int OpenSSLCrypto::open_private_key(const char *file, const char *passwd)
{
    if (!(mode & (CIPHER_RSA | MODE_DECRYPT)))
        return -1;
    fp = fopen(file, "r");
    if (NULL == fp)
    {
        printf("open_public_key: error on reading file!\n");
        return ERR_OPEN_FAILED;
    }
    char *header = new char[1024]();
    if (!fgets(header, 1024, fp))
    {
        fclose(fp);
        fp = NULL;
        delete[] header;
        return ERR_READ_FAILED;
    }
    fseek(fp, 0, 0);

    PEM_read_PrivateKey(fp, &pkey, NULL, NULL);
    if (NULL == pkey)
    {
        printf("open_private_key EVP_PKEY_new failed\n");
        fclose(fp);
        fp = NULL;
        return ERR_READ_FAILED;
    }
    pk_ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (NULL == pk_ctx)
    {
        printf("failed to open pk_ctx.\n");
        return ERR_NEW_FAILED;
    }
    if (EVP_PKEY_decrypt_init(pk_ctx) <= 0)
    {
        printf("ras_pubkey_encryptfailed to EVP_PKEY_encrypt_init.\n");
        return ERR_RUN_FAILED;
    }
    if (set_RSA_padding() < 0)
        return ERR_RUN_FAILED;

    return 0;
}

int OpenSSLCrypto::RSA_encrypt()
{
    if (EVP_PKEY_encrypt(pk_ctx, out, out_len, in, in_len) <= 0)
    {
        err_handle();
        return ERR_RUN_FAILED;
    }
    else
        return 0;
}

int OpenSSLCrypto::RSA_decrypt()
{
    if (EVP_PKEY_decrypt(pk_ctx, NULL, out_len, in, in_len) <= 0)
    {
        err_handle();
        return ERR_RUN_FAILED;
    }

    if (EVP_PKEY_decrypt(pk_ctx, out, out_len, in, in_len) <= 0)
    {
        err_handle();
        return ERR_RUN_FAILED;
    }
    else
    {
        return 0;
    }
}

int OpenSSLCrypto::AES_update(uint8_t *_out, int *_out_len, uint8_t *_in, int _in_len)
{
    if (!key || !iv)
    {
        return ERR_STATUS_UNINIT;
    }
    if (!(mode & CIPHER_AES))
        return ERR_BAD_MODE;

    if (EVP_CipherUpdate(sym_ctx, _out, _out_len, _in, _in_len) <= 0)
    {
        err_handle();
        return ERR_RUN_FAILED;
    }
    return 0;
}

int OpenSSLCrypto::AES_final(uint8_t *_out, int *_out_len)
{
    if (!key || !iv)
    {
        return ERR_STATUS_UNINIT;
    }
    if (!(mode & CIPHER_AES))
        return ERR_BAD_MODE;
    if (EVP_CipherFinal(sym_ctx, _out, _out_len) <= 0)
    {
        err_handle();
        return ERR_RUN_FAILED;
    }
    return 0;
}

int OpenSSLCrypto::AES_crypt()
{
    int tmp_out_len = 0;
    if (EVP_CipherUpdate(sym_ctx, out, (int *)out_len, in, in_len) <= 0)
    {
        err_handle();
        return ERR_RUN_FAILED;
    }
    tmp_out_len += *(int *)out_len;
    if (EVP_CipherFinal(sym_ctx, out + tmp_out_len, (int *)out_len) <= 0)
    {
        err_handle();
        return ERR_RUN_FAILED;
    }
    tmp_out_len += *(int *)out_len;
    *out_len = tmp_out_len;
    return 0;
}

// int OpenSSLCrypto::get_raw_private_key(uint8_t *priv, size_t *len)
// {
//     if (mode & CIPHER_RSA)
//     {
//         if(EVP_PKEY_get_raw_private_key(pkey, priv, len) <= 0) {
//             err_handle();
//             return ERR_RUN_FAILED;
//         }
//     }
//     else
//         return ERR_BAD_MODE;

//     return 0;
// }

int OpenSSLCrypto::get_raw_public_key(uint8_t *n_c, size_t *n_len, uint8_t *e_c, size_t *e_len)
{
    if (mode & CIPHER_RSA)
    {
        const BIGNUM **n = (const BIGNUM **)new BIGNUM**;
        const BIGNUM **e = (const BIGNUM **)new BIGNUM**;

        RSA_get0_key(rsa, n, e, NULL);
        *n_len = BN_num_bytes(*n);
        *e_len = BN_num_bytes(*e);
        if(n_c && e_c)
            if(BN_bn2bin(*n, n_c) <= 0 || BN_bn2bin(*e, e_c) <= 0 ) {
                err_handle();
                return ERR_RUN_FAILED;
            }
            return 0;
    }
    else
        return ERR_BAD_MODE;
        
    return 0;
}
int OpenSSLCrypto::crypt(uint8_t *_out, size_t *_out_len, uint8_t *_in, size_t _in_len)
{
    out = _out;
    out_len = _out_len;
    in = _in;
    in_len = _in_len;

    if (mode & CIPHER_RSA)
    {
        if (mode & MODE_DECRYPT)
            return RSA_decrypt();
        else if (mode & MODE_ENCRYPT)
        {
            return RSA_encrypt();
        }
        else
            return ERR_BAD_MODE;
    }
    else if (mode & CIPHER_AES)
    {
        return AES_crypt();
    }
    else
        return ERR_BAD_MODE;
}

int OpenSSLCrypto::file_crypt(FILE *in, FILE *out)
{
    unsigned char inbuf[1024], outbuf[1024 + EVP_MAX_BLOCK_LENGTH];
    int inlen, outlen;
    if (mode & CIPHER_AES)
    {
        for (;;)
        {
            inlen = fread(inbuf, 1, 1024, in);
            if (inlen <= 0)
                break;
            if (!EVP_CipherUpdate(sym_ctx, outbuf, &outlen, inbuf, inlen))
            {
                /* Error */
                err_handle();
                return ERR_RUN_FAILED;
            }
            fwrite(outbuf, 1, outlen, out);
        }
        if (!EVP_CipherFinal_ex(sym_ctx, outbuf, &outlen))
        {
            /* Error */
            err_handle();
            return ERR_RUN_FAILED;
        }
        fwrite(outbuf, 1, outlen, out);
        return 0;
    }
    else
        return ERR_BAD_MODE;
}

OpenSSLCrypto::~OpenSSLCrypto()
{
    if (pk_ctx)
        EVP_PKEY_CTX_free(pk_ctx);
    if (pkey)
        EVP_PKEY_free(pkey);
    if (fp)
        fclose(fp);
    if (sym_ctx)
        EVP_CIPHER_CTX_free(sym_ctx);
    if (rsa)
        RSA_free(rsa);
}

size_t OpenSSLCrypto::get_key_length()
{
    if (mode & CIPHER_AES)
        return EVP_CIPHER_CTX_key_length(sym_ctx);
    else
        return 0;
}

size_t OpenSSLCrypto::get_iv_length()
{
    if (mode & CIPHER_AES)
        return EVP_CIPHER_CTX_iv_length(sym_ctx);
    else
        return 0;
}

int OpenSSLCrypto::AES_init(uint8_t *_key, uint8_t *_iv)
{

    key = _key;
    iv = _iv;
    int key_len = EVP_CIPHER_CTX_key_length(sym_ctx);
    int iv_len = EVP_CIPHER_CTX_iv_length(sym_ctx);

    key = new uint8_t[key_len];
    iv = new uint8_t[iv_len];
    
    memcpy(key, _key, key_len);
    memcpy(iv, _iv, iv_len);
    if (mode & CIPHER_AES)
    {
        if (EVP_CipherInit(sym_ctx, NULL, key, iv, enc) <= 0)
        {
            err_handle();
            return ERR_RUN_FAILED;
        }
    }
}

void rsa_test()
{
    char pub[] = "Enclave/public.pem";
    char priv[] = "Enclave/private.pem";

    OpenSSLCrypto e(RSA_OAEP | MODE_ENCRYPT);
    OpenSSLCrypto d(RSA_OAEP | MODE_DECRYPT);

    int buf_size = 384;
    char plain[] = "lizheng";
    uint8_t *ciphertxt = new uint8_t[buf_size];
    size_t cipher_len = 386;
    char *deciphertxt = new char[buf_size];
    size_t decipher_len = 386;

    e.open_public_key(pub);
    e.crypt(ciphertxt, &cipher_len, (uint8_t *)plain, sizeof(plain));

    d.open_private_key(priv, NULL);
    d.crypt((uint8_t *)deciphertxt, &decipher_len, ciphertxt, cipher_len);
    assert(!(strncmp(plain, deciphertxt, sizeof(plain))));
}

void aes_128_gcm_test()
{
    unsigned char key[] = "0123456789abcdeF";
    unsigned char iv[] = "123456788765";

    OpenSSLCrypto e(AES_128_GCM | MODE_ENCRYPT);
    OpenSSLCrypto d(AES_128_GCM | MODE_DECRYPT);

    assert(sizeof(key) - 1 == e.get_key_length());
    assert(sizeof(iv) - 1 == e.get_iv_length());

    e.AES_init(key, iv);
    d.AES_init(key, iv);

    int buf_size = 384;
    unsigned char plain[] = "kkkkkkkkk";
    uint8_t *ciphertxt = new uint8_t[buf_size];
    size_t cipher_len = 386;
    unsigned char *deciphertxt = new uint8_t[buf_size];
    size_t decipher_len = 386;

    e.crypt(ciphertxt, &cipher_len, plain, sizeof(plain));
    d.crypt(deciphertxt, &decipher_len, ciphertxt, cipher_len);
    assert(!(strncmp((const char *)plain, (const char *)deciphertxt, sizeof(plain))));
}

// int main()
// {
//     OpenSSLCrypto d(RSA_OAEP);
//     printf("%d\n", d.open_public_key("/data/lz/sgx_project_v2/Enclave/public.pem"));
//     size_t n_len = 0;
//     size_t e_len = 0;
//     d.get_raw_public_key(NULL, &n_len, NULL, &e_len);
//     uint8_t* n = new uint8_t[n_len];
//     uint8_t* e = new uint8_t[e_len];
//     d.get_raw_public_key(n, &n_len, e, &e_len);
// }

