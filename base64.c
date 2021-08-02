/*
 *  RFC 1521 base64 encoding/decoding
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

#include <inttypes.h>
#include "base64.h"

#define PADDING_CHAR '='
#define LINE_BREAK_COUNT 76

static const uint8_t ENC_MAP[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const uint8_t URL_ENC_MAP[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

static const uint8_t DEC_MAP[128] = {
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127,  62, 127, 127, 127,  63,  52,  53,
     54,  55,  56,  57,  58,  59,  60,  61, 127, 127,
    127,   0, 127, 127, 127,   0,   1,   2,   3,   4,
      5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
     15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
     25, 127, 127, 127, 127, 127, 127,  26,  27,  28,
     29,  30,  31,  32,  33,  34,  35,  36,  37,  38,
     39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
     49,  50,  51, 127, 127, 127, 127, 127
};

static const uint8_t URL_DEC_MAP[128] = {
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127,  62, 127, 127,  52,  53,
     54,  55,  56,  57,  58,  59,  60,  61, 127, 127,
    127,   0, 127, 127, 127,   0,   1,   2,   3,   4,
      5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
     15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
     25, 127, 127, 127, 127,  63, 127,  26,  27,  28,
     29,  30,  31,  32,  33,  34,  35,  36,  37,  38,
     39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
     49,  50,  51, 127, 127, 127, 127, 127
};

size_t base64_encode_len(size_t srclen, bool padding, bool line_break) {
    size_t n = srclen / 3, m = srclen % 3, c = n << 2;

    if (m) {
        if (padding) c += 4;
        else {
            if (m == 1) c += 2;
            else c += 3;
        }
    }

    if (line_break) c += (c / LINE_BREAK_COUNT) << 1;

    return c;
}

size_t base64_decode_len(uint8_t *src, size_t srclen) {
    size_t n = srclen >> 2, m = srclen & 3, c = n * 3;

    if (srclen >= LINE_BREAK_COUNT + 2 && src[76] == '\r' && src[77] == '\n')
        c -= (c / LINE_BREAK_COUNT + 2) << 1;

    if (m) {
        if (m == 2) c += 1;
        else c += 2;
    } else {
        if (src[srclen - 1] == PADDING_CHAR) {
            c -= 1;
            if (src[srclen - 2] == PADDING_CHAR) c -= 1;
        }
    }

    return c;
}

size_t base64_encode_custom(uint8_t* dst, const void* src, size_t slen, bool padding, bool line_break, const uint8_t *map) {
    const uint8_t *s = (const uint8_t*)src, *src_max = (const uint8_t*)src + slen - 2;
    uint8_t *d = dst;

    // 每3个字节处理一次，将3字节转换成4字节，剩余不足三个字节的后续处理
    for (int line_count = 0; s < src_max; s += 3, d += 4) {
        d[0] = map[s[0] >> 2];
        d[1] = map[((s[0] & 3) << 4) | (s[1] >> 4)];
        d[2] = map[((s[1] & 15) << 2) | (s[2] >> 6)];
        d[3] = map[s[2] & 0x3F];

        // 如果有换行需要，处理换行，76字节换行一次，所以肯定是4字节的倍数
        if (line_break) {
            line_count += 4;
            if (line_count >= LINE_BREAK_COUNT) {
                d[4] = '\r';
                d[5] = '\n';
                d += 2;
                line_count = 0;
            }
        }
    }

    // 处理剩余不足3字节的内容，剩余字节肯定无需换行，所以无需考虑换行情况
    src_max += 2;
    if (s < src_max) {
        *d++ = map[s[0] >> 2];
        if (s + 1 < src_max) {
            *d++ = map[((s[0] & 3) << 4) + (s[1] >> 4)];
            *d++ = map[(s[1] & 15) << 2];
        } else {
            *d++ = map[(s[0] & 3) << 4];
            if (padding) *d++ = PADDING_CHAR;
        }
        if (padding) *d++ = PADDING_CHAR;
    }

    return d - dst;
}

size_t base64_encode(uint8_t* dst, const void* src, size_t slen, bool padding, bool line_break) {
    return base64_encode_custom(dst, src, slen , padding, line_break, ENC_MAP);
}

size_t base64_url_encode(uint8_t* dst, const void* src, size_t slen, bool padding, bool line_break) {
    return base64_encode_custom(dst, src, slen, padding, line_break, URL_ENC_MAP);
}

size_t base64_decode_custom(void* dst, const uint8_t* src, size_t slen, const uint8_t *map) {
    // 删除末尾的回车换行和=号
    while (slen--) {
        uint8_t c = src[slen];
        if (c != PADDING_CHAR && c != '\n' && c != '\r') break;
    }
    ++slen;

    uint8_t *d = (uint8_t*)dst, c0, c1, c2, c3;
    const uint8_t *src_max = src + slen - 3;

    // 每次解码4字节
    for (; src < src_max; src += 4, d += 3) {
        if (src[0] == '\r') src += 2;
        c0 = map[src[0]], c1 = map[src[1]], c2 = map[src[2]], c3 = map[src[3]];
        d[0] = (c0 << 2) | (c1 >> 4);
        d[1] = (c1 << 4) | (c2 >> 2);
        d[2] = (c2 << 6) | c3;
    }

    // 剩余不足4字节的内容解码
    src_max += 3;
    if (src + 1 < src_max) {
        c0 = map[src[0]], c1 = map[src[1]];
        *d++ = (c0 << 2) | (c1 >> 4);
        if (src + 2 < src_max)
            *d++ = (c1 << 4) | (map[src[2]] >> 2);
    }

    return d - (uint8_t*)dst;
}

size_t base64_decode(void* dst, const uint8_t* src, size_t slen) {
    return base64_decode_custom(dst, src, slen, DEC_MAP);
}

size_t base64_url_decode(void* dst, const uint8_t* src, size_t slen) {
    return base64_decode_custom(dst, src, slen, URL_DEC_MAP);
}


//---------------------------------------------------------------------
#ifdef TEST_BASE64
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <assert.h>

int main() {
    char s[] = "kiven1234567890abcde";
    char d[] = "a2l2ZW4xMjM0NTY3ODkwYWJjZGU=";
    int slen = strlen(s), dlen = strlen(d), ti;
    char senc[dlen + 1], denc[slen + 1];
    memset(senc, 'x', dlen + 1);
    memset(denc, 'x', slen + 1);


    if ((ti = base64_encode_len(slen, true, false)) != dlen) {
        printf("base64_encode_len error: expect: %d, actual: %d\n", dlen, ti);
        return 1;
    }
    if ((ti = base64_decode_len(d, dlen)) != slen) {
        printf("base64_decode_len error: expect: %d, actual: %d\n", slen, ti);
        return 1;
    }

    int selen = base64_encode(senc, s, slen, true, false);
    if (dlen != selen) {
        printf("base64_encode error: expect: %d, actual: %d\n", dlen, selen);
        return 1;
    }
    if (memcmp(d, senc, dlen)) {
        printf("base64_encode error: expect: %s, actual: %s\n", d, senc);
        return 1;
    }

    int delen = base64_decode(denc, d, dlen);
    if (slen != delen) {
        printf("base64_decode error: expect: %d, actual: %d\n", slen, delen);
        return 1;
    }
    if (memcmp(s, denc, slen)) {
        printf("base64_decode error: expect: %s, actual: %s\n", s, denc);
        return 1;
    }

    printf("test runing success.");
}

#endif
