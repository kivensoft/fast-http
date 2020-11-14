/*
 *  FIPS-46-3 compliant Triple-DES implementation
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
 *  DES, on which TDES is based, was originally designed by Horst Feistel
 *  at IBM in 1974, and was adopted as a standard by NIST (formerly NBS).
 *
 *  http://csrc.nist.gov/publications/fips/fips46-3/fips46-3.pdf
 */

#include "des.h"

/*
 * 32-bit integer manipulation macros (big endian)
 */
#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n,b,i)                            \
{                                                       \
    (n) = ( (uint32_t) (b)[(i)    ] << 24 )             \
        | ( (uint32_t) (b)[(i) + 1] << 16 )             \
        | ( (uint32_t) (b)[(i) + 2] <<  8 )             \
        | ( (uint32_t) (b)[(i) + 3]       );            \
}
#endif

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n,b,i)                            \
{                                                       \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
}
#endif

/*
 * Expanded DES S-boxes
 */
static const uint32_t SB1[64] = {
    0x01010400, 0x00000000, 0x00010000, 0x01010404, 0x01010004, 0x00010404, 0x00000004, 0x00010000,
    0x00000400, 0x01010400, 0x01010404, 0x00000400, 0x01000404, 0x01010004, 0x01000000, 0x00000004,
    0x00000404, 0x01000400, 0x01000400, 0x00010400, 0x00010400, 0x01010000, 0x01010000, 0x01000404,
    0x00010004, 0x01000004, 0x01000004, 0x00010004, 0x00000000, 0x00000404, 0x00010404, 0x01000000,
    0x00010000, 0x01010404, 0x00000004, 0x01010000, 0x01010400, 0x01000000, 0x01000000, 0x00000400,
    0x01010004, 0x00010000, 0x00010400, 0x01000004, 0x00000400, 0x00000004, 0x01000404, 0x00010404,
    0x01010404, 0x00010004, 0x01010000, 0x01000404, 0x01000004, 0x00000404, 0x00010404, 0x01010400,
    0x00000404, 0x01000400, 0x01000400, 0x00000000, 0x00010004, 0x00010400, 0x00000000, 0x01010004
};

static const uint32_t SB2[64] = {
    0x80108020, 0x80008000, 0x00008000, 0x00108020, 0x00100000, 0x00000020, 0x80100020, 0x80008020,
    0x80000020, 0x80108020, 0x80108000, 0x80000000, 0x80008000, 0x00100000, 0x00000020, 0x80100020,
    0x00108000, 0x00100020, 0x80008020, 0x00000000, 0x80000000, 0x00008000, 0x00108020, 0x80100000,
    0x00100020, 0x80000020, 0x00000000, 0x00108000, 0x00008020, 0x80108000, 0x80100000, 0x00008020,
    0x00000000, 0x00108020, 0x80100020, 0x00100000, 0x80008020, 0x80100000, 0x80108000, 0x00008000,
    0x80100000, 0x80008000, 0x00000020, 0x80108020, 0x00108020, 0x00000020, 0x00008000, 0x80000000,
    0x00008020, 0x80108000, 0x00100000, 0x80000020, 0x00100020, 0x80008020, 0x80000020, 0x00100020,
    0x00108000, 0x00000000, 0x80008000, 0x00008020, 0x80000000, 0x80100020, 0x80108020, 0x00108000
};

static const uint32_t SB3[64] = {
    0x00000208, 0x08020200, 0x00000000, 0x08020008, 0x08000200, 0x00000000, 0x00020208, 0x08000200,
    0x00020008, 0x08000008, 0x08000008, 0x00020000, 0x08020208, 0x00020008, 0x08020000, 0x00000208,
    0x08000000, 0x00000008, 0x08020200, 0x00000200, 0x00020200, 0x08020000, 0x08020008, 0x00020208,
    0x08000208, 0x00020200, 0x00020000, 0x08000208, 0x00000008, 0x08020208, 0x00000200, 0x08000000,
    0x08020200, 0x08000000, 0x00020008, 0x00000208, 0x00020000, 0x08020200, 0x08000200, 0x00000000,
    0x00000200, 0x00020008, 0x08020208, 0x08000200, 0x08000008, 0x00000200, 0x00000000, 0x08020008,
    0x08000208, 0x00020000, 0x08000000, 0x08020208, 0x00000008, 0x00020208, 0x00020200, 0x08000008,
    0x08020000, 0x08000208, 0x00000208, 0x08020000, 0x00020208, 0x00000008, 0x08020008, 0x00020200
};

static const uint32_t SB4[64] = {
    0x00802001, 0x00002081, 0x00002081, 0x00000080, 0x00802080, 0x00800081, 0x00800001, 0x00002001,
    0x00000000, 0x00802000, 0x00802000, 0x00802081, 0x00000081, 0x00000000, 0x00800080, 0x00800001,
    0x00000001, 0x00002000, 0x00800000, 0x00802001, 0x00000080, 0x00800000, 0x00002001, 0x00002080,
    0x00800081, 0x00000001, 0x00002080, 0x00800080, 0x00002000, 0x00802080, 0x00802081, 0x00000081,
    0x00800080, 0x00800001, 0x00802000, 0x00802081, 0x00000081, 0x00000000, 0x00000000, 0x00802000,
    0x00002080, 0x00800080, 0x00800081, 0x00000001, 0x00802001, 0x00002081, 0x00002081, 0x00000080,
    0x00802081, 0x00000081, 0x00000001, 0x00002000, 0x00800001, 0x00002001, 0x00802080, 0x00800081,
    0x00002001, 0x00002080, 0x00800000, 0x00802001, 0x00000080, 0x00800000, 0x00002000, 0x00802080
};

static const uint32_t SB5[64] = {
    0x00000100, 0x02080100, 0x02080000, 0x42000100, 0x00080000, 0x00000100, 0x40000000, 0x02080000,
    0x40080100, 0x00080000, 0x02000100, 0x40080100, 0x42000100, 0x42080000, 0x00080100, 0x40000000,
    0x02000000, 0x40080000, 0x40080000, 0x00000000, 0x40000100, 0x42080100, 0x42080100, 0x02000100,
    0x42080000, 0x40000100, 0x00000000, 0x42000000, 0x02080100, 0x02000000, 0x42000000, 0x00080100,
    0x00080000, 0x42000100, 0x00000100, 0x02000000, 0x40000000, 0x02080000, 0x42000100, 0x40080100,
    0x02000100, 0x40000000, 0x42080000, 0x02080100, 0x40080100, 0x00000100, 0x02000000, 0x42080000,
    0x42080100, 0x00080100, 0x42000000, 0x42080100, 0x02080000, 0x00000000, 0x40080000, 0x42000000,
    0x00080100, 0x02000100, 0x40000100, 0x00080000, 0x00000000, 0x40080000, 0x02080100, 0x40000100
};

static const uint32_t SB6[64] = {
    0x20000010, 0x20400000, 0x00004000, 0x20404010, 0x20400000, 0x00000010, 0x20404010, 0x00400000,
    0x20004000, 0x00404010, 0x00400000, 0x20000010, 0x00400010, 0x20004000, 0x20000000, 0x00004010,
    0x00000000, 0x00400010, 0x20004010, 0x00004000, 0x00404000, 0x20004010, 0x00000010, 0x20400010,
    0x20400010, 0x00000000, 0x00404010, 0x20404000, 0x00004010, 0x00404000, 0x20404000, 0x20000000,
    0x20004000, 0x00000010, 0x20400010, 0x00404000, 0x20404010, 0x00400000, 0x00004010, 0x20000010,
    0x00400000, 0x20004000, 0x20000000, 0x00004010, 0x20000010, 0x20404010, 0x00404000, 0x20400000,
    0x00404010, 0x20404000, 0x00000000, 0x20400010, 0x00000010, 0x00004000, 0x20400000, 0x00404010,
    0x00004000, 0x00400010, 0x20004010, 0x00000000, 0x20404000, 0x20000000, 0x00400010, 0x20004010
};

static const uint32_t SB7[64] = {
    0x00200000, 0x04200002, 0x04000802, 0x00000000, 0x00000800, 0x04000802, 0x00200802, 0x04200800,
    0x04200802, 0x00200000, 0x00000000, 0x04000002, 0x00000002, 0x04000000, 0x04200002, 0x00000802,
    0x04000800, 0x00200802, 0x00200002, 0x04000800, 0x04000002, 0x04200000, 0x04200800, 0x00200002,
    0x04200000, 0x00000800, 0x00000802, 0x04200802, 0x00200800, 0x00000002, 0x04000000, 0x00200800,
    0x04000000, 0x00200800, 0x00200000, 0x04000802, 0x04000802, 0x04200002, 0x04200002, 0x00000002,
    0x00200002, 0x04000000, 0x04000800, 0x00200000, 0x04200800, 0x00000802, 0x00200802, 0x04200800,
    0x00000802, 0x04000002, 0x04200802, 0x04200000, 0x00200800, 0x00000000, 0x00000002, 0x04200802,
    0x00000000, 0x00200802, 0x04200000, 0x00000800, 0x04000002, 0x04000800, 0x00000800, 0x00200002
};

static const uint32_t SB8[64] = {
    0x10001040, 0x00001000, 0x00040000, 0x10041040, 0x10000000, 0x10001040, 0x00000040, 0x10000000,
    0x00040040, 0x10040000, 0x10041040, 0x00041000, 0x10041000, 0x00041040, 0x00001000, 0x00000040,
    0x10040000, 0x10000040, 0x10001000, 0x00001040, 0x00041000, 0x00040040, 0x10040040, 0x10041000,
    0x00001040, 0x00000000, 0x00000000, 0x10040040, 0x10000040, 0x10001000, 0x00041040, 0x00040000,
    0x00041040, 0x00040000, 0x10041000, 0x00001000, 0x00000040, 0x10040040, 0x00001000, 0x00041040,
    0x10001000, 0x00000040, 0x10000040, 0x10040000, 0x10040040, 0x10000000, 0x00040000, 0x10001040,
    0x00000000, 0x10041040, 0x00040040, 0x10000040, 0x10040000, 0x10001000, 0x10001040, 0x00000000,
    0x10041040, 0x00041000, 0x00041000, 0x00001040, 0x00001040, 0x00040040, 0x10000000, 0x10041000
};

/*
 * PC1: left and right halves bit-swap
 */
static const uint32_t LHs[16] = {
    0x00000000, 0x00000001, 0x00000100, 0x00000101, 0x00010000, 0x00010001, 0x00010100, 0x00010101,
    0x01000000, 0x01000001, 0x01000100, 0x01000101, 0x01010000, 0x01010001, 0x01010100, 0x01010101
};

static const uint32_t RHs[16] = {
    0x00000000, 0x01000000, 0x00010000, 0x01010000, 0x00000100, 0x01000100, 0x00010100, 0x01010100,
    0x00000001, 0x01000001, 0x00010001, 0x01010001, 0x00000101, 0x01000101, 0x00010101, 0x01010101,
};

/*
 * Initial Permutation macro
 */
#define DES_IP(X,Y)                                             \
{                                                               \
    T = ((X >>  4) ^ Y) & 0x0F0F0F0F; Y ^= T; X ^= (T <<  4);   \
    T = ((X >> 16) ^ Y) & 0x0000FFFF; Y ^= T; X ^= (T << 16);   \
    T = ((Y >>  2) ^ X) & 0x33333333; X ^= T; Y ^= (T <<  2);   \
    T = ((Y >>  8) ^ X) & 0x00FF00FF; X ^= T; Y ^= (T <<  8);   \
    Y = ((Y << 1) | (Y >> 31)) & 0xFFFFFFFF;                    \
    T = (X ^ Y) & 0xAAAAAAAA; Y ^= T; X ^= T;                   \
    X = ((X << 1) | (X >> 31)) & 0xFFFFFFFF;                    \
}

/*
 * Final Permutation macro
 */
#define DES_FP(X,Y)                                             \
{                                                               \
    X = ((X << 31) | (X >> 1)) & 0xFFFFFFFF;                    \
    T = (X ^ Y) & 0xAAAAAAAA; X ^= T; Y ^= T;                   \
    Y = ((Y << 31) | (Y >> 1)) & 0xFFFFFFFF;                    \
    T = ((Y >>  8) ^ X) & 0x00FF00FF; X ^= T; Y ^= (T <<  8);   \
    T = ((Y >>  2) ^ X) & 0x33333333; X ^= T; Y ^= (T <<  2);   \
    T = ((X >> 16) ^ Y) & 0x0000FFFF; Y ^= T; X ^= (T << 16);   \
    T = ((X >>  4) ^ Y) & 0x0F0F0F0F; Y ^= T; X ^= (T <<  4);   \
}

/*
 * DES round macro
 */
#define DES_ROUND(X,Y)                          \
{                                               \
    T = *SK++ ^ X;                              \
    Y ^= SB8[ (T      ) & 0x3F ] ^              \
         SB6[ (T >>  8) & 0x3F ] ^              \
         SB4[ (T >> 16) & 0x3F ] ^              \
         SB2[ (T >> 24) & 0x3F ];               \
                                                \
    T = *SK++ ^ ((X << 28) | (X >> 4));         \
    Y ^= SB7[ (T      ) & 0x3F ] ^              \
         SB5[ (T >>  8) & 0x3F ] ^              \
         SB3[ (T >> 16) & 0x3F ] ^              \
         SB1[ (T >> 24) & 0x3F ];               \
}

#define SWAP(a,b) { uint32_t t = a; a = b; b = t; t = 0; }

static const unsigned char odd_parity_table[128] = {
        1,   2,   4,   7,   8,   11,  13,  14,  16,  19,  21,  22,  25,  26,  28,  31,
        32,  35,  37,  38,  41,  42,  44,  47,  49,  50,  52,  55,  56,  59,  61,  62,
        64,  67,  69,  70,  73,  74,  76,  79,  81,  82,  84,  87,  88,  91,  93,  94,
        97,  98,  100, 103, 104, 107, 109, 110, 112, 115, 117, 118, 121, 122, 124, 127,
        128, 131, 133, 134, 137, 138, 140, 143, 145, 146, 148, 151, 152, 155, 157, 158,
        161, 162, 164, 167, 168, 171, 173, 174, 176, 179, 181, 182, 185, 186, 188, 191,
        193, 194, 196, 199, 200, 203, 205, 206, 208, 211, 213, 214, 217, 218, 220, 223,
        224, 227, 229, 230, 233, 234, 236, 239, 241, 242, 244, 247, 248, 251, 253, 254 };

void des_key_set_parity(unsigned char key[DES_KEY_SIZE]) {
    for(int i = 0; i < DES_KEY_SIZE; i++ )
        key[i] = odd_parity_table[key[i] / 2];
}

/** Check the given key's parity, returns 1 on failure, 0 on SUCCESS */
bool des_key_check_key_parity(const unsigned char key[DES_KEY_SIZE]) {
    for(int i = 0; i < DES_KEY_SIZE; i++ )
        if ( key[i] != odd_parity_table[key[i] / 2] )
            return false;

    return true;
}

#define WEAK_KEY_COUNT 16

static const unsigned char weak_key_table[WEAK_KEY_COUNT][DES_KEY_SIZE] = {
    { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
    { 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE },
    { 0x1F, 0x1F, 0x1F, 0x1F, 0x0E, 0x0E, 0x0E, 0x0E },
    { 0xE0, 0xE0, 0xE0, 0xE0, 0xF1, 0xF1, 0xF1, 0xF1 },

    { 0x01, 0x1F, 0x01, 0x1F, 0x01, 0x0E, 0x01, 0x0E },
    { 0x1F, 0x01, 0x1F, 0x01, 0x0E, 0x01, 0x0E, 0x01 },
    { 0x01, 0xE0, 0x01, 0xE0, 0x01, 0xF1, 0x01, 0xF1 },
    { 0xE0, 0x01, 0xE0, 0x01, 0xF1, 0x01, 0xF1, 0x01 },
    { 0x01, 0xFE, 0x01, 0xFE, 0x01, 0xFE, 0x01, 0xFE },
    { 0xFE, 0x01, 0xFE, 0x01, 0xFE, 0x01, 0xFE, 0x01 },
    { 0x1F, 0xE0, 0x1F, 0xE0, 0x0E, 0xF1, 0x0E, 0xF1 },
    { 0xE0, 0x1F, 0xE0, 0x1F, 0xF1, 0x0E, 0xF1, 0x0E },
    { 0x1F, 0xFE, 0x1F, 0xFE, 0x0E, 0xFE, 0x0E, 0xFE },
    { 0xFE, 0x1F, 0xFE, 0x1F, 0xFE, 0x0E, 0xFE, 0x0E },
    { 0xE0, 0xFE, 0xE0, 0xFE, 0xF1, 0xFE, 0xF1, 0xFE },
    { 0xFE, 0xE0, 0xFE, 0xE0, 0xFE, 0xF1, 0xFE, 0xF1 }
};

bool des_key_check_weak(const unsigned char key[DES_KEY_SIZE]) {
    for(int i = 0; i < WEAK_KEY_COUNT; i++ )
        if( memcmp( weak_key_table[i], key, DES_KEY_SIZE) == 0)
            return false;

    return true;
}

static void des_setkey(uint32_t SK[32], const unsigned char key[DES_KEY_SIZE]) {
    uint32_t X, Y, T;

    GET_UINT32_BE( X, key, 0 );
    GET_UINT32_BE( Y, key, 4 );

    // Permuted Choice 1
    T =  ((Y >>  4) ^ X) & 0x0F0F0F0F;  X ^= T; Y ^= (T <<  4);
    T =  ((Y      ) ^ X) & 0x10101010;  X ^= T; Y ^= (T      );

    X =   (LHs[ (X      ) & 0xF] << 3) | (LHs[ (X >>  8) & 0xF ] << 2)
        | (LHs[ (X >> 16) & 0xF] << 1) | (LHs[ (X >> 24) & 0xF ]     )
        | (LHs[ (X >>  5) & 0xF] << 7) | (LHs[ (X >> 13) & 0xF ] << 6)
        | (LHs[ (X >> 21) & 0xF] << 5) | (LHs[ (X >> 29) & 0xF ] << 4);

    Y =   (RHs[ (Y >>  1) & 0xF] << 3) | (RHs[ (Y >>  9) & 0xF ] << 2)
        | (RHs[ (Y >> 17) & 0xF] << 1) | (RHs[ (Y >> 25) & 0xF ]     )
        | (RHs[ (Y >>  4) & 0xF] << 7) | (RHs[ (Y >> 12) & 0xF ] << 6)
        | (RHs[ (Y >> 20) & 0xF] << 5) | (RHs[ (Y >> 28) & 0xF ] << 4);

    X &= 0x0FFFFFFF;
    Y &= 0x0FFFFFFF;

    // calculate subkeys
    for (int i = 0; i < 16; i++) {
      if (i < 2 || i == 8 || i == 15) {
        X = ((X << 1) | (X >> 27)) & 0x0FFFFFFF;
        Y = ((Y << 1) | (Y >> 27)) & 0x0FFFFFFF;
      } else {
        X = ((X << 2) | (X >> 26)) & 0x0FFFFFFF;
        Y = ((Y << 2) | (Y >> 26)) & 0x0FFFFFFF;
      }

      *SK++ = ((X << 4) & 0x24000000) | ((X << 28) & 0x10000000) |
              ((X << 14) & 0x08000000) | ((X << 18) & 0x02080000) |
              ((X << 6) & 0x01000000) | ((X << 9) & 0x00200000) |
              ((X >> 1) & 0x00100000) | ((X << 10) & 0x00040000) |
              ((X << 2) & 0x00020000) | ((X >> 10) & 0x00010000) |
              ((Y >> 13) & 0x00002000) | ((Y >> 4) & 0x00001000) |
              ((Y << 6) & 0x00000800) | ((Y >> 1) & 0x00000400) |
              ((Y >> 14) & 0x00000200) | ((Y)&0x00000100) |
              ((Y >> 5) & 0x00000020) | ((Y >> 10) & 0x00000010) |
              ((Y >> 3) & 0x00000008) | ((Y >> 18) & 0x00000004) |
              ((Y >> 26) & 0x00000002) | ((Y >> 24) & 0x00000001);

      *SK++ = ((X << 15) & 0x20000000) | ((X << 17) & 0x10000000) |
              ((X << 10) & 0x08000000) | ((X << 22) & 0x04000000) |
              ((X >> 2) & 0x02000000) | ((X << 1) & 0x01000000) |
              ((X << 16) & 0x00200000) | ((X << 11) & 0x00100000) |
              ((X << 3) & 0x00080000) | ((X >> 6) & 0x00040000) |
              ((X << 15) & 0x00020000) | ((X >> 4) & 0x00010000) |
              ((Y >> 2) & 0x00002000) | ((Y << 8) & 0x00001000) |
              ((Y >> 14) & 0x00000808) | ((Y >> 9) & 0x00000400) |
              ((Y)&0x00000200) | ((Y << 7) & 0x00000100) |
              ((Y >> 7) & 0x00000020) | ((Y >> 3) & 0x00000011) |
              ((Y << 2) & 0x00000004) | ((Y >> 21) & 0x00000002);
    }
}

/** DES key schedule (56-bit, encryption) */
void des_setkey_enc(des_ctx_t* pctx, const unsigned char key[DES_KEY_SIZE]) {
    pctx->mode = DES_ENCRYPT;
    des_setkey(pctx->sk, key);
}

/** DES key schedule (56-bit, decryption) */
void des_setkey_dec(des_ctx_t* pctx, const unsigned char key[DES_KEY_SIZE]) {
    pctx->mode = DES_DECRYPT;
    des_setkey(pctx->sk, key);
    for(int i = 0; i < 16; i += 2) {
        SWAP(pctx->sk[i    ], pctx->sk[30 - i]);
        SWAP(pctx->sk[i + 1], pctx->sk[31 - i]);
    }
}

static void des3_set2key(uint32_t esk[96], uint32_t dsk[96], const unsigned char key[DES_KEY_SIZE*2]) {
    des_setkey(esk, key);
    des_setkey(dsk + 32, key + 8);

    for(int i = 0; i < 32; i += 2) {
        dsk[i     ] = esk[30 - i];
        dsk[i +  1] = esk[31 - i];

        esk[i + 32] = dsk[62 - i];
        esk[i + 33] = dsk[63 - i];

        esk[i + 64] = esk[i    ];
        esk[i + 65] = esk[i + 1];

        dsk[i + 64] = dsk[i    ];
        dsk[i + 65] = dsk[i + 1];
    }
}

/** Triple-DES key schedule (112-bit, encryption) */
void des3_set2key_enc(des3_ctx_t* pctx, const unsigned char key[DES_KEY_SIZE * 2]) {
    uint32_t sk[96];
    pctx->mode = DES_ENCRYPT;
    des3_set2key(pctx->sk, sk, key);
    memset(sk, 0, sizeof(sk));
}

/** Triple-DES key schedule (112-bit, decryption) */
void des3_set2key_dec(des3_ctx_t* pctx, const unsigned char key[DES_KEY_SIZE * 2]) {
    uint32_t sk[96];
    pctx->mode = DES_DECRYPT;
    des3_set2key(sk, pctx->sk, key);
    memset(sk, 0, sizeof(sk));
}

static void des3_set3key(uint32_t esk[96], uint32_t dsk[96], const unsigned char key[24]) {
    des_setkey(esk, key);
    des_setkey(dsk + 32, key + 8);
    des_setkey(esk + 64, key + 16);

    for (int i = 0; i < 32; i += 2) {
        dsk[i] = esk[94 - i];
        dsk[i + 1] = esk[95 - i];

        esk[i + 32] = dsk[62 - i];
        esk[i + 33] = dsk[63 - i];

        dsk[i + 64] = esk[30 - i];
        dsk[i + 65] = esk[31 - i];
    }
}

/** Triple-DES key schedule (168-bit, encryption) */
void des3_set3key_enc(des3_ctx_t* pctx, const unsigned char key[DES_KEY_SIZE * 3]) {
    uint32_t sk[96];
    pctx->mode = DES_ENCRYPT;
    des3_set3key(pctx->sk, sk, key);
    memset(sk, 0, sizeof(sk));
}

/** Triple-DES key schedule (168-bit, decryption) */
void des3_set3key_dec(des3_ctx_t* pctx, const unsigned char key[DES_KEY_SIZE * 3]) {
    uint32_t sk[96];
    pctx->mode = DES_DECRYPT;
    des3_set3key(sk, pctx->sk, key);
    memset(sk, 0, sizeof(sk));
}

/** DES-ECB block encryption/decryption */
void des_crypt_ecb(uint32_t sk[32], const unsigned char input[8], unsigned char output[8]) {
    uint32_t X, Y, T, *SK = sk;

    GET_UINT32_BE(X, input, 0);
    GET_UINT32_BE(Y, input, 4);

    DES_IP(X, Y);

    for(int i = 0; i < 8; i++) {
        DES_ROUND(Y, X);
        DES_ROUND(X, Y);
    }

    DES_FP(Y, X);

    PUT_UINT32_BE(Y, output, 0);
    PUT_UINT32_BE(X, output, 4);
}

/** DES-CBC buffer encryption/decryption */
bool des_crypt_cbc(int mode, unsigned char iv[8], uint32_t sk[32], size_t length,
        const unsigned char *input, unsigned char *output) {
    if (length % 8) return false;

    if(mode == DES_ENCRYPT) {
        while(length > 0) {
            for(int i = 0; i < 8; i++)
                output[i] = (unsigned char)(input[i] ^ iv[i]);

            des_crypt_ecb(sk, output, output);
            memcpy(iv, output, 8);

            input  += 8;
            output += 8;
            length -= 8;
        }
    }
    else  { /* DES_DECRYPT */
        unsigned char temp[8];
        while(length > 0) {
            memcpy(temp, input, 8);
            des_crypt_ecb(sk, input, output);

            for(int i = 0; i < 8; i++)
                output[i] = (unsigned char)(output[i] ^ iv[i]);

            memcpy(iv, temp, 8);

            input  += 8;
            output += 8;
            length -= 8;
        }
    }

    return true;
}

/** 3DES-ECB block encryption/decryption */
void des3_crypt_ecb(uint32_t sk[96], const unsigned char input[8], unsigned char output[8]) {
    int i;
    uint32_t X, Y, T, *SK = sk;

    GET_UINT32_BE( X, input, 0 );
    GET_UINT32_BE( Y, input, 4 );

    DES_IP( X, Y );

    for(i = 0; i < 8; i++ ) {
        DES_ROUND( Y, X );
        DES_ROUND( X, Y );
    }

    for(i = 0; i < 8; i++ ) {
        DES_ROUND( X, Y );
        DES_ROUND( Y, X );
    }

    for(i = 0; i < 8; i++ ) {
        DES_ROUND( Y, X );
        DES_ROUND( X, Y );
    }

    DES_FP( Y, X );

    PUT_UINT32_BE( Y, output, 0 );
    PUT_UINT32_BE( X, output, 4 );
}

/** 3DES-CBC buffer encryption/decryption */
bool des3_crypt_cbc(int mode, unsigned char iv[8], uint32_t sk[96], size_t length,
        const unsigned char *input, unsigned char *output) {
    if(length % 8) return false;

    if(mode == DES_ENCRYPT) {
        while(length > 0) {
            for(int i = 0; i < 8; i++)
                output[i] = (unsigned char)(input[i] ^ iv[i]);

            des3_crypt_ecb(sk, output, output);
            memcpy( iv, output, 8 );

            input  += 8;
            output += 8;
            length -= 8;
        }
    }
    else { /* DES_DECRYPT */
        unsigned char temp[8];
        while(length > 0) {
            memcpy(temp, input, 8);
            des3_crypt_ecb(sk, input, output);

            for(int i = 0; i < 8; i++)
                output[i] = (unsigned char)(output[i] ^ iv[i]);

            memcpy(iv, temp, 8);

            input  += 8;
            output += 8;
            length -= 8;
        }
    }

    return true;
}

void des_init(des_ctx_t* pctx, int mode, unsigned char key[8], unsigned char iv[8]) {
    pctx->mode = mode;
    pctx->input_len = 0;
    memcpy(pctx->iv, iv, 8);

    des_setkey(pctx->sk, key);
    if (mode == DES_DECRYPT) {
        for(int i = 0; i < 16; i += 2) {
            SWAP(pctx->sk[i    ], pctx->sk[30 - i]);
            SWAP(pctx->sk[i + 1], pctx->sk[31 - i]);
        }
    }
}

size_t des_update(des_ctx_t* pctx, const void* input, size_t input_len, void* output) {
    uint32_t pl = pctx->input_len;
    uint8_t *ip = (uint8_t*) input, *op = (uint8_t*) output;

    if (pl) { // 缓存有内容，优先处理缓存
        bool is_full = pl + input_len >= 8; // 本次加密内容加上原有的缓存是否能够满足至少8个字节长度
        uint32_t copy_len = is_full ? 8 - pl : input_len;
        memcpy(pctx->input + pl, ip, copy_len); // 复制到缓存
        if (!is_full) { // 缓存未满8字节，无需加密
            pctx->input_len += copy_len;
            return 0;
        }
        des_crypt_cbc(pctx->mode, pctx->iv, pctx->sk, 8, pctx->input, op);
        input_len -= copy_len;
        ip += copy_len;
        pctx->input_len = 0;
        op += 8;
    }

    uint32_t block_size = (input_len >> 3) << 3;
    if (block_size) {
        des_crypt_cbc(pctx->mode, pctx->iv, pctx->sk, block_size, ip, op);
        ip += block_size;
        op += block_size;
        input_len -= block_size;
    }

    if (input_len) {
        memcpy(pctx->input, ip, input_len);
        pctx->input_len = input_len;
    }

    return op - (uint8_t*) output;
}

size_t des_final(des_ctx_t* pctx, const void* input, size_t input_len, void* output) {
    uint8_t* op = (uint8_t*) output;
    op += des_update(pctx, input, input_len, op);
    if (pctx->mode == DES_ENCRYPT) {
        pkcs5_padding(pctx->input, pctx->input_len);
        des_crypt_cbc(pctx->mode, pctx->iv, pctx->sk, 8, pctx->input, op);
        op += 8;
    } else {
        op -= *(op - 1);
    }
    return op - (uint8_t*) output;
}

void des3_init(des3_ctx_t* pctx, int mode, unsigned char key[24], unsigned char iv[8]) {
    pctx->mode = mode;
    pctx->input_len = 0;
    memcpy(pctx->iv, iv, 8);

    uint32_t sk[96];
    if (mode == DES_ENCRYPT)
        des3_set3key(pctx->sk, sk, key);
    else // DES_DECRYPT
        des3_set3key(sk, pctx->sk, key);
    memset(sk, 0, sizeof(sk));
}

size_t des3_update(des3_ctx_t* pctx, const void* input, size_t input_len, void* output) {
    uint32_t pl = pctx->input_len;
    uint8_t *ip = (uint8_t*) input, *op = (uint8_t*) output;

    if (pl) { // 缓存有内容，优先处理缓存
        bool is_full = pl + input_len >= 8; // 本次加密内容加上原有的缓存是否能够满足至少8个字节长度
        uint32_t copy_len = is_full ? 8 - pl : input_len;
        memcpy(pctx->input + pl, ip, copy_len); // 复制到缓存
        if (!is_full) { // 缓存未满8字节，无需加密
            pctx->input_len += copy_len;
            return 0;
        }

        des3_crypt_cbc(pctx->mode, pctx->iv, pctx->sk, 8, pctx->input, op);
        input_len -= copy_len;
        ip += copy_len;
        pctx->input_len = 0;
        op += 8;
    }

    uint32_t block_size = (input_len >> 3) << 3;
    if (block_size) {
        des3_crypt_cbc(pctx->mode, pctx->iv, pctx->sk, block_size, ip, op);
        ip += block_size;
        op += block_size;
        input_len -= block_size;
    }

    if (input_len) {
        memcpy(pctx->input, ip, input_len);
        pctx->input_len = input_len;
    }

    return op - (uint8_t*) output;
}

size_t des3_final(des3_ctx_t* pctx, const void* input, size_t input_len, void* output) {
    uint8_t* op = (uint8_t*) output;
    op += des3_update(pctx, input, input_len, op);
    pkcs5_padding(pctx->input, pctx->input_len);
    op += des3_crypt_cbc(pctx->mode, pctx->iv, pctx->sk, 8, pctx->input, op);
    return op - (uint8_t*) output;
}


//---------------------------------------------------------------------------
#ifdef TEST_DES
#include <stdio.h>

int main() {
    char *pwd = "password";
    unsigned char iv[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    char *text = "12345678abcdefgh";
    unsigned char enc_data[] = {
            0x51, 0x49, 0x93, 0xae, 0x2a, 0xa4, 0x8a, 0x57,
            0x64, 0xe1, 0x08, 0x21, 0x39, 0xec, 0xdf, 0x1a,
            0xef, 0x46, 0xd1, 0xb0, 0x33, 0xe7, 0x62, 0xc2};
    unsigned char enc_data2[] = {
            0x51, 0x49, 0x93, 0xae, 0x2a, 0xa4, 0x8a, 0x57,
            0x28, 0x06, 0x9d, 0x49, 0x49, 0x6f, 0x68, 0x1b};
    size_t len = PKCS5_PADDING_SIZE(strlen(text));
    unsigned char data[len], *pdata = data;
    memset(data, 0, len);

    des_ctx_t ctx;
    des_init(&ctx, DES_ENCRYPT, (unsigned char*) pwd, iv);
    pdata += des_update(&ctx, text, 7, pdata);
    des_final(&ctx, text + 7, strlen(text) - 7, pdata);
    if (memcmp(enc_data, data, len)) {
        printf("des error!\n");
        for (int i = 0; i < len; ++i)
            printf("%02x", enc_data[i]);
        printf("\n");
        for (int i = 0; i < len; ++i)
            printf("%02x", data[i]);
        printf("\n");
    } else
        printf("des succeed!\n");

    des_init(&ctx, DES_DECRYPT, (unsigned char*)pwd, iv);
    int dlen = des_final(&ctx, data, len, data);
    if (dlen != strlen(text) || memcmp(data, text, dlen)) {
        printf("des dec error!\n");
        for (int i = 0; i < dlen; ++i)
            printf("%c", data[i]);
        printf("\n");
    } else
        printf("des succeed!\n");
    
    pdata = data;
    len = PKCS5_PADDING_SIZE(10);
    des_init(&ctx, DES_ENCRYPT, (unsigned char*) pwd, iv);
    pdata += des_update(&ctx, text, 7, pdata);
    des_final(&ctx, text + 7, 3, pdata);
    if (memcmp(enc_data2, data, len)) {
        printf("des error!\n");
        for (int i = 0; i < len; ++i)
            printf("%02x", enc_data[i]);
        printf("\n");
        for (int i = 0; i < len; ++i)
            printf("%02x", data[i]);
        printf("\n");
    } else
        printf("des succeed!\n");

    unsigned char pwd3[] = "password12345678abcdefgh";
    unsigned char enc_data3[24] = {
        0xda, 0x78, 0x25, 0x32, 0xc6, 0x6e, 0xfd, 0x2c,
        0xf2, 0x78, 0xed, 0x08, 0xb0, 0x1e, 0xa7, 0xe1,
        0x69, 0x35, 0x61, 0x9b, 0xa5, 0xa5, 0xad, 0xb8};
    len = PKCS5_PADDING_SIZE(strlen(text));

    des3_ctx_t ctx3;
    des3_init(&ctx3, DES_ENCRYPT, pwd3, iv);
    pdata += des3_update(&ctx3, text, 7, pdata);
    des3_final(&ctx3, text + 7, strlen(text) - 7, pdata);
    if (memcmp(enc_data3, data, len)) {
        printf("des error!\n");
        for (int i = 0; i < len; ++i)
            printf("%02x", enc_data3[i]);
        printf("\n");
        for (int i = 0; i < len; ++i)
            printf("%02x", data[i]);
        printf("\n");
    } else
        printf("des succeed!\n");

}
#endif