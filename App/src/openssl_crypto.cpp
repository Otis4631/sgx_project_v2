
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/md5.h>
#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include <iostream>
#include <cstring>
#include <assert.h>
#include "openssl_crypto.h"

#define BUF_SIZE 1024

using namespace std;
 
int md5_encrypt(const void* data, size_t len, unsigned char* md5)
{
	if (data == NULL || len <= 0 || md5 == NULL) {
		printf("Input param invalid!\n");
		return -1;
	}
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, data, len);
	MD5_Final(md5, &ctx);
	return 0;
}

class OpenSSLCrypto {
	public:
		OpenSSLCrypto();
		OpenSSLCrypto(int _mode);
		void err_handle();
		int init_status();
		int open_public_key(const char* pub_key_file);	
		int open_private_key(const char* priv_key_file, const char* passwd);	
		int encrypt(const unsigned char* data, size_t len, uint8_t *encrypt_data, size_t *encrypt_data_len);
		int decrypt(const unsigned char *encrypt_data, size_t encrypt_data_len, unsigned char *decrypt_data, size_t *decrypt_data_len);
		~OpenSSLCrypto();
	private:
		int mode;
		EVP_PKEY *key = NULL;
		RSA *rsa = NULL;
		EVP_PKEY_CTX *ctx = NULL;
		ENGINE *eng;
		size_t out_len;
		FILE* fp = NULL;
		BIO * bio = NULL;
		unsigned long e;

		const string pkcs1_header = "-----BEGIN RSA P";
		const string pkcs8_header = "-----BEGIN P";
};
int OpenSSLCrypto::init_status(){
	bio = BIO_new(BIO_s_mem());
	if(bio == NULL) {
		printf("can not create new BIO\n");
		return -1;
	}
	return 0;
}

OpenSSLCrypto::OpenSSLCrypto() {
	init_status();	
}
OpenSSLCrypto::OpenSSLCrypto(int _mode): mode(_mode){
	init_status();
}

void OpenSSLCrypto::err_handle() {
	e = ERR_peek_last_error();
	char* err_str = new char[BUF_SIZE]();
	char* c_err_str = (char*)ERR_reason_error_string(e);
	if(c_err_str)
		printf("%s\n", c_err_str);	
	ERR_print_errors(bio);
	BIO_read(bio, err_str, BUF_SIZE);
	BIO_read(bio, err_str + 100, BUF_SIZE);

	printf("ERR: %s", err_str);
	printf("ERR: %s\n", err_str + 100);
	ERR_clear_error();
	free(err_str);

}

int OpenSSLCrypto::open_public_key(const char* pub_key_file) {
	if(!(mode & (RSA_ENCRYPT | RSA_DECRYPT)))
		return -1;
	fp = fopen(pub_key_file, "r");
	if(NULL == fp) {
		printf("open_public_key: error on reading file!\n");
		return -2;
	}

	char* header = new char[1024]();
	if(!fgets(header, 1024, fp)){
		fclose(fp);
		fp = NULL;
       	delete[] header;
        return -2;
    }
	fseek(fp, 0, 0);
	if( 0 == strncmp(header, pkcs8_header.c_str(),pkcs8_header.size())){
		rsa = PEM_read_RSA_PUBKEY(fp,NULL,NULL,NULL);
	}
	else if(0 == strncmp(header, pkcs1_header.c_str(),pkcs1_header.size())){
		rsa = PEM_read_RSAPublicKey(fp,NULL,NULL,NULL);
	}
	delete[] header;
	fclose(fp);
	fp = NULL;
	if (rsa == NULL)
	{
		printf("open_public_key: failed to PEM_read_bio_RSAPublicKey!\n");
		return -3;
	}

	key = EVP_PKEY_new();
	if (NULL == key)
	{
		printf("open_public_key: EVP_PKEY_new failed\n");
		return -4;
	}
	EVP_PKEY_assign_RSA(key, rsa);
	rsa = NULL;
	ctx = EVP_PKEY_CTX_new(key,eng);
	if (NULL == ctx)
	{
		printf("failed to open ctx.\n");
		return -5;
	}
 if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0)
 {
	 err_handle();
	 return -1;

 }
	if (EVP_PKEY_encrypt_init(ctx) <= 0)
	{
		printf("ras_pubkey_encryptfailed to EVP_PKEY_encrypt_init.\n");
		return -6;
	}
	return 0;
}	

int OpenSSLCrypto::open_private_key(const char* file, const char* passwd) {
	if(!(mode & (RSA_DECRYPT | RSA_ENCRYPT)))
		return -1;
	fp = fopen(file, "r");
	if(NULL == fp) {
		printf("open_public_key: error on reading file!\n");
		return -2;

	}
	char* header = new char[1024]();
	if(!fgets(header, 1024, fp)){
		fclose(fp);
		fp = NULL;
       	delete[] header;
        return -2;
    }
	fseek(fp, 0, 0);
	key = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
	if (NULL == key)
	{
		printf("open_private_key EVP_PKEY_new failed\n");
		fclose(fp);
		fp = NULL;
		return -4;
	}
 	rsa = NULL;
	ctx = EVP_PKEY_CTX_new(key, NULL);
	if (NULL == ctx)
	{
		printf("failed to open ctx.\n");
		return -5;
	}
	if (EVP_PKEY_decrypt_init(ctx) <= 0)
	{
		printf("ras_pubkey_encryptfailed to EVP_PKEY_encrypt_init.\n");
		return -6;
	}

	return 0;
}

int OpenSSLCrypto::encrypt(const unsigned char* data, size_t len, uint8_t *encrypt_data, size_t *encrypt_data_len) {
	if(mode & RSA_ENCRYPT) {
		int ret = EVP_PKEY_encrypt(ctx, NULL, &out_len, data, len);
		if ( ret <= 0)
		{
			printf("Error:%d, ras_pubkey_encryptfailed to EVP_PKEY_encrypt.\n", ret);
			return -1;
		}
		if(out_len > *encrypt_data_len) {
			printf("data buffer is too small!\n");
			return -1;
		}
		else if(EVP_PKEY_encrypt(ctx, encrypt_data, encrypt_data_len, data, len) <= 0) {
			printf("unkown error!\n");
			return -1;
		}
		else 
			return 0;
	}
	else
		return -2;
}

int OpenSSLCrypto::decrypt(const unsigned char *encrypt_data, size_t encrypt_data_len, unsigned char *decrypt_data, size_t *decrypt_data_len) {
	ERR_clear_error();
	if(mode & RSA_DECRYPT)
		if (EVP_PKEY_decrypt(ctx, decrypt_data, decrypt_data_len, encrypt_data, encrypt_data_len) <= 0)
		{
			err_handle();			
			return -1;
		}
		else
		{
			return 0;
		}
	else 
		return -2;
		
}

OpenSSLCrypto::~OpenSSLCrypto(){
	if(ctx)		EVP_PKEY_CTX_free(ctx);
	if(key)		EVP_PKEY_free(key);
	if(rsa)		RSA_free(rsa);
	if(fp)		fclose(fp);
}

int main() {
	// OpenSSLCrypto c(RSA_ENCRYPT);
	// OpenSSLCrypto d(RSA_DECRYPT);
	// printf("opening priv: %d\n",d.open_private_key("Enclave/private.pem", NULL));
	// printf("opening pub: %d\n",c.open_public_key("Enclave/public.pem"));
	// const unsigned char raw_data[] = "lizheng";

	// uint8_t* decrypt_data = new uint8_t[10]();
	// size_t a = 384;
	// size_t d_len = 385;
	// unsigned char* encrypt_data = (uint8_t*)OPENSSL_malloc(a);
	// printf("encrypt:%d \n", c.encrypt(raw_data, sizeof(raw_data), encrypt_data, &a));

	// d.decrypt(encrypt_data, a, decrypt_data, &d_len);
	// printf("decrypt: '%s'\n", decrypt_data);
	// assert(d_len == 8);


	
}
