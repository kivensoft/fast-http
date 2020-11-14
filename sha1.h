/**
 * SHA-1 cryptographic hash function
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
/* Example:
    unsigned char *pwd = (unsigned char*) "password";
    unsigned char data[20];
    const unsigned char enc[] = {0x5b, 0xaa, 0x61, 0xe4, 0xc9, 0xb9, 0x3f, 0x3f,
            0x06, 0x82, 0x25, 0x0b, 0x6c, 0xf8, 0x33, 0x1b, 0x7e, 0xe6, 0x8f, 0xd8};
    sha1(pwd, strlen((char*) pwd), data);
    if (memcmp(data, enc, sizeof(data)))
        printf("sha1 succeed\n");
    else
        printf("sha1 fail\n");

    unsigned char enc2[] = {0xa4, 0x62, 0xb4, 0xd9, 0x10, 0x54, 0x4d, 0x3f,
            0xfb, 0x39, 0xf3, 0xa6, 0x40, 0x17, 0xf6, 0x5e, 0x02, 0x9b, 0x73, 0xfb};
    sha1_hmac("secret", 6, pwd, strlen((char*)pwd), data);
    if (memcmp(data, enc2, sizeof(data)))
        printf("sha1_hmac succeed\n");
    else
        printf("sha1_hmac fail\n");
 */
#pragma once
#ifndef __SHA1_H__
#define __SHA1_H__

#include <inttypes.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/** SHA-1 context structure */
typedef struct {
  uint32_t total[2];        /*!< number of bytes processed  */
  uint32_t state[5];        /*!< intermediate digest state  */
  unsigned char buffer[64]; /*!< data block being processed */

  unsigned char ipad[64];   /*!< HMAC: inner padding        */
  unsigned char opad[64];   /*!< HMAC: outer padding        */
} sha1_ctx_t;


/** SHA-1 context setup
 * @param pctx     context to be initialized
 */
void sha1_starts(sha1_ctx_t* pctx);

/** SHA-1 process buffer
 * @param pctx     SHA-1 context
 * @param input    buffer holding the  data
 * @param ilen     length of the input data
 */
void sha1_update(sha1_ctx_t* pctx, const void *input, size_t ilen);

/** SHA-1 final digest
 * @param pctx     SHA-1 context
 * @param output   SHA-1 checksum result
 */
void sha1_finish(sha1_ctx_t* pctx, uint8_t output[20]);

/** Output = SHA-1( input buffer )
 * @param input    buffer holding the  data
 * @param ilen     length of the input data
 * @param output   SHA-1 checksum result
 */
void sha1(const void *input, size_t ilen, uint8_t output[20]);

/** SHA-1 HMAC context setup
 * @param pctx     HMAC context to be initialized
 * @param key      HMAC secret key
 * @param keylen   length of the HMAC key
 */
void sha1_hmac_starts(sha1_ctx_t* pctx, const void *key, size_t keylen);

/** SHA-1 HMAC process buffer
 * @param pctx     HMAC context
 * @param input    buffer holding the  data
 * @param ilen     length of the input data
 */
void sha1_hmac_update(sha1_ctx_t* pctx, const void *input, size_t ilen);

/** SHA-1 HMAC final digest
 * @param pctx     HMAC context
 * @param output   SHA-1 HMAC checksum result
 */
void sha1_hmac_finish(sha1_ctx_t* pctx, uint8_t output[20]);

/** SHA-1 HMAC context reset
 * @param pctx     HMAC context to be reset
 */
void sha1_hmac_reset(sha1_ctx_t* pctx);

/** Output = HMAC-SHA-1( hmac key, input buffer )
 * @param key      HMAC secret key
 * @param keylen   length of the HMAC key
 * @param input    buffer holding the  data
 * @param ilen     length of the input data
 * @param output   HMAC-SHA-1 result
 */
void sha1_hmac(const void *key, size_t keylen, const void *input, size_t ilen, uint8_t output[20]);

#ifdef __cplusplus
}
#endif

#endif  // __SHA1_H__
