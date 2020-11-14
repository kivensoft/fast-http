/**
 * AES block cipher
 *
 *  Copyright (C) 2006-2010, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
Example:
    unsigned char pwd[] = "password12345678";
    unsigned char iv[] = "1234567887654321";
    unsigned char text[] = "12345678abcdefgh";
    unsigned char enc_data[] = {
        0x7f, 0x56, 0x67, 0xd8, 0x5f, 0xf5, 0x99, 0xbc,
        0x42, 0xf1, 0xd8, 0x2d, 0xa2, 0xcb, 0xb0, 0x85,
        0x4a, 0xe0, 0xa4, 0x8e, 0x8c, 0x22, 0x8a, 0x3f,
        0x9e, 0xec, 0x0d, 0xa9, 0xa9, 0xa6, 0x2a, 0xba
    };
    size_t tlen = strlen((char*) text);
    size_t len = aes_pkcs7_padding_size(tlen);
    unsigned char data[len], *pdata = data;

    aes_ctx_t pctx;
    aes_init(&pctx, AES_ENCRYPT, pwd, 128, iv);
    pdata += aes_update(&pctx, text, 7, pdata);
    pdata += aes_update(&pctx, text + 7, 2, pdata);
    pdata += aes_final(&pctx, text + 9, tlen - 9, pdata);
 */

#pragma once
#ifndef __AES_H__
#define __AES_H__

#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PKCS7_PADDING_SIZE
#   define PKCS7_PADDING_SIZE(len, align) (((len / align) + 1) * align)
#endif

#ifndef pkcs7_padding_func
#   define pkcs7_padding_func
static inline void pkcs7_padding(void* data, size_t datalen, size_t align) {
    size_t n = PKCS7_PADDING_SIZE(datalen, align);
    uint8_t m = (uint8_t)(n - datalen);
    uint8_t *p = (uint8_t*) data + datalen, *pe = (uint8_t*) data + n;
    while (p < pe) *p++ = m;
}
#endif // pkcs7_padding_func

#define AES_ENCRYPT     1
#define AES_DECRYPT     0

/** AES context structure */
typedef struct {
    int mode;                   /*!< encrypt/decrypt mode */
    unsigned char iv[16];       /*!< iv                 */
    unsigned char input[16];    /*!< input buffer       */
    uint32_t input_len;         /*!< input buffer len   */
    int nr;                     /*!<  number of rounds  */
    uint32_t *rk;               /*!<  AES round keys    */
    uint32_t buf[68];           /*!<  unaligned data    */
} aes_ctx_t;

/** AES key schedule (encryption)
 * @param pctx     AES context to be initialized
 * @param key      encryption key
 * @param keysize  must be 128, 192 or 256
 * @return         true if successful, or false
 */
bool aes_setkey_enc(aes_ctx_t* pctx, const unsigned char *key, unsigned int keysize);

/** AES key schedule (decryption)
 * @param pctx     AES context to be initialized
 * @param key      decryption key
 * @param keysize  must be 128, 192 or 256
 * @return         true if successful
 */
bool aes_setkey_dec(aes_ctx_t* pctx, const unsigned char *key, unsigned int keysize);

/** AES-ECB block encryption/decryption
 * @param pctx     AES context
 * @param mode     AES_ENCRYPT or AES_DECRYPT
 * @param input    16-byte input block
 * @param output   16-byte output block
 */
void aes_crypt_ecb(aes_ctx_t* pctx, const unsigned char input[16], unsigned char output[16]);

/** AES-CBC buffer encryption/decryption Length should be a multiple of the block size (16 bytes)
 * @param pctx     AES context
 * @param mode     AES_ENCRYPT or AES_DECRYPT
 * @param length   length of the input data
 * @param iv       initialization vector (updated after use)
 * @param input    buffer holding the input data
 * @param output   buffer holding the output data
 * @return         true if successful
 */
bool aes_crypt_cbc(aes_ctx_t* pctx, size_t length, unsigned char iv[16], const unsigned char *input, unsigned char *output);

/** AES-CBC-PKCS7_PADDING require len
 * @param len      data len
 * @return         padding require len
 */
static inline size_t aes_pkcs7_padding_size(size_t len) {
    return PKCS7_PADDING_SIZE(len, 16);
}

/** AES-CBC-PKCS7_PADDING fill padding
 * @param data     fill data
 * @param len      data len
 */
static inline void aes_pkcs7_padding(void* data, size_t len) {
    return pkcs7_padding(data, len, 16);
}

/** AES-CBC-PKCS7-PADDING 算法初始化，带向量模式
 * @param pctx     AES上下文对象
 * @param mode     加解密模式 AES_ENCRYPT or AES_DECRYPT
 * @param key      秘钥
 * @param keysize  秘钥长度, 128 or 192 or 256
 * @param iv       初始化向量，16字节长
 */
void aes_init(aes_ctx_t* pctx, int mode, const unsigned char *key, uint32_t keysize, const unsigned char iv[16]);

/** AES-CBC-PKCS7-PADDING 流式加密，可连续调用
 * @param pctx     AES上下文对象
 * @param input    输入缓冲区
 * @param input_len 输入缓冲区长度
 * @param output   输出缓冲区
 * @return         输出缓冲区写入字节数
 */
size_t aes_update(aes_ctx_t* pctx, const void* input, size_t input_len, void* output);

/** AES-CBC-PKCS7-PADDING 流式加密，最后结束时调用
 * @param pctx     AES上下文对象
 * @param input    输入缓冲区
 * @param input_len 输入缓冲区长度
 * @param output   输出缓冲区
 * @return         输出缓冲区写入字节数
 */
size_t aes_final(aes_ctx_t* pctx, const void* input, size_t input_len, void* output);

#ifdef __cplusplus
}
#endif

#endif // __AES_H__
