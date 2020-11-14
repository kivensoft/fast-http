#pragma once
#ifndef __SHAREDPTR_H__
#define __SHAREDPTR_H__

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OFFSET_OF
// 获取member成员在结构体type中的位置偏移
#    define OFFSET_OF(type, member) ((size_t) &((type *)0)->member)
#endif

#ifndef CONTAINER_OF
// 根据结构体type变量中的域成员变量member的指针ptr来获取指向整个结构体变量的指针
#    define CONTAINER_OF(ptr, type, member) ({\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);\
        (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

typedef struct {
    uint32_t ref;   // 引用计数
    uint32_t len;   // 缓冲区大小
    uint32_t idx;   // 当前读写位置
    char data[];    // 缓冲区
} shared_ptr_t, *pshared_ptr_t;

/** 分配共享指针内存
 * @param len       缓冲区长度
 * @return          新的指针
*/
static inline pshared_ptr_t shared_ptr_malloc(uint32_t len) {
    pshared_ptr_t p = (pshared_ptr_t) malloc(sizeof(shared_ptr_t) + len);
    p->ref = 1;
    p->len = len;
    p->idx = 0;
    return p;
}

/** 释放指针 */
static inline void shared_ptr_free(pshared_ptr_t p) {
    if (p && p->ref && !--p->ref)
        free(p);
}

/** 返回指针缓冲区地址 */
static inline char* shared_ptr_data(pshared_ptr_t p) {
    return p ? p->data : NULL;
}

/** 返回指针引用计数 */
static inline uint32_t shared_ptr_reference(pshared_ptr_t p) {
    return p ? p->ref : 0;
}

/** 返回指针缓冲区长度 */
static inline uint32_t shared_ptr_length(pshared_ptr_t p) {
    return p ? p->len : 0;
}

/** 返回指针当前读写位置 */
static inline uint32_t shared_ptr_index(pshared_ptr_t p) {
    return p ? p->idx : 0;
}

/** 指针引用计数+1 */
static inline pshared_ptr_t shared_ptr_copy(pshared_ptr_t p) {
    if (p)
        ++p->ref;
    return p;
}

/** 克隆一个全新的指针，内容与源指针相同 */
static inline pshared_ptr_t shared_ptr_clone(pshared_ptr_t p) {
    if (!p) return NULL;
    pshared_ptr_t d = (pshared_ptr_t) malloc(sizeof(shared_ptr_t) + p->len);
    d->ref = 1;
    d->len = p->len;
    d->idx = p->idx;
    memcpy(d->data, p->data, p->len);
    return d;
}

/** 重置指针当前读写位置 */
static inline void shared_ptr_reset(pshared_ptr_t p) {
    if (p) p->idx = 0;
}

/** 读取指针内容
 * @param p         指针
 * @param dst       读取内容的目标写入地址
 * @param dstlen    目标地址长度
 * @return          实际读取的长度
*/
static inline uint32_t shared_ptr_read(pshared_ptr_t p, void* dst, uint32_t dstlen) {
    if (!p) return 0;
    uint32_t n = p->len - p->idx;
    if (n > dstlen) n = dstlen;
    memcpy(dst, p->data + p->idx, n);
    p->idx += n;
    return n;
}

/** 写入内容到指针
 * @param p         指针
 * @param src       写入指针的源地址
 * @param srclen    源地址长度
 * @return          实际写入长度
*/
static inline uint32_t shared_ptr_write(pshared_ptr_t p, void* src, uint32_t srclen) {
    if (!p) return 0;
    uint32_t n = p->len - p->idx;
    if (n > srclen) n = srclen;
    memcpy(p->data + p->idx, src, n);
    p->idx += n;
    return n;
}

#ifdef __cplusplus
}
#endif

#endif // __SHAREDPTR_H__