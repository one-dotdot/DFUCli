#pragma once
#ifndef _AES_H_
#define _AES_H_

#include <stdint.h>
#include <string.h> // CBC mode, for memset

#define CBC 1

void AES128_CBC_encrypt_buffer(uint8_t* output, uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv);
void AES128_CBC_decrypt_buffer(uint8_t* output, uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv);

unsigned int AES_add_pkcs7Padding(unsigned char* input, unsigned int len);

#endif //_AES_H_
