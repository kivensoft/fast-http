/** 内存池分配库， 初始化时创建指定大小的池，优先从池中分配内存，在池内存分配完毕后，调用系统malloc函数进行分配
 * @author kiven lee
 * @version 1.0
*/
#pragma once
#ifndef __POOL_H__
#define __POOL_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _pool_head_t *pool_t;

/** 初始化位图
 * @param capacity      可分配的项数量
 * @param alloc_size    项的大小
 * @return              新的内存池对象
*/
extern pool_t pool_malloc(uint32_t capacity, uint32_t alloc_size);

/** 释放内存池 */
extern void pool_free(pool_t self);

/** 从内存池获取可用对象, 内存池使用满了之后，返回NULL */
extern void* pool_tryget(pool_t self);

/** 从内存池获取可用对象, 内存池使用满了后，使用malloc进行分配 */
extern void* pool_get(pool_t self);

/** 将使用完毕的对象放回内存池
 * @param self          内存池对象
 * @param entry         需要放回内存池的项
*/
extern void pool_put(pool_t self, void* entry);

#ifdef __cplusplus
}
#endif

#endif // __POOL_H__
