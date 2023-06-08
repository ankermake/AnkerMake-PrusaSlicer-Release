#ifndef ANKER_NETWORK_AES_H
#define ANKER_NETWORK_AES_H

#include <stdint.h>
#include <string>

class NetworkAes
{
public:
    enum MODE {
        ECB,
        CBC,
        CFB
    };

    static int aes_encrypt(uint8_t* plaintext, int plaintext_len, uint8_t* key, uint8_t* iv, uint8_t* ciphertext, MODE mode = CBC);
    static int aes_decrypt(uint8_t* ciphertext, int ciphertext_len, uint8_t* key, uint8_t* iv, uint8_t* plaintext, MODE mode = CBC);

    static bool is_base64(unsigned char c);
    static std::string base64_encode(char const* bytes_to_encode, int in_len);
    static std::string base64_decode(const std::string& encoded_string);

    // base64 decode for openssl
    static std::string base64Decode(const std::string& base64Str);
    static std::string base64Encode(const std::string& str);

    static unsigned int ssstrlen(const char* str);
    static int hex2char(uint8_t c);
    static int HexToAscii(char* hex, std::string& outStr);
};



#endif // !ANKER_NETWORK_AES_H
