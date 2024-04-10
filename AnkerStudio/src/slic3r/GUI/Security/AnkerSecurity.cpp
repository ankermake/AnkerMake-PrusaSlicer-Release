#include "AnkerSecurity.hpp"
#include <iostream>
#include <ctime>
#include <random>
#include <chrono>

#ifndef __APPLE__
#include <Windows.h>
#include <winnt.rh>
#include <wincrypt.h>
#endif // !__APPLE__

const std::string global_pub_key = std::string("-----BEGIN PUBLIC KEY-----\n") +
                                   std::string("MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvsZNbZBjM/ZGU5tGXTYe\n") +
                                   std::string("gZX7rMaieSojpX29fgZu1bT1kounwk8srtbxfN3lj2cFm+O1wvQ1FDTYiZOG5OxO\n") +
                                   std::string("o+8BQPcI/8QjGYff4AskzGR8RK1dSYwpq9kPKrbMV1K15Gq+FLtsViIqg53fVjmx\n") +
                                   std::string("q2OkCEhX2+pY7FqgUvTYQNnxhsHs+csVRTt3HKc2tpY+rJmYQaS+/wqiCLBJSzuB\n") +
                                   std::string("ioCKzmNDTI0JH4pKNAl4JC6KhnQTHJvSuYXdym+EqmEzOyGKX/Ku/ngG76XZBJXI\n") +
                                   std::string("BfxHVkKc/xEuyRXhDFUYBhVt0/oOoXNjTlfdQGi8pSTNp4BTKMwMcFd5+I01akfT\n") +
                                   std::string("7wIDAQAB\n")+
                                   std::string("-----END PUBLIC KEY-----");

const char* global_priv_key = (" ");
std::mutex global_log_mutex;

namespace Slic3r {

    // create 32Byte random number
    int elog_random_data_generate(uint8_t* data, const uint32_t& len)
    {

#ifdef __APPLE__
        FILE* fp = NULL;
        uint32_t read_len = 0;

        if ((data == NULL) || (len == 0)) 
        {
            std::cout << "data or len is null. " << std::endl;
            return -1;
        }

        fp = fopen("/dev/urandom", "rb");
        if (fp == NULL)
            return -1;

        read_len = fread(data, 1, len, fp);
        if (read_len != len)
        {
            fclose(fp);
            std::cout << "read the data file is error. " << std::endl;
            return -1;
        }

        fclose(fp);
#else

        const std::string CHARACTERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

        std::random_device random_device;
        std::mt19937 generator(random_device());
        std::uniform_int_distribution<int> distribution(0, CHARACTERS.size() - 1);

        std::string random_string;

        for (std::size_t i = 0; i < len; ++i) 
            data[i] = CHARACTERS[distribution(generator)];
#endif        
            return 0;        
    }
    // RSA encrypt
    int elog_rsa_enc(const unsigned char* p_plain_data,
                     unsigned int data_len,
                     unsigned char* p_enc_data)
    {
        int len = -1;
        BIO* bio = nullptr;
        RSA* rsa_pub = nullptr;

        if ((p_plain_data == NULL) || (p_enc_data == NULL))
        {
            std::cout << "data or len is null. " << std::endl;
            goto res;
        }        

        //new logic
        bio = BIO_new_mem_buf(global_pub_key.c_str(), -1);  // write pub key to bio
        if (bio == NULL) {            
            std::cout << "Failed to create BIO" << std::endl;
            goto res;
        }

        rsa_pub = PEM_read_bio_RSA_PUBKEY(bio, NULL, NULL, NULL);  // get pub key from bio        
        if (rsa_pub == NULL) {            
            unsigned long err = ERR_get_error(); // get error code
            char* str = ERR_error_string(err, NULL);
            std::cout << "unable to read private key!" << std::endl;              
            goto res;
        }

        if (rsa_pub == NULL) {            
            std::cout << "unable to read public key!" << std::endl;
            goto res;
        }        

        len = RSA_public_encrypt(data_len,
                                     p_plain_data, 
                                     p_enc_data, 
                                     rsa_pub,
                                     RSA_PKCS1_PADDING);
        if (len == -1) {
            fprintf(stderr, "failed to encrypt\n");
            goto res;
        }        

    res:
        RSA_free(rsa_pub);
        BIO_free(bio);
        return len;
    }
   
    //not to use add by alves
    int elog_rsa_dec(const unsigned char* p_enc_data,
                     unsigned int data_len,
                     unsigned char* p_dec_data,
                     char* p_key_file)
    {
        int len = -1;
        BIO* bio = nullptr;
        RSA* rsa_pub = nullptr;        

        if ((p_enc_data == NULL) || (p_dec_data == NULL))
        {
            std::cout << "data or len is null. " << std::endl;
            goto res;
        }

        //new logic. no global_priv_key
        bio = BIO_new_mem_buf(global_priv_key, -1);  // write pub key to bio
        if (bio == NULL) {
            std::cout << "Failed to create BIO" << std::endl;
            goto res;
        }

        rsa_pub = PEM_read_bio_RSAPrivateKey(bio, NULL, NULL, NULL);  // get pri key from bio
        if (rsa_pub == NULL) {            
            std::cout << "unable to read private key!" << std::endl;
            goto res;
        }

        len = RSA_private_decrypt(data_len,
                                  p_enc_data,
                                  p_dec_data,
                                  rsa_pub,
                                  RSA_PKCS1_PADDING);
        if (len == -1) {            
            std::cout << "failed to decrypt!" << std::endl;
            goto res;
        }

    res:        
        RSA_free(rsa_pub);
        BIO_free(bio);
        return len;
    }


    // AES encrypt
    uint32_t PKCS7Padding(unsigned char* data, uint32_t len)
    {
        uint32_t remain;

        remain = 16 - len % 16;
        memset(&data[len], remain, remain);

        return len + remain;
    }


    uint32_t PKCS7DePadding(unsigned char* data, uint32_t len)
    {
        uint8_t remain, i;
        remain = data[len - 1];

        for (i = 0; i < remain; i++)
        {
            if (data[len - remain + i] != remain)
                return -1;
        }

        memset(&data[len - remain], 0, remain);

        return len - remain;
    }


    int elog_aes128_cbc_pkcs5_enc(unsigned char* p_in_data,
                                  unsigned int len,
                                  unsigned char* p_out_data,
                                  unsigned char* p_key,
                                  unsigned char* p_iv)
    {
        AES_KEY encrypt_key;

        uint8_t iv[16] = { 0 };
        memcpy(iv, p_iv, sizeof(iv));

        if (AES_set_encrypt_key(p_key, 128, &encrypt_key) < 0)
        {            
            std::cout << "set encrypt key failed" << std::endl;
            return -1;
        }

        int f_len = PKCS7Padding(p_in_data, len);

        AES_cbc_encrypt(p_in_data, 
                        p_out_data,
                        f_len, 
                        &encrypt_key,
                        iv,
                        AES_ENCRYPT);

        return f_len;
    }


    int elog_aes128_cbc_pkcs5_dec(unsigned char* p_in_data,
                                  unsigned int len,
                                  unsigned char* p_out_data,
                                  unsigned char* p_key,
                                  unsigned char* p_iv)
    {
        AES_KEY decrypt_key;

        if (AES_set_decrypt_key(p_key, 128, &decrypt_key) < 0)
        {            
            std::cout << "set decrypt key failed" << std::endl;
            return -1;
        }

        int f_len = 0;
        unsigned char iv[16] = { 0 };
        memcpy(iv, p_iv, sizeof(iv));

        AES_cbc_encrypt(p_in_data,
                        p_out_data, 
                        len,
                        &decrypt_key,
                        iv,
                        AES_DECRYPT);

        f_len = PKCS7DePadding(p_in_data, len);

        return f_len;
    }


    // create key block  header
    int elog_key_block_generate(uint8_t** data, uint32_t* len)
    {
        uint8_t rm_tmp[32] = { 0 };
        unsigned char enc_tmp[ENC_KEY_DATA_SIZE] = { 0 };

        char pubKeyPath[ENC_KEY_DATA_SIZE] = { 0 };
        // create 32Byte random numbers
        uint32_t rm_tmp_length = sizeof(rm_tmp);
        elog_random_data_generate(rm_tmp, rm_tmp_length);

        // rsa encrypt
        // elog_rsa_enc(rm_tmp, sizeof(rm_tmp), enc_tmp, PUBLIC_KEY_PATH);
        elog_rsa_enc(rm_tmp, sizeof(rm_tmp), enc_tmp);        

        memcpy(elog_sec_data.aes_key, rm_tmp, 16);
        memcpy(elog_sec_data.aes_iv, rm_tmp + 16, 16);

        elog_sec_data.sk_blk.header.magic_number = 0x5cef; //big endian
        elog_sec_data.sk_blk.header.setting_flag = 0;
        elog_sec_data.sk_blk.header.cipher_flag = 1;
        elog_sec_data.sk_blk.header.length = 0x00010000; //big endian
        memcpy(elog_sec_data.sk_blk.ciphertext, enc_tmp, ENC_KEY_DATA_SIZE);

      
        *data = (uint8_t*)&elog_sec_data.sk_blk;
        *len = sizeof(KEY_BLK_DATA_T);
        return 0;
    }


    // create ciphertext block content
    int elog_ct_block_generate(uint8_t* in, 
                               uint32_t in_len, 
                               uint8_t** out,
                               uint32_t* out_len)
    {
        std::unique_lock<std::mutex> lock(global_log_mutex);
        uint32_t ct_len = 0;
        if (elog_sec_data.st_blk)
        {
            free(elog_sec_data.st_blk);
            elog_sec_data.st_blk = nullptr;
        }

        elog_sec_data.st_blk = (ST_BLK_DATA_T*)malloc(in_len * (sizeof(uint8_t)+(sizeof(BLK_HEADER_T))));

        if(!elog_sec_data.st_blk)
            return -1;
        // aes128-cbc encrypt
        ct_len = elog_aes128_cbc_pkcs5_enc(in, 
                                            in_len, 
                                            elog_sec_data.st_blk->ciphertext,
                                            elog_sec_data.aes_key, 
                                            elog_sec_data.aes_iv);
        
        if(ct_len == -1)
            return -1;
            
        elog_sec_data.st_blk->header.magic_number = 0xc0f1; //big endian
        elog_sec_data.st_blk->header.setting_flag = 0;
        elog_sec_data.st_blk->header.cipher_flag = 0;
        elog_sec_data.st_blk->header.length = BIG_ENDIAN_32BIT(ct_len); //big endian

        *out = (uint8_t*)elog_sec_data.st_blk;
        *out_len = ct_len + sizeof(BLK_HEADER_T);


        return 0;
    }

}
