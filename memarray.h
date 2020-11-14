/** 内存页数组，采取分页方式实现内存自动扩展
 *  优点：
 *      1. 分页大小由用户自定义，系统会自动选择合适的2的幂次方容量
 *      2. 内部存储页面地址信息采用双向链表的方式，
 * @author Kiven Lee
 * @version 1.0
 * @date 2020-11-11
*/

#pragma once
#ifndef __MEMARRAY_H__
#define __MEMARRAY_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 内存页地址信息双向链表 */
struct _mem_list_t {
    struct _mem_list_t* prev;
    struct _mem_list_t* next;
    uint8_t*            data;
};

/** mem_array_t缓冲区数据结构 */
typedef struct {
    uint32_t        ref;        // 引用计数
    uint32_t        len;        // 缓冲区长度
    uint32_t        cap;        // 缓冲区容量
    uint32_t        pagesize;   // 分页大小 

    struct _mem_list_t list;   // 内存页链表
} mem_array_t;

/** 循环缓冲区内容的回调函数定义
 * @param arg       用户定制参数，调用buffer_foreach时传入
 * @param data      来自缓冲区的地址
 * @param len       缓冲区长度
 * @return          0: 结束读取, 其他值: 继续读取(一般返回处理的长度)
*/
typedef uint32_t (*mem_array_on_foreach) (void* arg, void* data, uint32_t len);

/** 获取缓冲区引用计数
 * @param self      缓冲区指针
 * @return          缓冲区引用计数
*/
inline static uint32_t mem_array_reference(mem_array_t* self) {
    return self->ref;
}

/** 获取缓冲区长度
 * @param self      缓冲区指针
 * @return          缓冲区长度
*/
inline static uint32_t mem_array_length(mem_array_t* self) {
    return self->len;
}

/** 获取缓冲区容量
 * @param self      缓冲区指针
 * @return          缓冲区容量
*/
inline static uint32_t mem_array_capicity(mem_array_t* self) {
    return self->cap;
}

/** 初始化缓冲区
 * @param self      缓冲区指针
 * @param pagesize  分页大小，缓冲区扩容以分页大小为单位进行扩容
*/
extern void mem_array_init(mem_array_t* self, uint32_t pagesize);

/** 释放缓冲区指针占用的所有内存(不释放buffer_t本身)
 * @param self      缓冲区指针
*/
extern void mem_array_destroy(mem_array_t* self);

/** 创建缓冲区对象
 * @param pagesize  分页大小，缓冲区扩容以分页大小为单位进行扩容
 * @return          返回缓冲区对象
*/
inline static mem_array_t* mem_array_malloc(uint32_t pagesize) {
    mem_array_t* ret = malloc(sizeof(mem_array_t));
    mem_array_init(ret, pagesize);
    return ret;
}

/** 释放缓冲区指针占用的所有内存
 * @param self      缓冲区指针
*/
inline static void mem_array_free(mem_array_t* self) {
    mem_array_destroy(self);
    if (self && !self->ref)
        free(self);
}

/** 缓冲区指针赋值，缓冲区引用计数+1
 * @param self      缓冲区指针
*/
inline static mem_array_t* mem_array_copy(mem_array_t* self) {
    if (self)
        ++self->ref;
    return self;
}

/** 复制一个全新的缓冲区，内容一样(容量不一定一样)
 * @param self      缓冲区指针
 * @return          新的缓冲区指针
*/
extern mem_array_t* mem_array_clone(mem_array_t* self);

/** 设置缓冲区长度, 可扩充也可缩小
 * @param self      缓冲区指针
 * @param newlen    新的长度
*/
extern void mem_array_set_length(mem_array_t* self, uint32_t new_len);

/** 设置缓冲区容量, 可扩充也可缩小
 * @param self      缓冲区指针
 * @param newlen    新的容量
*/
extern void mem_array_set_capicity(mem_array_t* self, uint32_t new_capicity);

/** 增加缓冲区长度
 * @param self      缓冲区指针
 * @param size      增加的大小
*/
inline static void mem_array_inc_len(mem_array_t* self, uint32_t size) {
    mem_array_set_length(self, self->len + size);
}

/** 设置缓冲区长度, 扩展长度并对齐到下一页
 * @param self      缓冲区指针
 * @return          新的对齐的长度
*/
extern uint32_t mem_array_align_len(mem_array_t* self);

/** 获取缓冲区指定位置的指针，为了效率, 本函数不做任何校验，传入大于容量的pos值将引发不可预料的后果
 * @param self      缓冲区指针
 * @param offset    指定的位置
 * @return          指定位置的指针
*/
extern uint8_t* mem_array_get(mem_array_t* self, uint32_t offset);

/** 获取缓冲区指定位置的页剩余的连续地址的空间大小
 * @param self      缓冲区指针
 * @param offset    指定的位置
 * @return          连续地址的空间大小(字节为单位)
*/
inline static uint32_t mem_array_surplus(mem_array_t* self, uint32_t offset) {
    return self->pagesize - (offset & (self->pagesize - 1));
}

/** 获取用于追加写入的页面信息(返回页面地址和可用长度)
 * @param self      缓冲区指针
 * @param out_ptr   返回写入地址的指针
 * @param out_len   返回写入地址长度
*/
extern void mem_array_last_page(mem_array_t* self, uint8_t** out_ptr, uint32_t* out_len);

/** 获取缓冲区下一个写入位置的指针
 * @param self      缓冲区指针
 * @param offset    指定的位置
 * @return          指定位置的指针
*/
inline static uint8_t* mem_array_next(mem_array_t* self) {
    return mem_array_get(self, self->len);
}

/** 获取缓冲区下一个写入位置的指针的连续空间大小
 * @param self      缓冲区指针
 * @param offset    指定的位置
 * @return          下一个写入位置连续空间大小
*/
inline static uint32_t mem_array_next_surplus(mem_array_t* self) {
    return mem_array_surplus(self, self->len);
}

/** 查找地址在缓冲区对应的偏移位置
 * @param self      缓冲区指针
 * @param pointer   地址指针
 * @return          地址指针在缓冲区中的偏移，找不到返回0xFFFFFFFF
*/
extern uint32_t mem_array_offset(mem_array_t* self, const void* pointer);

/** 循环获取内容，通过回调方式进行，效率比较高, 循环的范围是在整个容量内，不限于使用长度
 * @param self      缓冲区指针
 * @param arg       用户自定义参数，回调时传给回调函数
 * @param off       起始偏移位置
 * @param len       循环处理长度
 * @param callback  回调函数
 * @return          处理的内容长度
*/
extern uint32_t mem_array_foreach(mem_array_t* self, void* arg, uint32_t off, uint32_t len, mem_array_on_foreach callback);


/** 写入缓冲区, 写入操作允许自动扩展缓冲区大小
 * @param self      缓冲区指针
 * @param off       起始偏移位置
 * @param len       写入长度
 * @param src       写入内容的来源地址
 * @return          写入的数量
*/
extern uint32_t mem_array_write(mem_array_t* self, uint32_t off, uint32_t len, const void* src);

/** 读取缓冲区，读取操作只允许读取有效长度范围内的数据
 * @param self      缓冲区指针
 * @param off       起始偏移位置
 * @param len       读取长度
 * @param dst       读取内容写入的目的地
 * @return          读取的数量
*/
extern uint32_t mem_array_read(mem_array_t* self, uint32_t off, uint32_t len, void* dst);

/** 追加写入缓冲区, 写入操作允许自动扩展缓冲区大小
 * @param self      缓冲区指针
 * @param src       写入内容的来源地址
 * @param len       写入长度
 * @return          写入的数量
*/
inline static uint32_t mem_array_append(mem_array_t* self, const void* src, uint32_t len) {
    return mem_array_write(self, self->len, len, src);
}

/** 在指定的偏移范围内查找第一个符合条件的字符
 * @param self      缓冲区指针
 * @param off       起始偏移位置
 * @param len       写入长度
 * @param ch        要查找的字符串
 * @return          找到的位置，找不到返回-1
*/
extern int mem_array_chr(mem_array_t* self, uint32_t off, uint32_t len, uint8_t ch);

/** 在指定的偏移范围内查找最后一个符合条件的字符
 * @param self      缓冲区指针
 * @param off       起始偏移位置
 * @param len       写入长度
 * @param ch        要查找的字符串
 * @return          找到的位置，找不到返回-1
*/
extern int mem_array_lastchr(mem_array_t* self, uint32_t off, uint32_t len, uint8_t ch);

/** 判断指定位置是否特定内容
 * @param self      缓冲区指针
 * @param off       起始偏移位置
 * @param len       写入长度
 * @param data      要比较的内容地址
 * @return          比较结果，true: 相等, false: 不相等
*/
extern bool mem_array_equal(mem_array_t* self, uint32_t off, uint32_t len, const void* data);

#ifdef __cplusplus
}
#endif

#endif // __MEMARRAY_H__