#ifndef RC4_H
#define RC4_H

void rc4_crypt(unsigned char *key, unsigned long key_len, unsigned char *Data, unsigned long Len); //加解密


void crypt_aux(unsigned char *key, unsigned long key_len, unsigned char *Data, int size, int cnt);
#endif