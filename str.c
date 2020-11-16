#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "str.h"

#define MIN_CAP 32
#define LARGE_CAP 1024

string_t* string_malloc(uint32_t capacity) {
    // 最小分配32字节，小于1024时倍数增长，大于1024时按1024增长
    uint32_t cap, min_cap = capacity + sizeof(string_t) + 1;

    if (min_cap <= LARGE_CAP)
        for (cap = MIN_CAP; cap < min_cap;) cap <<= 1;
    else
        for (cap = LARGE_CAP; cap < min_cap;) cap += LARGE_CAP;

    string_t* p = (string_t*) malloc(cap);

    p->ref = 1;
    p->len = 0;
    p->cap = cap - sizeof(string_t) - 1;
    *(p->data) = '\0';

    return p;
}

void string_frees(size_t count, ...) {
    va_list args;
    va_start(args, count);
    for (unsigned i = 0, imax = count; i < imax; ++i)
        string_free(va_arg(args, string_t*));
    va_end(args);
}

string_t* string_from_cstr(const char *src) {
    uint32_t len = src ? strlen(src) : 0;
    string_t* p = string_malloc(len);

    if (src) {
        memcpy(p->data, src, len + 1);
        p->len = len;
    }

    return p;
}

string_t* string_expand(string_t* src, uint32_t capacity) {
    string_t* p = string_malloc(capacity);

    if (src) {
        memcpy(p->data, src->data, src->len + 1);
        p->len = src->len;
        string_free(src);
    }

    return p;
}

string_t* string_clone(string_t* src) {
    uint32_t cap = src ? src->cap : 0;
    string_t* p = string_malloc(cap);

    if (src) {
        memcpy(p->data, src->data, src->len + 1);
        p->len = src->len;
    }

    return p;
}

string_t* string_cat(string_t* dst, const char *src) {
    uint32_t len, cap, sl = src ? strlen(src) : 0;
    if (dst) {
        len = sl + dst->len;
        cap = dst->cap;
    } else {
        len = sl;
        cap = 0;
    }

    if (cap < len) {
        string_t* p = string_malloc(len);
        if (dst) {
            memcpy(p->data, dst->data, dst->len + 1);
            p->len = dst->len;
            string_free(dst);
        }
        dst = p;
    }

    if (src) {
        memcpy(dst->data + dst->len, src, sl + 1);
        dst->len += sl;
    }

    return dst;
}

string_t* string_cats(string_t* dst, size_t count, ...) {
    uint32_t len, cap;
    if (dst) {
        len = dst->len;
        cap = dst->cap;
    } else {
        len = 0;
        cap = 0;
    }

    uint32_t args_len[count];
    va_list args;
    va_start(args, count);
    for (unsigned i = 0, imax = count; i < imax; ++i) {
        char *p = va_arg(args, char*);
        if (p) {
            size_t n = strlen(p);
            args_len[i] = n;
            len += n;
        }
    }
    va_end(args);

    if (cap < len) {
        string_t* p = string_malloc(len);
        if (dst) {
            memcpy(p->data, dst->data, dst->len + 1);
            p->len = dst->len;
            string_free(dst);
        }
        dst = p;
    }

    char* pdata = dst->data;
    uint32_t pos = dst->len;
    va_start(args, count);
    for (unsigned i = 0, imax = count; i < imax; ++i) {
        char *p = va_arg(args, char*);
        if (p) {
            uint32_t n = args_len[i];
            memcpy(pdata + pos, p, n);
            pos += n;
        }
    }
    va_end(args);

    dst->len = len;
    pdata[len] = '\0';

    return dst;
}

string_t* string_join(string_t* dst, const string_t* src) {
    uint32_t len, cap;
    // 源字符串长度
    len = src ? src->len : 0;
    if (dst) {
        len += dst->len;
        cap = dst->cap;
    } else {
        cap = 0;
    }

    // 加上追加的字符串，如果容量不足，则重新分配空间
    if (cap < len) {
        string_t* p = string_malloc(len);
        if (dst) {
            memcpy(p->data, dst->data, dst->len + 1);
            p->len = dst->len;
            string_free(dst);
        }
        dst = p;
    }

    if (src)  {
        memcpy(dst->data + dst->len, src->data, src->len + 1);
        dst->len = len;
    }

    return dst;
}

string_t* string_joins(string_t* dst, size_t count, ...) {
    uint32_t len, cap;
    // 原有字符串长度
    if (dst) {
        len = dst->len;
        cap = dst->cap;
    } else {
        len = 0;
        cap = 0;
    }

    // 计算多个参数的字符串总长度
    va_list args;
    va_start(args, count);
    for (unsigned i = 0, imax = count; i < imax; ++i) {
        string_t* pa = va_arg(args, string_t*);
        if (pa)
            len += pa->len;
    }
    va_end(args);

    // 如果源字符串容量不足，则扩展源字符串空间
    if (cap < len) {
        string_t* p = string_malloc(len);
        if (dst) {
            memcpy(p->data, dst->data, dst->len + 1);
            p->len = dst->len;
            string_free(dst);
        }
        dst = p;
    }

    // 将参数的内容挨个追加到源字符串中
    char* pdata = dst->data;
    uint32_t pos = dst->len;
    va_start(args, count);
    for (unsigned i = 0, imax = count; i < imax; ++i) {
        string_t* pa = va_arg(args, string_t*);
        if (pa) {
            memcpy(pdata + pos, pa->data, pa->len);
            pos += pa->len;
        }
    }
    va_end(args);

    dst->len = len;
    pdata[len] = '\0';

    return dst;
}