/*
 *  FIPS-180-1 compliant SHA-1 implementation
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
 *  The SHA-1 standard was published by NIST in 1993.
 *
 *  http://www.itl.nist.gov/fipspubs/fip180-1.htm
 */

#include "sha1.h"

/** 32-bit integer manipulation macros (big endian) */
#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n, b, i)                                                \
	{                                                                         \
		(n) = ((uint32_t)(b)[(i)] << 24) | ((uint32_t)(b)[(i) + 1] << 16) |   \
				((uint32_t)(b)[(i) + 2] << 8) | ((uint32_t)(b)[(i) + 3]);     \
	}
#endif

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n, b, i)                     \
	{                                              \
		(b)[(i)] = (unsigned char)((n) >> 24);     \
		(b)[(i) + 1] = (unsigned char)((n) >> 16); \
		(b)[(i) + 2] = (unsigned char)((n) >> 8);  \
		(b)[(i) + 3] = (unsigned char)((n));       \
	}
#endif

/** SHA-1 context setup */
void sha1_starts(sha1_ctx_t* pctx) {
	pctx->total[0] = 0;
	pctx->total[1] = 0;

	pctx->state[0] = 0x67452301;
	pctx->state[1] = 0xEFCDAB89;
	pctx->state[2] = 0x98BADCFE;
	pctx->state[3] = 0x10325476;
	pctx->state[4] = 0xC3D2E1F0;
}

static void sha1_process(sha1_ctx_t* pctx, const unsigned char data[64]) {
	uint32_t temp, W[16], A, B, C, D, E;

	GET_UINT32_BE(W[0], data, 0);
	GET_UINT32_BE(W[1], data, 4);
	GET_UINT32_BE(W[2], data, 8);
	GET_UINT32_BE(W[3], data, 12);
	GET_UINT32_BE(W[4], data, 16);
	GET_UINT32_BE(W[5], data, 20);
	GET_UINT32_BE(W[6], data, 24);
	GET_UINT32_BE(W[7], data, 28);
	GET_UINT32_BE(W[8], data, 32);
	GET_UINT32_BE(W[9], data, 36);
	GET_UINT32_BE(W[10], data, 40);
	GET_UINT32_BE(W[11], data, 44);
	GET_UINT32_BE(W[12], data, 48);
	GET_UINT32_BE(W[13], data, 52);
	GET_UINT32_BE(W[14], data, 56);
	GET_UINT32_BE(W[15], data, 60);

#define S(x, n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))
#define R(t) (temp = W[(t - 3) & 0x0F] ^ W[(t - 8) & 0x0F] ^ W[(t - 14) & 0x0F] ^ W[t & 0x0F], (W[t & 0x0F] = S(temp, 1)))
#define P(a, b, c, d, e, x) { e += S(a, 5) + F(b, c, d) + K + x; b = S(b, 30); }

	A = pctx->state[0];
	B = pctx->state[1];
	C = pctx->state[2];
	D = pctx->state[3];
	E = pctx->state[4];

#define F(x, y, z) (z ^ (x & (y ^ z)))
#define K 0x5A827999

	P(A, B, C, D, E, W[0]);
	P(E, A, B, C, D, W[1]);
	P(D, E, A, B, C, W[2]);
	P(C, D, E, A, B, W[3]);
	P(B, C, D, E, A, W[4]);
	P(A, B, C, D, E, W[5]);
	P(E, A, B, C, D, W[6]);
	P(D, E, A, B, C, W[7]);
	P(C, D, E, A, B, W[8]);
	P(B, C, D, E, A, W[9]);
	P(A, B, C, D, E, W[10]);
	P(E, A, B, C, D, W[11]);
	P(D, E, A, B, C, W[12]);
	P(C, D, E, A, B, W[13]);
	P(B, C, D, E, A, W[14]);
	P(A, B, C, D, E, W[15]);
	P(E, A, B, C, D, R(16));
	P(D, E, A, B, C, R(17));
	P(C, D, E, A, B, R(18));
	P(B, C, D, E, A, R(19));

#undef K
#undef F

#define F(x, y, z) (x ^ y ^ z)
#define K 0x6ED9EBA1

	P(A, B, C, D, E, R(20));
	P(E, A, B, C, D, R(21));
	P(D, E, A, B, C, R(22));
	P(C, D, E, A, B, R(23));
	P(B, C, D, E, A, R(24));
	P(A, B, C, D, E, R(25));
	P(E, A, B, C, D, R(26));
	P(D, E, A, B, C, R(27));
	P(C, D, E, A, B, R(28));
	P(B, C, D, E, A, R(29));
	P(A, B, C, D, E, R(30));
	P(E, A, B, C, D, R(31));
	P(D, E, A, B, C, R(32));
	P(C, D, E, A, B, R(33));
	P(B, C, D, E, A, R(34));
	P(A, B, C, D, E, R(35));
	P(E, A, B, C, D, R(36));
	P(D, E, A, B, C, R(37));
	P(C, D, E, A, B, R(38));
	P(B, C, D, E, A, R(39));

#undef K
#undef F

#define F(x, y, z) ((x & y) | (z & (x | y)))
#define K 0x8F1BBCDC

	P(A, B, C, D, E, R(40));
	P(E, A, B, C, D, R(41));
	P(D, E, A, B, C, R(42));
	P(C, D, E, A, B, R(43));
	P(B, C, D, E, A, R(44));
	P(A, B, C, D, E, R(45));
	P(E, A, B, C, D, R(46));
	P(D, E, A, B, C, R(47));
	P(C, D, E, A, B, R(48));
	P(B, C, D, E, A, R(49));
	P(A, B, C, D, E, R(50));
	P(E, A, B, C, D, R(51));
	P(D, E, A, B, C, R(52));
	P(C, D, E, A, B, R(53));
	P(B, C, D, E, A, R(54));
	P(A, B, C, D, E, R(55));
	P(E, A, B, C, D, R(56));
	P(D, E, A, B, C, R(57));
	P(C, D, E, A, B, R(58));
	P(B, C, D, E, A, R(59));

#undef K
#undef F

#define F(x, y, z) (x ^ y ^ z)
#define K 0xCA62C1D6

	P(A, B, C, D, E, R(60));
	P(E, A, B, C, D, R(61));
	P(D, E, A, B, C, R(62));
	P(C, D, E, A, B, R(63));
	P(B, C, D, E, A, R(64));
	P(A, B, C, D, E, R(65));
	P(E, A, B, C, D, R(66));
	P(D, E, A, B, C, R(67));
	P(C, D, E, A, B, R(68));
	P(B, C, D, E, A, R(69));
	P(A, B, C, D, E, R(70));
	P(E, A, B, C, D, R(71));
	P(D, E, A, B, C, R(72));
	P(C, D, E, A, B, R(73));
	P(B, C, D, E, A, R(74));
	P(A, B, C, D, E, R(75));
	P(E, A, B, C, D, R(76));
	P(D, E, A, B, C, R(77));
	P(C, D, E, A, B, R(78));
	P(B, C, D, E, A, R(79));

#undef K
#undef F

	pctx->state[0] += A;
	pctx->state[1] += B;
	pctx->state[2] += C;
	pctx->state[3] += D;
	pctx->state[4] += E;
}

/** SHA-1 process buffer */
void sha1_update(sha1_ctx_t* pctx, const void *input, size_t ilen) {
	if (ilen <= 0) return;

	uint32_t left = pctx->total[0] & 0x3F;
	size_t fill = 64 - left;

	pctx->total[0] += (uint32_t)ilen;
	pctx->total[0] &= 0xFFFFFFFF;

	if (pctx->total[0] < (uint32_t)ilen) pctx->total[1]++;

	if (left && ilen >= fill) {
		memcpy(pctx->buffer + left, input, fill);
		sha1_process(pctx, pctx->buffer);
		input += fill;
		ilen -= fill;
		left = 0;
	}

	while (ilen >= 64) {
		sha1_process(pctx, input);
		input += 64;
		ilen -= 64;
	}

	if (ilen > 0) memcpy(pctx->buffer + left, input, ilen);
}

static const unsigned char sha1_padding[64] = {
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/** SHA-1 final digest */
void sha1_finish(sha1_ctx_t* pctx, unsigned char output[20]) {
	uint32_t last, padn;
	uint32_t high, low;
	unsigned char msglen[8];

	high = (pctx->total[0] >> 29) | (pctx->total[1] << 3);
	low = (pctx->total[0] << 3);

	PUT_UINT32_BE(high, msglen, 0);
	PUT_UINT32_BE(low, msglen, 4);

	last = pctx->total[0] & 0x3F;
	padn = (last < 56) ? (56 - last) : (120 - last);

	sha1_update(pctx, sha1_padding, padn);
	sha1_update(pctx, msglen, 8);

	PUT_UINT32_BE(pctx->state[0], output, 0);
	PUT_UINT32_BE(pctx->state[1], output, 4);
	PUT_UINT32_BE(pctx->state[2], output, 8);
	PUT_UINT32_BE(pctx->state[3], output, 12);
	PUT_UINT32_BE(pctx->state[4], output, 16);
}

/** output = SHA-1( input buffer ) */
void sha1(const void *input, size_t ilen, uint8_t output[20]) {
	sha1_ctx_t ctx;
	sha1_starts(&ctx);
	sha1_update(&ctx, input, ilen);
	sha1_finish(&ctx, output);
	memset(&ctx, 0, sizeof(sha1_ctx_t));
}

/** SHA-1 HMAC context setup */
void sha1_hmac_starts(sha1_ctx_t* pctx, const void *key, size_t keylen) {
	uint8_t sum[20];

	if (keylen > 64) {
		sha1(key, keylen, sum);
		keylen = 20;
		key = sum;
	}

	memset(pctx->ipad, 0x36, 64);
	memset(pctx->opad, 0x5C, 64);

	for (int i = 0; i < keylen; i++) {
		pctx->ipad[i] = (uint8_t)(pctx->ipad[i] ^ ((const uint8_t*)key)[i]);
		pctx->opad[i] = (uint8_t)(pctx->opad[i] ^ ((const uint8_t*)key)[i]);
	}

	sha1_starts(pctx);
	sha1_update(pctx, pctx->ipad, 64);

	memset(sum, 0, sizeof(sum));
}

/** SHA-1 HMAC process buffer */
void sha1_hmac_update(sha1_ctx_t* pctx, const void *input, size_t ilen) {
	sha1_update(pctx, input, ilen);
}

/** SHA-1 HMAC final digest */
void sha1_hmac_finish(sha1_ctx_t* pctx, uint8_t output[20]) {
	uint8_t tmpbuf[20];

	sha1_finish(pctx, tmpbuf);
	sha1_starts(pctx);
	sha1_update(pctx, pctx->opad, 64);
	sha1_update(pctx, tmpbuf, 20);
	sha1_finish(pctx, output);

	memset(tmpbuf, 0, sizeof(tmpbuf));
}

/** SHA1 HMAC context reset */
void sha1_hmac_reset(sha1_ctx_t* pctx) {
	sha1_starts(pctx);
	sha1_update(pctx, pctx->ipad, 64);
}

/** output = HMAC-SHA-1( hmac key, input buffer ) */
void sha1_hmac(const void *key, size_t keylen, const void *input, size_t ilen, uint8_t output[20]) {
	sha1_ctx_t ctx;

	sha1_hmac_starts(&ctx, key, keylen);
	sha1_hmac_update(&ctx, input, ilen);
	sha1_hmac_finish(&ctx, output);

	memset(&ctx, 0, sizeof(sha1_ctx_t));
}

#ifdef SHA1_TEST
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char** argv) {
    unsigned char *pwd = (unsigned char*) "password";
    unsigned char data[20];
    const unsigned char enc[] = {0x5b, 0xaa, 0x61, 0xe4, 0xc9, 0xb9, 0x3f, 0x3f, 0x06, 0x82, 0x25, 0x0b, 0x6c, 0xf8, 0x33, 0x1b, 0x7e, 0xe6, 0x8f, 0xd8};
    sha1(pwd, strlen((char*) pwd), data);
    log_hex(LOG_DEBUG, "sha1", data, sizeof(data));
    if (memcmp(data, enc, sizeof(data)))
        log_debug("sha1 fail");
    else
        log_debug("sha1 succeed");

    unsigned char enc2[] = {0xa4, 0x62, 0xb4, 0xd9, 0x10, 0x54, 0x4d, 0x3f, 0xfb, 0x39, 0xf3, 0xa6, 0x40, 0x17, 0xf6, 0x5e, 0x02, 0x9b, 0x73, 0xfb};
    sha1_hmac("secret", 6, pwd, strlen((char*)pwd), data);
    log_hex(LOG_DEBUG, "sha1mac", data, sizeof(data));
    if (memcmp(data, enc2, sizeof(data)))
        log_debug("sha1-hmac fail");
    else
        log_debug("sha1-hmac succeed");
}
#endif