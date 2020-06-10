#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "rc4.h"
typedef unsigned long ULONG;



void crypt_aux(const unsigned char *key, unsigned long key_len, unsigned char *Data, int size, int batch) {
    #ifdef CRYPTO
    unsigned char* p;
    p = Data;
    for(int i = 0; i < batch; i ++, p+= size) {
        rc4_crypt(key, key_len, p, size);
    }
    #endif
}


void rc4_crypt(unsigned char *key, unsigned long key_len, unsigned char *Data, unsigned long Len) //加解密
{
	int i =0, j = 0;
    char k[256] = {0};
	unsigned char s[256] = {0};
    unsigned char tmp = 0;
    for (i=0;i<256;i++) {
        s[i] = i;
        k[i] = key[i%key_len];
    }
    for (i=0; i<256; i++) {
        j=(j+s[i]+k[i])%256;
        tmp = s[i];
        s[i] = s[j]; //交换s[i]和s[j]
        s[j] = tmp;
    }
    i = 0, j = 0;
	int t = 0;
    unsigned long q = 0;
    //unsigned char tmp;
    for(q=0;q<Len;q++) {
        i=(i+1)%256;
        j=(j+s[i])%256;
        tmp = s[i];
        s[i] = s[j]; //交换s[x]和s[y]
        s[j] = tmp;
        t=(s[i]+s[j])%256;
        Data[q] ^= s[t];
     }
} 
