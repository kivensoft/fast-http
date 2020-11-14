/**
 * DES block cipher
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
/* example:
    char *pwd = "password";
    unsigned char iv[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    char *text = "12345678abcdefgh";
    size_t len = PKCS5_PADDING_SIZE(strlen(text));
    unsigned char data[len], *pdata = data;

    des_ctx_t ctx;
    des_init(&ctx, DES_ENCRYPT, (unsigned char*) pwd, iv);
    pdata += des_update(&ctx, text, 7, pdata);
    des_final(&ctx, text + 7, strlen(text) - 7, pdata);
    for (int i = 0; i < len; ++i)
        printf("%02x", data[i]);
 */

#pragma once
#ifndef __DES_H__
#define __DES_H__

#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

#define DES_ENCRYPT     1
#define DES_DECRYPT     0

#define DES_KEY_SIZE    8

#ifndef PKCS5_PADDING_SIZE
#   define PKCS5_PADDING_SIZE(x) (((x >> 3) + 1) << 3)
#endif

#ifndef pkcs5_padding_func
#   define pkcs5_padding_func
static inline void pkcs5_padding(void* data, size_t datalen) {
    size_t n = PKCS5_PADDING_SIZE(datalen);
    uint8_t m = (uint8_t)(n - datalen);
    uint8_t *p = (uint8_t*) data + datalen, *pe = (uint8_t*) data + n;
    while (p < pe) *p++ = m;
}
#endif // pkcs5_padding_func

/** DES context structure */
typedef struct {
    int mode;                   /*!<  encrypt/decrypt   */
    unsigned char iv[8];        /*!<  iv                */
    uint32_t sk[32];            /*!<  DES subkeys       */
    unsigned char input[8];     /*!<  input buffer data */
    uint32_t input_len;         /*!<  input buffer len  */
} des_ctx_t;

/** Triple-DES context structure */
typedef struct {
    int mode;                   /*!<  encrypt/decrypt   */
    unsigned char iv[8];        /*!<  iv                */
    uint32_t sk[96];            /*!<  3DES subkeys      */
    unsigned char input[8];     /*!<  input buffer data */
    uint32_t input_len;         /*!<  input buffer len  */
} des3_ctx_t;

#ifdef __cplusplus
extern "C" {
#endif

/** Set key parity on the given key to odd.
 *  DES keys are 56 bits long, but each byte is padded with
 *  a parity bit to allow verification.
 * @param key      8-byte secret key
 */
void des_key_set_parity(unsigned char key[DES_KEY_SIZE]);

/** Check that key parity on the given key is odd.
 *  DES keys are 56 bits long, but each byte is padded with
 *  a parity bit to allow verification.
 * @param key      8-byte secret key
 * @return         true is parity was ok, false if parity was not correct.
 */
bool des_key_check_key_parity(const unsigned char key[DES_KEY_SIZE]);

/** Check that key is not a weak or semi-weak DES key
 * @param key      8-byte secret key
 * @return         true if no weak key was found, false if a weak key was identified.
 */
bool des_key_check_weak(const unsigned char key[DES_KEY_SIZE]);

/** DES key schedule (56-bit, encryption)
 * @param pctx     DES context to be initialized
 * @param key      8-byte secret key
 */
void des_setkey_enc(des_ctx_t* pctx, const unsigned char key[DES_KEY_SIZE]);

/** DES key schedule (56-bit, decryption)
 * @param pctx     DES context to be initialized
 * @param key      8-byte secret key
 */
void des_setkey_dec(des_ctx_t* pctx, const unsigned char key[DES_KEY_SIZE]);

/** Triple-DES key schedule (112-bit, encryption)
 * @param pctx     3DES context to be initialized
 * @param key      16-byte secret key
 */
void des3_set2key_enc(des3_ctx_t* pctx, const unsigned char key[DES_KEY_SIZE * 2]);

/** Triple-DES key schedule (112-bit, decryption)
 * @param pctx     3DES context to be initialized
 * @param key      16-byte secret key
 */
void des3_set2key_dec(des3_ctx_t* pctx, const unsigned char key[DES_KEY_SIZE * 2]);

/** Triple-DES key schedule (168-bit, encryption)
 * @param pctx     3DES context to be initialized
 * @param key      24-byte secret key
 */
void des3_set3key_enc(des3_ctx_t* pctx, const unsigned char key[DES_KEY_SIZE * 3]);

/** Triple-DES key schedule (168-bit, decryption)
 * @param pctx     3DES context to be initialized
 * @param key      24-byte secret key
 */
void des3_set3key_dec(des3_ctx_t* pctx, const unsigned char key[DES_KEY_SIZE * 3]);

/** DES-ECB block encryption/decryption
 * @param pctx     DES context
 * @param input    64-bit input block
 * @param output   64-bit output block
 */
void des_crypt_ecb(uint32_t sk[32], const unsigned char input[8], unsigned char output[8]);

/** DES-CBC buffer encryption/decryption
 * @param pctx     DES context
 * @param length   length of the input data
 * @param iv       initialization vector (updated after use)
 * @param input    buffer holding the input data
 * @param output   buffer holding the output data
 */
bool des_crypt_cbc(int mode, unsigned char iv[8], uint32_t sk[32], size_t length,
        const unsigned char *input, unsigned char *output);

/** 3DES-ECB block encryption/decryption
 * @param pctx     3DES context
 * @param input    64-bit input block
 * @param output   64-bit output block
 */
void des3_crypt_ecb(uint32_t sk[96], const unsigned char input[8], unsigned char output[8]);

/** 3DES-CBC buffer encryption/decryption
 * @param pctx     3DES context
 * @param mode     DES_ENCRYPT or DES_DECRYPT
 * @param length   length of the input data
 * @param iv       initialization vector (updated after use)
 * @param input    buffer holding the input data
 * @param output   buffer holding the output data
 * @return         true if successful, or false is fail
 */
bool des3_crypt_cbc(int mode, unsigned char iv[8], uint32_t sk[96], size_t length,
    const unsigned char *input, unsigned char *output);

/** des 初始化, des-pkcs5-padding模式
 * @param pctx      des加解密上下文
 * @param mode      DES_ENCRYPT/DES_DECRYPT, 加密/解密模式
 * @param key       秘钥，必须为8字节
 * @param iv        加密向量，必须为8字节
*/
extern void des_init(des_ctx_t* pctx, int mode, unsigned char key[8], unsigned char iv[8]);

/** des 算法流式更新，用于分段加解密模式，自动缓存，输入流大于8字节后自动进行加密
 * @param pctx      des加解密上下文
 * @param input     输入流
 * @param input_len 输入流长度
 * @param output    输出流
 * @return size_t   返回输出流写入字节数量
*/
extern size_t des_update(des_ctx_t* pctx, const void* input, size_t input_len, void* output);

/** des 算法流式更新，用于分段加解密模式结束，对末尾进行pkcs5-padding模式对齐
 * @param pctx      des加解密上下文
 * @param input     输入流
 * @param input_len 输入流长度
 * @param output    输出流
 * @return size_t   返回输出流写入字节数量
*/
extern size_t des_final(des_ctx_t* pctx, const void* input, size_t input_len, void* output);

/** 3des 初始化, des-pkcs5-padding模式
 * @param pctx      3des加解密上下文
 * @param mode      DES_ENCRYPT/DES_DECRYPT, 加密/解密模式
 * @param key       秘钥，必须为8字节
 * @param iv        加密向量，必须为8字节
*/
extern void des3_init(des3_ctx_t* pctx, int mode, unsigned char key[24], unsigned char iv[8]);

/** 3des 算法流式更新，用于分段加解密模式，自动缓存，输入流大于8字节后自动进行加密
 * @param pctx      3des加解密上下文
 * @param input     输入流
 * @param input_len 输入流长度
 * @param output    输出流
 * @return size_t   返回输出流写入字节数量
*/
extern size_t des3_update(des3_ctx_t* pctx, const void* input, size_t input_len, void* output);

/** 3des 算法流式更新，用于分段加解密模式结束，对末尾进行pkcs5-padding模式对齐
 * @param pctx      3des加解密上下文
 * @param input     输入流
 * @param input_len 输入流长度
 * @param output    输出流
 * @return size_t   返回输出流写入字节数量
*/
extern size_t des3_final(des3_ctx_t* pctx, const void* input, size_t input_len, void* output);

#ifdef __cplusplus
}
#endif

#endif  // __DES_H__
