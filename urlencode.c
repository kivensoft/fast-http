#include "urlencode.h"

const static char HEX[] = "0123456789ABCDEF";

/** URL编码算法中需要编码的位图数组，从空格（32）开始，127结束，标志为1表示需要编码 */
const static uint8_t URL_MAP[] = { 37, 0, 0, 80, 0, 0, 0, 120, 1, 0, 0, 184 };

/** URL Component编码算法中需要编码的位图数组，从空格（32）开始，127结束，标志为1表示需要编码 */
const static uint8_t URL_COMPONENT_MAP[] = { 125, 152, 0, 252, 1, 0, 0, 120, 1, 0, 0, 184 };

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

// 判断字符是否需要编码
inline static _Bool required_encode(unsigned char ch, const uint8_t* map) {
    if (ch < 32 || ch > 127) return 1;
    // 查找ch在map位图数组中对应的标志位，标志为1表示需要编码, 0表示不需要
    unsigned idx = ch - 32;
    return (map[idx >> 3] >> idx & 7) & 1;
}

// 计算编码需要的长度
inline static size_t encode_length(const char *src, size_t src_len, const uint8_t* map) {
    if (!src || !src_len)
        return 0;

    size_t count = 0;
    const char* src_end = src + src_len;
    while (src < src_end) {
        if (required_encode((unsigned char)(*src++), map))
            count += 3;
        else
            ++count;
    }

    return count;
}

// 计算解码需要的长度
size_t url_encode_length(const char *src, size_t src_len) {
    return encode_length(src, src_len, URL_MAP);
}

// 计算url参数解码需要的长度
size_t url_component_encode_length(const char *src, size_t src_len) {
    return encode_length(src, src_len, URL_COMPONENT_MAP);
}

// url编码
static size_t encode(const char *src, size_t src_len, char *dst, size_t dst_len, const uint8_t* map) {
    if (!src || !src_len || !dst || !dst_len)
        return 0;

    char *p = dst, *pmax = dst + dst_len;
    for (size_t i = 0; i < src_len; ++i) {
        unsigned char cc = src[i];
        if (required_encode(cc, map)) {
            if (p + 3 > pmax)
                goto _ret;
            p[0] = '%';
            p[1] = HEX[cc >> 4];
            p[2] = HEX[cc & 0xF];
            p += 3;
        } else {
            *p++ = cc;
            if (p >= pmax)
                goto _ret;
        }
    }

_ret:
    return p - dst;
}

size_t url_encode(const char *src, size_t src_len, char *dst, size_t dst_len) {
    return encode(src, src_len, dst, dst_len, URL_MAP);
}

size_t url_component_encode(const char *src, size_t src_len, char *dst, size_t dst_len) {
    return encode(src, src_len, dst, dst_len, URL_COMPONENT_MAP);
}

size_t url_decode_length(const char* src, size_t src_len) {
    if (!src || !src_len)
        return 0;
    
    size_t count = 0;
    const char *src_end = src + src_len;
    while(src < src_end) {
        unsigned char cc = *src;
        if (cc == '%') {
            if (src + 3 > src_end)
                return count;
            src += 3;
        } else {
            ++src;
        }
        ++count;
    }

    return count;
}

size_t url_decode(const char* src, size_t src_len, char* dst, size_t dst_len) {
    if (!src || !src_len || !dst || !dst_len)
        return 0;
    
    char *p = dst, *pmax = dst + dst_len;
    for (size_t i = 0; i < src_len; ++i) {
        unsigned char cc = src[i];
        if (cc == '%') {
            if (i + 3 > src_len)
                goto _ret;
            *p++ = hex_decode_char_fast(src[i + 1], src[i + 2]);
            i += 2;
        } else {
            *p++ = cc;
            if (p >= pmax)
                goto _ret;
        }
    }

_ret:
    return p - dst;
}


#ifdef TEST_URLENCODE
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <url>\n", argv[0]);
        return 0;
    }

    size_t len = url_encode_length(argv[1], strlen(argv[1]));
    char buf[len + 1];
    buf[len] = '\0';
    size_t count = url_encode(argv[1], strlen(argv[1]), buf, len);
    printf("count = %lu\nurl: 【%s】\nenc: 【%s】\n", count, argv[1], buf);

    size_t len2 = url_decode_length(buf, len);
    char buf2[len2 + 1];
    buf2[len2] = '\0';
    size_t count2 = url_decode(buf, len, buf2, len2);
    printf("count = %lu, len2 = %lu\ndec: 【%s】\n", count2, len2, buf2);
    count2 = url_decode(buf, len, buf, len);
    buf[count2] = '\0';
    printf("slf: 【%s】\n", buf);
}
#endif