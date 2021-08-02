/** hashmap
 * @file hashmap.h
 * @author Kiven Lee (kivensoft@gmail.com)
 * @version 1.0
 * @date 2021-04-15
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "list.h"
#include "rbtree.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 计算hashmap需要的容量大小 */
#define HMAP_CAPACITY(n) ((n + 2) / 3 * 4)
/** 计算当前大小相对于容量因子的大小 */
#define HMAP_REQ(n) (((n + 3) >> 2) * 3)
/** hashmap中的列表允许的最大长度，超过该长度转为红黑树 */
#define HMAP_LIST_MAX 8

/** 计算hash的回调函数
 * @param v 要计算hash的键
 * @return hash值
 */
typedef uint32_t (*hmap_hash_fn) (const void* entry);

/** 用户扩展hmap_key_t类型的结构的内存释放函数
 * @param data 指向hmap_key_t类型的结构
 */
typedef void (*hmap_entry_free_fn) (void* data);

/** hash表键值对结构基本定义，用户自行扩展 */
typedef struct hmap_key_t {
    bool                    is_tree;        // 是否链表
    uint32_t                hash;           // hash值
} hmap_key_t;

typedef struct hmap_rbroot_t {
    rb_root_t               root;
    void*                   data;
} hmap_rbroot_t;

typedef struct hmap_lhead_t {
    list_head_t             head;
    void*                   
} hmap_lhead_t;

/** hash表指向的数组指针类型 */
typedef struct hmap_addr_t {
    uint8_t                 size;           // 0: 没有节点, <= HMAP_LIST_MAX: 链表长度, 255: 红黑树
    union {
        rb_root_t           root;
        list_head_t         head;
    };
} hmap_addr_t;

typedef struct hmap_t {
    uint32_t                cap;            // 容量
    uint32_t                len;            // 当前使用数量
    hmap_hash_fn            hash;           // hash算法回调函数
    rb_keycmp_fn            cmp;            // hash算法回调函数
    hmap_entry_free_fn      free;           // 内存释放回调函数
    hmap_addr_t*            addrs;          // 指向hmap_attr_t数组的指针
} hmap_t;

/** 初始化hashmap
 * @param self 指向自身 
 * @param size 初始化大小，最终分配的大小不一定是该值，需要经过计算
 * @param hash hash算法回调函数
 * @param keycmp 比较算法回调函数
 * @param free 内存释放回调函数
 */
inline static void hmap_init(hmap_t* self, uint32_t size, hmap_hash_fn hash, rb_keycmp_fn keycmp, hmap_entry_free_fn free) {
    self->cap = HMAP_CAPACITY(size);
    self->len = 0;
    self->hash = hash;
    self->cmp = keycmp;
    self->free = free;
    uint32_t nlen = sizeof(hmap_addr_t) * self->cap;
    self->addrs = (hmap_addr_t*) malloc(nlen);
    memset(self->addrs, 0, nlen);
}

/** 初始化hashmap
 * @param self 指向自身 
 * @param size 初始化大小，最终分配的大小不一定是该值，需要经过计算
 * @param hash hash算法回调函数
 * @param equals 比较算法回调函数
 * @param free 内存释放回调函数
 */
inline static hmap_t* hmap_malloc(uint32_t size, hmap_hash_fn hash, rb_keycmp_fn keycmp, hmap_key_free_fn free) {
    hmap_t* ret = malloc(sizeof(hmap_t));
    hmap_init(ret, size, hash, keycmp, free);
    return ret;
}

/** 获取值
 * 
 * @param self 指向自身
 * @param key 键
 * @return 找到的值
 */
extern hmap_key_t* hmap_get(hmap_t* self, const void* key);

/** 设置值
 * 
 * @param self 指向自身
 * @param key 键
 */
extern void hmap_put(hmap_t* self, hmap_key_t* key);

/** 删除
 * 
 * @param self 指向自身
 * @param key 键
 * @return _Bool 成功删除标志，0: 未找到, 1: 找到并删除 
 */
extern _Bool hmap_del(hmap_t* self, const void* key);

/** 清除所有项
 * 
 * @param self 指向自身
 */
extern void hmap_clear(hmap_t* self);

/** 释放不包括自身的所有内存
 * 
 * @param self 指向自身
 */
extern void hmap_free(hmap_t* self);

/** 预定义的int32类型hash函数 */
extern uint32_t hmap_key_int32_hash(const void* v1);

/** 预定义的string类型hash函数 */
extern uint32_t hmap_key_str_hash(const void* v1);

#ifdef __cplusplus
}
#endif


#endif // __HASHMAP_H__