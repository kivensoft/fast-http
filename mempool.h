/** 内存池分配库， 初始化时创建指定大小的池，优先从池中分配内存，在池内存分配完毕后，调用系统malloc函数进行分配
 * @author kiven lee
 * @version 1.0
*/
#pragma once
#ifndef __MEMPOOL_H__
#define __MEMPOOL_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t    len;            // 内存池容量, 指定内存池总共有多少个分配项
    uint32_t    size;           // 内存池分配项大小
    uint8_t*    pointer;        // 内存池指针
    uint8_t     bits[];         // 内存池位图数组, 1表示对应索引项未被使用，0表示已被使用
} mempool_t;

/** 初始化位图
 * @param len       可分配的项数量
 * @param size      项的大小
 * @return          新的内存池对象
*/
extern mempool_t* mempool_malloc(uint32_t len, uint32_t size);

/** 释放内存池 */
extern void mempool_free(mempool_t* pool);

/** 从内存池获取可用对象 */
extern void* mempool_get(mempool_t* pool);

/** 将使用完毕的对象放回内存池
 * @param pool      内存池对象
 * @param entry     需要放回内存池的项
*/
extern void mempool_put(mempool_t* pool, void* entry);

#ifdef __cplusplus
}
#endif

#endif // __MEMPOOL_H__
