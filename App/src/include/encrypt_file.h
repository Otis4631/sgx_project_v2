#ifndef ENCRYPT_FILE
#define ENCRYPT_FILE

void encrypt_csv(char* filename_plaintext, char* filename_ciphertext, unsigned char* passwd, size_t passwd_len);
void decrypt_csv(char* filename_ciphertext, unsigned char* passwd, size_t passwd_len);


#endif