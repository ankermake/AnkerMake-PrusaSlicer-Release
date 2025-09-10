#ifndef _ANKER_SECURITY_HPP_
#define _ANKER_SECURITY_HPP_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <unistd.h>
#endif

#include <openssl/aes.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/err.h>

//#include "elog_file_cfg.h"

namespace Slic3r {

#define ENC_KEY_DATA_SIZE 256
#define ENC_TEXT_DATA_SIZE_MAX 4096 + 24
#define BIG_ENDIAN_32BIT(X) (((X<<24)&0xff000000) + ((X<<8)&0x00ff0000) + ((X>>8)&0x0000ff00) + ((X>>24)&0x000000ff))

    typedef struct
    {
        uint16_t magic_number;
        uint8_t  setting_flag;
        uint8_t  cipher_flag;
        uint32_t length;
    }BLK_HEADER_T;

    typedef struct
    {
        BLK_HEADER_T header;
        uint8_t  ciphertext[ENC_KEY_DATA_SIZE];
    }KEY_BLK_DATA_T;

    typedef struct
    {
        BLK_HEADER_T header;
        uint8_t  ciphertext[0];
    }ST_BLK_DATA_T;

    typedef struct
    {
        uint8_t aes_key[16];
        uint8_t aes_iv[16];
        KEY_BLK_DATA_T sk_blk; // key block
        ST_BLK_DATA_T*  st_blk; // ciphertext block

    }ELOG_SEC_DATA_T;

    static ELOG_SEC_DATA_T elog_sec_data;

    // create 32Byte random number
    extern int elog_random_data_generate(uint8_t* data, const uint32_t& len);

    // RSA encrypt
    extern int elog_rsa_enc(const unsigned char* p_plain_data,
                            unsigned int data_len,
                            unsigned char* p_enc_data);

    extern int elog_rsa_dec(const unsigned char* p_enc_data,
                            unsigned int data_len,
                            unsigned char* p_dec_data,
                            char* p_key_file);

    // AES encrypt
    extern uint32_t PKCS7Padding(unsigned char* data, uint32_t len);
    
    extern uint32_t PKCS7DePadding(unsigned char* data, uint32_t len);

    extern int elog_aes128_cbc_pkcs5_enc(unsigned char* p_in_data,
                                         unsigned int len, 
                                         unsigned char* p_out_data,
                                         unsigned char* p_key, 
                                         unsigned char* p_iv);

    extern int elog_aes128_cbc_pkcs5_dec(unsigned char* p_in_data,
                                         unsigned int len, 
                                         unsigned char* p_out_data,
                                         unsigned char* p_key,
                                         unsigned char* p_iv);    

    // create key block
    extern int elog_key_block_generate(uint8_t** data,
                                       uint32_t* len);    

    // create ciphertext block
    extern int elog_ct_block_generate(uint8_t* in,
                                      uint32_t in_len, 
                                      uint8_t** out, 
                                      uint32_t* out_len);    

}
#endif