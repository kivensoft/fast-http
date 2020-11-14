#include <string.h>
#include <memory.h>
#include "hex.h"

/** 16进制转换常量 */
static const char HEX[] = "0123456789abcdef";

#define HEX_ENC(dst, ch) dst[0] = HEX[ch >> 4]; dst[1] = HEX[ch & 0xF];
// 快速拷贝宏，针对小于3个字节的字符串进行内联拷贝，避免函数调用开销
#define HEX_QUICK_COPY(dst, src, len)       \
    switch (len) {                          \
        case 3: dst[2] = src[2];            \
        case 2: dst[1] = src[1];            \
        case 1: dst[0] = src[0]; break;     \
        default: memcpy(dst, src, len);     \
    }

// 采用快速，但不是很严格的判断，加快解码速度
inline static uint8_t hex_decode_char_fast(uint8_t b1, uint8_t b2) {
    // 0-9
    if (b1 & 16) b1 &= 15;
    // A-F/a-f
    else b1 = 9 + (b1 & 7);
    if (b2 & 16) b2 &= 15;
    else b2 = 9 + (b2 & 7);
    return b1 << 4 | b2;
}

size_t hex_encode(char *dst, const void *src, size_t srclen, char *pre, char *spec) {
    if (!srclen) return 0;
    assert(src && dst);
    const uint8_t *s = (const uint8_t*)src, *s_max = s + srclen;
    uint8_t *d = (uint8_t*)dst;
    size_t prelen = pre ? strlen(pre) : 0;
    size_t speclen = spec ? strlen(spec) : 0;

    // 大部分时候没有分隔符，对其进行特殊处理，加快编码速度
    if (!prelen && !speclen) {
        for (; s < s_max; ++s, d += 2) {
            HEX_ENC(d, *s);
        }
    } else {
        if (prelen) {
            HEX_QUICK_COPY(d, pre, prelen);
            d += prelen;
        }
        HEX_ENC(d, *s);
        for (++s, d += 2; s < s_max; ++s, d += 2) {
            if (speclen) {
                HEX_QUICK_COPY(d, spec, speclen);
                d += speclen;
            }
            if (prelen) {
                HEX_QUICK_COPY(d, pre, prelen);
                d += prelen;
            }
            HEX_ENC(d, *s);
        }
    }

    return d - (uint8_t*)dst;
}

size_t hex_decode(void* dst, const char *src, size_t srclen, size_t prelen, size_t speclen) {
    if (srclen < 2) return 0;
    assert(dst && src);
    uint8_t *d = (uint8_t*)dst;
    const char *src_max = src + srclen - 1;
    uint8_t src_step = 2 + prelen + speclen;

    for (src += prelen; src < src_max; src += src_step)
        *d++ = hex_decode_char_fast(src[0], src[1]);

    return d - (uint8_t*)dst;
}

#ifdef TEST_HEX
#include <stdio.h>
int main() {
    const char* text = "abcd1234";
    char buf[256];
    if (hex_encode_len(0, 4, 3) != 0)
        printf("%s:%d -- test fail\n", __FILE__, __LINE__);
    if (hex_encode_len(3, 2, 1) != 14)
        printf("%s:%d -- test fail\n", __FILE__, __LINE__);

    if (hex_decode_len(0, 3, 4) != 0)
        printf("%s:%d -- test fail\n", __FILE__, __LINE__);
    if (hex_decode_len(16, 2, 2) != 3)
        printf("%s:%d -- test fail\n", __FILE__, __LINE__);

    char *v1 = "0x61, 0x62, 0x63, 0x64, 0x31, 0x32, 0x33, 0x34";
    size_t dlen = hex_encode(buf, text, 8, "0x", ", ");
    if (dlen != hex_encode_len(8, 2, 2) || dlen != strlen(v1))
        printf("%s:%d -- test fail\n", __FILE__, __LINE__);
    if (memcmp(buf, v1, dlen))
        printf("%s:%d -- test fail\n", __FILE__, __LINE__);

    char *v2 = "61,62,63,64,31,32,33,34";
    size_t vlen2 = hex_encode(buf, text, strlen(text), NULL, ",");
    if (vlen2 != hex_encode_len(8, 0, 1) || vlen2 != strlen(v2))
        printf("%s:%d -- test fail\n", __FILE__, __LINE__);
    if (memcmp(buf, v2, vlen2))
        printf("%s:%d -- test fail\n", __FILE__, __LINE__);
    
    hex_decode(buf, v2, strlen(v2), 0, 1);
    if (memcmp(text, buf, strlen(text)))
        printf("%s:%d -- test fail\n", __FILE__, __LINE__);
    hex_decode(buf, v1, strlen(v1), 2, 2);
    if (memcmp(text, buf, strlen(text)))
        printf("%s:%d -- test fail\n", __FILE__, __LINE__);

    printf("test complete!\n");
}
#endif