#include "buffer.h"
#include <string.h>

#define B0 5
#define C0 (1 << B0)
#define B1 10
#define C1 (1 << B1)
#define M1 (C1 - 1)

void buffer_init_ex(buffer_t* p, size_t capacity, size_t array_cap) {
    // cap: 实际分配容量，alen: 二级指针数组需要的大小, acap: 实际分配的二级指针容量
    size_t cap, alen, acap;
    // 分配空间<=1024，按32的幂次方增长，>1024按1024进行增长
    if (capacity <= C1) {
        // 容量计算，针对实际容量小于1024的情况，每次增长1倍
        cap = C0;
        while (cap < capacity)
            cap <<= 1;

        alen = 1;
        acap = 1;
    } else {
        // 容量计算，每次增长1024
        cap = C1;
        while (cap < capacity) cap += C1;

        // 二级索引数组容量计算，每次增长1倍
        alen = cap >> B1;
        acap = 1;
        while (acap < alen)
            acap <<= 1;
    }

    // 处理二级指针预分配情况
    while (acap < array_cap)
        acap <<= 1;
    
    p->array = malloc(sizeof(void*) * acap);
    p->ref = 1;
    p->len = 0;
    p->cap = cap;
    p->array_cap = acap;
    for (size_t i = 0; i < alen; ++i)
        p->array[i] = malloc(C1);
}

void buffer_destroy(buffer_t* p) {
    if (p && p->ref && !--p->ref && p->cap) {
        // 释放所有分配的块，使用数组索引中的指针进行释放
        size_t alen = p->cap < C1 ? 1 : p->cap >> B1;
        for (void **ap = p->array, **ape = &ap[alen]; ap < ape; ++ap)
            free(*ap);
        // 释放二级数组索引
        free(p->array);
        memset(p, 0, sizeof(buffer_t));
    }
}

/** 重新分配容量（适用于容量不大于1024时）
 * @param p 缓冲区地址
 * @param capacity 新的容量
*/
static inline void realloc_small(buffer_t* p, size_t capacity) {
    void* new_data = malloc(capacity);
    // 复制原有内容到新分配的空间
    if (p->len)
        memcpy(new_data, p->array[0], p->len);
    if (p->array_cap)
        free(p->array[0]);
    else
        p->array = malloc(sizeof(void*));
    p->array[0] = new_data;
}

/** 重新分配二级索引数组
 * @param p 缓冲区地址
 * @param capacity 新的容量
*/
static inline void realloc_array(buffer_t* p, size_t capacity) {
    void** array = malloc(sizeof(void*) * capacity);
    if (p->array_cap) {
        memcpy(array, p->array, sizeof(void*) * p->array_cap);
        free(p->array);
    }
    p->array = array;
    p->array_cap = capacity;
}

/** 扩展容量，自动扩展，用于已知需要扩展的容量大小时
 * @param p 缓冲区指针
 * @param capacity 新容量大小
*/
void buffer_expand_capacity(buffer_t* p, size_t capacity) {
    size_t cap = p->cap;
    if (capacity < cap) return;
    // 容量未到1024，按倍数扩展
    if (capacity <= C1) {
        while (cap < capacity) cap <<= 1;
        realloc_small(p, cap);
    } else { // 容量超过1024，按1024步长扩展
        // 原有容量不到1024，则扩展data到1024
        if (p->len < C1) {
            cap = C1;
            realloc_small(p, C1);
        }

        while (cap < capacity) cap += C1;

        // new_alen: 新数组需要的长度, used_alen原数组长度
        size_t new_alen = cap >> B1, used_alen = p->cap >> B1;
        // alloc_acap 计算新容量大小， old_acap 原有数组容量
        size_t alloc_acap = p->array_cap, old_acap = alloc_acap;
        if (new_alen > old_acap) {
            while (alloc_acap < new_alen) alloc_acap <<= 1;
            realloc_array(p, alloc_acap);
        }

        // 为扩展的容量以1024为单位分配内存，并保存地址到二级指针索引
        void** arr = &(p->array[used_alen]);
        for (size_t i = used_alen; i < new_alen; ++i)
            *arr++ = malloc(C1);
    }
    p->cap = cap;
}

buffer_t* buffer_clone(buffer_t* p) {
    if (!p) return NULL;
    uint32_t len = p->len;
    buffer_t* ret = buffer_malloc(len, 0);
    ret->len = len;

    // 复制源缓冲区的所有内容
    for (int i = 0; len; ++i) {
        uint32_t cl = len >= C1 ? C1 : len;
        memcpy(ret->array[i], p->array[i], cl);
        len -= cl;
    }

    return ret;
}

void buffer_set_length(buffer_t* p, size_t newlen) {
    if (p) {
        // 新的长度大于已有容量，则进行扩充
        if (newlen > p->cap)
            buffer_expand_capacity(p, newlen);
        p->len = (uint32_t) newlen;
    }
}

size_t buffer_foreach(buffer_t* p, void* arg, size_t begin, size_t end, buffer_on_foreach on_foreach) {
    if (!p) return 0;
    size_t pos = begin;
    while (pos < end) {
        char* data = p->array[pos >> B1];
        size_t off = pos & M1;
        size_t x = C1 - off, y = end - pos;
        size_t len = x < y ? x : y;
        if (!on_foreach(arg, data + off, len))
            return pos - begin;
        pos += len;
    }
    return pos - begin;
}

/** buffer_write默认实现的函数
 * @param arg 源地址
 * @param data 要写入的目标地址
 * @param len 可以写入的目标地址长度
 * @return 写入长度
*/
static size_t on_write(void* arg, void* data, size_t len) {
    memcpy(data, *(char**)arg, len);
    *(char**)arg += len; // 源地址指针前移
    return len;
}

size_t buffer_write(buffer_t* p, const void* src, size_t srclen) {
    if (!p) return 0;
    size_t oldlen = p->len, newlen = oldlen + srclen;
    buffer_set_length(p, newlen);
    buffer_foreach(p, &src, oldlen, newlen, on_write);
    return srclen;
}

/** buffer_read模式实现的回调函数
 * @param arg 目标地址
 * @param data 缓冲区的数据源
 * @param len 数据源长度
 * @return 实际读取长度
 * 
*/
static size_t on_read(void* arg, void* data, size_t len) {
    memcpy(*(char**)arg, data, len);
    *(char**)arg += len;
    return len;
}

size_t buffer_read(buffer_t* p, void* dst, size_t src_begin, size_t src_end) {
    if (!p) return 0;
    buffer_foreach(p, &dst, src_begin, src_end, on_read);
    return src_end - src_begin;
}

//======================================================================
#ifdef TEST_BUFFER
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>

struct abc { uint32_t a : 1; uint32_t b : 31; };

#define ASSERT(x) if (!(x)) printf("%s:%d -- buffer test fail!\n", __FILE__, __LINE__)

int main() {
    const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    uint8_t data[5000];
    // 初始化data，用随机值填充
    srand((unsigned)time(NULL));
    for (int i = 0; i < sizeof(data); i++) {
        data[i] = base64[rand() & 63];
    }

    // 测试小内存分配有效性
    buffer_t buf, *pb = &buf;
    buffer_init(pb, 0);
    ASSERT(!pb->len && pb->cap == C0);
    buffer_write(pb, data, 30);
    ASSERT(pb->len == 30);
    ASSERT(!memcmp(data, pb->array[0], pb->len));
    buffer_write(pb, data + 30, 30);
    ASSERT(pb->len == 60 && pb->cap == 64 && pb->array_cap == 1);
    ASSERT(!memcmp(data, pb->array[0], pb->len));

    pb->len = 0;
    buffer_write(pb, data, 1023);
    buffer_write(pb, data + 1023, 1023);
    buffer_write(pb, data + 2046, 4);
    ASSERT(pb->len == 2050 && pb->cap == 3072 && pb->array_cap == 4);
    ASSERT(!memcmp(data, pb->array[0], 1024));
    ASSERT(!memcmp(data + 1024, pb->array[1], 1024));
    ASSERT(!memcmp(data + 2048, pb->array[2], 2));

    struct abc xx = {.a = 1, .b = 287};
    printf("abc = %" PRId64 "\n", sizeof(struct abc));
    printf("xx = %x, a = %d, b = %d\n", *(uint32_t*)(&xx), xx.a, xx.b);

    printf("test complete!\n");
}
#endif