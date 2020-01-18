
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>
extern "C" {
#include "rc4.h"
#include "utils.h"
#include "base64.h"
}

void encrypt_csv(char* filename_plaintext, char* filename_ciphertext, unsigned char* passwd, size_t passwd_len) {
    FILE *fp_plain = fopen(filename_plaintext, "r");
    FILE *fp_cipher = fopen(filename_ciphertext, "w");
        if(!fp_plain || !fp_cipher) {
        printf("error fp\n");
    }
    char* line;
    char* base_out = nullptr;
    while((line = fgetl(fp_plain))) {
        if (48 > line[0] || line[0] > 57)
            continue;
        int fields = count_fields(line);
        float* input = parse_fields(line, fields, NULL);
        int i;
        rc4_crypt(passwd, passwd_len, (unsigned char *)input, 4 * fields);
        int base_out_len = ceil((fields *  4) / 3.0) * 4;
        if(!base_out)
            base_out = new char[base_out_len];
        base64_encode((unsigned char *)input, fields * 4, base_out);
        fwrite((const void*)base_out, sizeof(char), base_out_len, fp_cipher);
        char nl = '\n';
        fwrite((const void*)(&nl), 1, 1, fp_cipher);
        fclose(fp_cipher); 
        free(line);
        free(input);
        delete[] base_out;
        base_out = nullptr;
        line = NULL;
        FILE *fp_cipher = fopen(filename_ciphertext, "a");

    }
    fclose(fp_plain);
    fclose(fp_cipher); 
}

void decrypt_csv(char* filename_ciphertext, unsigned char* passwd, size_t passwd_len) {
    FILE *fp_cipher = fopen(filename_ciphertext, "r");
    char* line;
    while((line = fgetl(fp_cipher))) {
        int line_size = count_from_base64(line);
        int base_out_len = ceil(line_size / 3.0) * 4;
        int fields = line_size / 4;

        unsigned char* base_out = new unsigned char[line_size]();
        base64_decode((const char*)line, base_out_len, base_out);
        float* p;
        p = (float*)base_out;
        rc4_crypt(passwd, passwd_len, (unsigned char *)base_out, 4 * fields);
        for(int i=0; i< fields;i++) {
            printf("%.2f ", ((float*)base_out)[i]);
        }
        printf("\n");
        delete[] base_out;
        free(line);
    }
    fclose(fp_cipher); 
}

