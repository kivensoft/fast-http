/** 字符串库，创建的字符串对象具备长度属性，可动态扩展
 * @author kiven lee
 * @version 1.0
*/
#pragma once
#ifndef __STR_H__
#define __STR_H__

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>

// usage: string_cats(x, PP_ARGS(a, b, c, d))
// expand: string_cats(x, 4, a, b, c, d);
#ifndef PP_NARG
#   define PP_ARGS(...) PP_NARG(__VA_ARGS__), __VA_ARGS__
#   define PP_NARG(...) PP_NARG_(__VA_ARGS__, PP_RSEQ_N())
#   define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#   define PP_ARG_N(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,N,...) N
#   define PP_RSEQ_N() 32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
#endif

#ifdef __cplusplus
extern "C" {
#endif

// 自定义字符串复制功能，返回值是复制的长度
static inline size_t strcpy_len(char* dst, const char* src) {
    const char *osrc = src;
    while ((*dst++ = *src++));
    return src - osrc - 1;
}

typedef struct {
    char* data;
    uint32_t len;
} fast_string_t;

typedef struct {
    uint32_t ref;
    uint32_t len;
    uint32_t cap;
    char data[];
} string_t;

/** 分配string_t空间，容量为capacity
 * @param capacity      string_t的容量
 * @return              string_t指针
*/
extern string_t* string_malloc(uint32_t capacity);

/** string_t增加引用
 * @param src           增加引用的string_t
*/
static inline string_t* string_copy(string_t* src) {
    ++src->ref;
    return src;
}

/** string_t引用计数减一，如果引用计数为0，则释放内存
 * @param src           string_t
*/
static inline void string_free(string_t* src) {
    if (src->ref && !--src->ref)
        free(src);
}

/** string_t引用计数减一，如果引用计数为0，则释放内存
 * @param src           string_t
*/
extern void string_frees(size_t count, ...);

/** 返回string_t指向的c风格字符串地址
 * @param src           string_t
 * @return              c风格字符串地址
*/
static inline char* string_cstr(string_t* src) {
    return src->data;
}

/** 返回string_t的长度
 * @param src           string_t
 * @return              字符串长度
*/
static inline uint32_t string_len(string_t* src) {
    return src->len;
}

/** 返回string_t的容量
 * @param src           string_t
 * @return              字符串容量
*/
static inline uint32_t string_cap(string_t* src) {
    return src->cap;
}

/** 返回string_t的引用计数
 * @param src           string_t
 * @return              引用计数
*/
static inline uint32_t string_ref(string_t* src) {
    return src->ref;
}

/** 缩减字符串长度，通常用于截断
 * @param src           string_t
 * @param count         缩减长度
*/
static inline void string_trim(string_t *src, uint32_t count) {
    uint32_t len = src->len;
    if (len < count) count = len;
    len -= count;
    src->len = len;
    src->data[len] = '\0';
}

/** 分配string_t空间，原始内容为src内容
 * @param src           源字符串内容
 * @return              string_t指针
*/
extern string_t* string_from_cstr(const char *src);

/** 复制一个stirng，全新分配内存空间
 * @param src           string_t
 * @return              新的string_t指针
*/
extern string_t* string_clone(string_t* src);

/** string_t增加容量
 * @param src           string_t
 * @param capacity      新增容量
 * @return              string_t指针
*/
extern string_t* string_expand(string_t* src, uint32_t capacity);

/** 连接字符串
 * @param dst           连接到的目标字符串
 * @param src           源字符串
 * @return              新的string_t
*/
extern string_t* string_cat(string_t* dst, const char *src);

/** 连接字符串
 * @param dst           连接到的目标字符串
 * @param count         字符串不定参数个数
 * @param ...           不定参数
 * @return              新的string_t
*/
extern string_t* string_cats(string_t* dst, size_t count, ...);

/** 连接字符串
 * @param dst           连接到的目标字符串
 * @param src           源字符串
 * @return              新的string_t
*/
extern string_t* string_join(string_t* dst, const string_t* src);

/** 连接字符串
 * @param dst           连接到的目标字符串
 * @param count         字符串不定参数个数
 * @param ...           不定参数, 必须为string_t*类型
 * @return              新的string_t
*/
extern string_t* string_joins(string_t* dst, size_t count, ...);

#ifdef __cplusplus
}
#endif

#endif // __STR_H__