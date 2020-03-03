#include "aes.h"

#include <sgx_tcrypto.h>

int aes128_gcm_encrypt(
    const sgx_aes_gcm_128bit_key_t *p_key, 
    const uint8_t *p_src, uint32_t src_len, 
    uint8_t *p_dst, 
    uint8_t *p_iv, 
    uint32_t iv_len, 
    const uint8_t *p_aad, 
    uint32_t aad_len, 
    sgx_aes_gcm_128bit_tag_t *p_out_mac) {
}
