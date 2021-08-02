#include <stdlib.h>
#include <string.h>

#include "str.h"

#define MIN_CAP 32
#define LARGE_CAP 1024

str_t str_malloc(uint32_t capacity) {
    // 最小分配32字节，小于1024时倍数增长，大于1024时按1024增长
    uint32_t cap = MIN_CAP, min_cap = capacity + sizeof(struct _str_head_t) + 1;

    while (cap < min_cap)
        if (min_cap <= LARGE_CAP) cap <<= 1; else cap += LARGE_CAP;

    struct _str_head_t *p = malloc(cap);
    p->len = 0;
    p->cap = cap - sizeof(struct _str_head_t) - 1;
    p->data[0] = '\0';

    return p->data;
}

void str_frees(int count, ...) {
    va_list args;
    va_start(args, count);
    while (count--)
        str_free(va_arg(args, str_t));
    va_end(args);
}

void str_grow(str_t* self, uint32_t capacity) {
    struct _str_head_t *s = STR_OFFSET_HEAD(*self);
    if (capacity <= s->cap) return;
    str_t p = str_malloc(capacity);

    memcpy(p, s->data, s->len + 1);
    STR_OFFSET_HEAD(p)->len = s->len;
    str_free(s);

    *self = p;
}

str_t str_copy(str_t src) {
    str_t p = str_malloc(src ? STR_OFFSET_HEAD(src)->len : 0);
    if (src) {
        memcpy(p, src, STR_OFFSET_HEAD(src)->len + 1);
        STR_OFFSET_HEAD(p)->len = STR_OFFSET_HEAD(src)->len;
    }
    return p;
}

void str_append(str_t* self, const char *src, uint32_t len) {
    if (!src) return;
    uint32_t sl = STR_OFFSET_HEAD(*self)->len, nl = sl + len;
    if (nl > STR_OFFSET_HEAD(*self)->cap) str_grow(self, nl);
    memcpy(*self + sl, src, len);
    (*self)[nl] = '\0';
    STR_OFFSET_HEAD(*self)->len = nl;
}

void string_cats(str_t* self, int count, ...) {
    uint32_t len = STR_OFFSET_HEAD(*self)->len, cap = STR_OFFSET_HEAD(*self)->cap;

    uint32_t args_len[count];
    va_list args;

    // 计算所有字符串的总长度，加上自身长度，等于将要扩展的长度
    va_start(args, count);
    for (int i = 0, imax = count; i < imax; ++i) {
        char *p = va_arg(args, char*);
        if (p) {
            args_len[i] = (uint32_t) strlen(p);
            len += args_len[i];
        }
    }
    va_end(args);

    if (cap < len) str_grow(self, len);

    char *pos = self + STR_OFFSET_HEAD(self)->len;
    va_start(args, count);
    for (int i = 0, imax = count; i < imax; ++i) {
        uint32_t al = args_len[i];
        memcpy(pos, va_arg(args, char*), al);
        pos += al;
    }
    va_end(args);
    *pos = '\0';
}
