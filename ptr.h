/** 共享指针，带引用计数
 * @author kiven lee
 * @version 1.0
*/
#pragma once
#ifndef __PTR_H__
#define __PTR_H__

#include <stdint.h>
#include <memory.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ptr_head_t {
    uint32_t    ref;
    uint32_t    len;
    uint8_t     data[];
};

typedef uint8_t* ptr_t;

#define PTR_OFFSET_HEAD(x) ((struct _ptr_head_t*) ((x) - sizeof(struct _ptr_head_t)))

/** 分配内存资源并创建强引用
 * 
 * @param size 资源长度
 */
inline ptr_t ptr_malloc(size_t size) {
    struct _ptr_head_t* ret = (struct _ptr_head_t*) malloc(sizeof(struct _ptr_head_t) + size);
    ret->ref = 1;
    ret->len = size;
    return ret->data;
}

/** 资源的引用计数加1 */
inline void ptr_addref(ptr_t self) { ++(PTR_OFFSET_HEAD(self)->ref); }

/** 释放资源，引用计数减1，当引用计数为0时释放资源 */
inline void ptr_free(ptr_t self) {
    uint32_t ref = PTR_OFFSET_HEAD(self)->ref;
    if (ref && !--ref) free(PTR_OFFSET_HEAD(self));
}

/** 对资源进行扩容
 * 
 * @param size 增加的容量 
 */
inline void ptr_expand(ptr_t* self, uint32_t size) {
    struct _ptr_head_t* s = PTR_OFFSET_HEAD(*self);
    *self = ptr_malloc(s->len + size);
    memcpy(*self, s->data, s->len);
    ptr_free(s->data);
}

/** 写入内容
 * 
 * @param pos 写入位置
 * @param src 要写入的内容
 * @param len 要写入内容的长度
 */
inline void ptr_append(ptr_t* self, uint32_t pos, void* src, uint32_t len) {
    uint32_t nl = pos + len;
    if (PTR_OFFSET_HEAD(*self)->len < nl)
        ptr_expand(self, nl);
    memcpy(*self + pos, src, len);
}

#ifdef __cplusplus
}
#endif

#endif //__PTR_H__
