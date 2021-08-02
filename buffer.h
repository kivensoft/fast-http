/** 自动增长的缓冲区操作函数, 内部实现为不连续数组，分块大小为1024
 * @file buffer.h
 * @author Kiven Lee
 * @version 1.0
 * @date 2020-08-07
*/

#pragma once
#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 范例：
    buffer_t* buf = buffer_init(0); // 缺省大小分配，也可指定初始容量大小
    buffer_write(buf, "12345678", 8);
    char dd[8];
    buffer_read(buf, dd, 0, 8);
    char c = *buffer_get(buf, 3);
    *buffer_get(buf, 4) = 'x';
    buffer_t* buf2 = buffer_copy(buf);
    buffer_destroy(buf);
    buffer_destroy(buf2);
*/

#define BUFFER_B1 10
#define BUFFER_C1 (1 << BUFFER_B1)
#define BUFFER_M1 (BUFFER_C1 - 1)

// buffer_t缓冲区数据结构
typedef struct {
    uint32_t                ref;        // 引用计数
    uint32_t                len;        // 缓冲区长度
    uint32_t                cap;        // 缓冲区容量
    uint32_t                array_cap;  // 缓冲区二级指针索引数组容量
    void**                  array;      // 二级指针，指向每个块的指针
} buffer_t;

/** 循环缓冲区内容的回调函数定义
 * @param arg       用户定制参数，调用buffer_foreach时传入
 * @param src       源地址
 * @param begin     起始位置
 * @param end       结束位置
 * @return          0: 结束读取, 其他值: 继续读取(一般返回处理的长度)
*/
typedef size_t (*buffer_on_foreach) (void* arg, void* data, size_t len);

/** 初始化缓冲区，分配推荐容量和二级指针容量
 * @param p         缓冲区指针
 * @param capacity  容量大小，实际生成的容量可能大于该值，因为需要考虑到分块的缘故
 * @param array_cap 二级指针容量大小，实际生成的二级指针容量可能大于该值，因为需要考虑到分块的缘故
*/
extern void buffer_init_ex(buffer_t* p, size_t capacity, size_t array_cap);

/** 初始化指定容量大小的缓冲区
 * @param p         缓冲区指针
 * @param capacity  容量大小，实际生成的容量可能大于该值，因为需要考虑到分块的缘故
*/
inline static void buffer_init(buffer_t* p, size_t capacity) {
    buffer_init_ex(p, capacity, 0);
}

/** 释放缓冲区指针占用的所有内存(不释放buffer_t本身)
 * @param p         缓冲区指针
*/
extern void buffer_destroy(buffer_t* p);

/** 创建缓冲区对象并生成指定容量大小和二级数组容量的缓冲区
 * @param capacity  容量大小，实际生成的容量可能大于该值，因为需要考虑到分块的缘故
 * @param array_cap 预分配的二级指针数组大小
 * @param mf        自定义malloc函数指针，NULL时使用系统malloc
 * @param f         自定义free函数指针，NULL时使用系统free
 * @return          返回缓冲区对象
*/
inline static buffer_t* buffer_malloc_ex(size_t capacity, size_t array_cap) {
    buffer_t* ret = malloc(sizeof(buffer_t));
    buffer_init_ex(ret, capacity, array_cap);
    return ret;
}

/** 创建缓冲区对象并生成指定容量大小的缓冲区
 * @param capacity  容量大小，实际生成的容量可能大于该值，因为需要考虑到分块的缘故
 * @return          返回缓冲区对象
*/
inline static buffer_t* buffer_malloc(size_t capacity, size_t array_capacity) {
    buffer_t* ret = malloc(sizeof(buffer_t));
    buffer_init(ret, capacity);
    return ret;
}

/** 释放缓冲区指针占用的所有内存
 * @param p         缓冲区指针
*/
inline static void buffer_free(buffer_t* p) {
    buffer_destroy(p);
    if (p && !p->ref)
        free(p);
}

/** 返回缓冲区容量大小
 * @param p         缓冲区指针
*/
inline static uint32_t buffer_capacity(buffer_t* p) {
    return p ? p->cap : 0;
}

/** 返回缓冲区长度大小
 * @param p         缓冲区指针
*/
inline static uint32_t buffer_length(buffer_t* p) {
    return p ? p->len : 0;
}

/** 返回缓冲区引用计数
 * @param p         缓冲区指针
*/
inline static uint32_t buffer_reference(buffer_t* p) {
    return p ? p->ref : 0;
}

/** 缓冲区指针赋值，缓冲区引用计数+1
 * @param p         缓冲区指针
*/
inline static buffer_t* buffer_copy(buffer_t* p) {
    if (p)
        p->ref++;
    return p;
}

/** 缩减缓冲区大小
 * @param p         缓冲区指针
 * @param count     缩减数量
*/
inline static void buffer_trim(buffer_t* p, size_t count) {
    if (p) {
        uint32_t len = p->len;
        if (len < count) count = len;
        p->len = len - count;
    }
}

/** 复制一个全新的缓冲区，内容一样(容量不一定一样)
 * @param p         缓冲区指针
 * @return          新的缓冲区指针
*/
extern buffer_t* buffer_clone(buffer_t* p);

/** 扩展缓冲区容量
 * @param p         缓冲区指针
 * @param capacity  新的容量
*/
extern void buffer_expand_capacity(buffer_t* p, size_t capacity);

/** 设置缓冲区长度, 可扩充也可缩小
 * @param p         缓冲区指针
 * @param newlen    新的长度
*/
extern void buffer_set_length(buffer_t* p, size_t newlen);

/** 循环获取内容，通过回调方式进行，效率比较高
 * @param p         缓冲区指针
 * @param arg       用户自定义参数，回调时传给回调函数
 * @param begin     起始位置
 * @param end       结束位置
 * @param func      回调函数
 * @return          处理的内容长度
*/
extern size_t buffer_foreach(buffer_t* p, void* arg, size_t begin, size_t end,
        buffer_on_foreach on_foreach);

/** 写入缓冲区
 * @param src       源地址
 * @param srclen    写入长度
 * @return          写入的数量
*/
extern size_t buffer_write(buffer_t* p, const void* src, size_t srclen);

/** 读取缓冲区，采用回调方式进行，半开区间方式，即读取begin <= x < end
 * @param p         缓冲区指针
 * @param arg       用户自定义参数，回调时传给回调函数
 * @param begin     开始位置
 * @param end       结束位置
 * @param func      回调函数
 * @return          读取的数量
*/
extern size_t buffer_read(buffer_t* p, void* dst, size_t src_begin, size_t src_end);

/** 获取缓冲区指定位置的指针，返回指针是为了可写入
 * @param p         缓冲区指针
 * @param pos       指定的位置
 * @return          指定位置的指针
*/
inline static char* buffer_get(buffer_t* p, size_t pos) {
    return (char*)(p->array[pos >> BUFFER_B1]) + (pos & BUFFER_M1);
}

#ifdef __cplusplus
}
#endif

#endif // __BUFFER_H__