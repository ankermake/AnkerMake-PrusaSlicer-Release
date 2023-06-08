#include "NetworkAes.h"
#include <iostream>

static const std::string base64_chars = "";

int NetworkAes::aes_encrypt(uint8_t* plaintext, int plaintext_len, uint8_t* key, uint8_t* iv, uint8_t* ciphertext, MODE mode)
{
    return 0;
}

int NetworkAes::aes_decrypt(uint8_t* ciphertext, int ciphertext_len, uint8_t* key, uint8_t* iv, uint8_t* plaintext, MODE mode)
{
    return 0;
}

bool NetworkAes::is_base64(unsigned char c)
{
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string NetworkAes::base64_encode(char const* bytes_to_encode, int in_len)
{
    return "";
}

std::string NetworkAes::base64_decode(const std::string& encoded_string)
{
    return "";
}

std::string NetworkAes::base64Decode(const std::string& base64Str)
{
    return "";
}

std::string NetworkAes::base64Encode(const std::string& data)
{
    return "";
}

unsigned int NetworkAes::ssstrlen(const char* str)
{
    const char* cp = str;
    while (*cp++);
    return (cp - str - 1);
}

int NetworkAes::hex2char(uint8_t c)
{
    return ((c >= '0') && (c <= '9')) ? int(c - '0') :
        ((c >= 'A') && (c <= 'F')) ? int(c - 'A' + 10) :
        ((c >= 'a') && (c <= 'f')) ? int(c - 'a' + 10) :
        -1;
}

int NetworkAes::HexToAscii(char* hex, std::string& outStr)
{
    int hexLen = ssstrlen(hex);
    int asciiLen = 0;

    int arrayLength = ssstrlen(hex) + 1;

    char* asciiArray = (char*)malloc(arrayLength);
    memset(asciiArray, 0, arrayLength);
    for (int i = 0, cnt = 0; i < hexLen; i++)
    {
        char c = hex2char(hex[i]);

        if (-1 == c)
            continue;
        if (cnt) {
            cnt = 0;
            asciiArray[asciiLen++] += c;
        }
        else {
            cnt = 1;
            asciiArray[asciiLen] = c << 4;
        }
    }
    asciiArray[asciiLen] = '\0';

    if (asciiArray)
        outStr = asciiArray;

    free(asciiArray);
    return asciiLen;
}
