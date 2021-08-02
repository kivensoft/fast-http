#pragma once
#ifndef __AIDB_H__
#define __AIDB_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 错误代码宏定义
typedef enum {
    AIDB_OK,            // 没有错误
    AIDB_ERR_OPEN,      // 无法打开文件
    AIDB_ERR_TOO_SMALL, // 文件长度太小
    AIDB_ERR_MAGIC,     // 魔法值不正确
    AIDB_ERR_VERSION,   // 版本号错误
    AIDB_ERR_WRITE,     // 写入文件错误
    AIDB_ERR_BLOCK,    // 文件格式错误, 包括区块指向的下一区块不存在或已被删除
} AIDB_ERROR;

// 内部实现的结构，隐藏实现细节
// extern struct aidb_t;
// extern struct aidb_iterator_t;

// 定义aidb_t类型
typedef struct aidb_t aidb_t;

// 定义aidb迭代器
typedef struct aidb_iterator_t aidb_iterator_t;

/** 获取aidb_t句柄内部结构的长度(字节为单位), 以便于用户自行分配aidb_t类型的内存 */
extern size_t aidb_sizeof();

/** 获取aidb_iterator_t迭代器内部结构的长度(字节为单位), 以便于用户自行分配aidb_iterator_t类型的内存 */
extern size_t aidb_iterator_sizeof();

/** 创建aidb数据库
 * @param aidb      用户分配的aidb句柄地址
 * @param filename  数据库文件名
 * @param key       秘钥 
 * @return          成功返回AIDB_OK, 失败返回错误代码, 参见AIDB_ERROR定义
*/
extern AIDB_ERROR aidb_create(aidb_t* aidb, const char* filename, const char* key);

/** 打开aidb数据库
 * @param aidb      用户分配的aidb句柄地址
 * @param filename  数据库文件名
 * @param key       秘钥 
 * @return          成功返回AIDB_OK, 失败返回错误代码, 参见AIDB_ERROR定义
*/
extern AIDB_ERROR aidb_open(aidb_t* aidb, const char* filename, const char* key);

/** 关闭aidb数据库
 * @param aidb      aidb句柄
*/
extern void aidb_close(aidb_t* aidb);

/** 缓存内容立即写入数据库
 * @param aidb      aidb句柄
*/
extern void aidb_flush(aidb_t* aidb);

/** 获取记录总数
 * @param aidb      aidb句柄
 * @return 记录数量
*/
extern uint32_t aidb_record_count(aidb_t* aidb);

/** 初始化aidb_iterator_t迭代器，准备开始循环读取记录
 * @param aidb      aidb句柄
 * @param iterator   记录结构指针, 操作将初始化该指针的内容, 为后续的循环做好准备
*/
extern void aidb_foreach(aidb_t* aidb, aidb_iterator_t* iterator);

/** 记录跳转至下一条, 并返回下一条记录的长度
 * @param iterator   记录结构指针
 * @return          下一条记录长度, 返回0表示到了末尾
*/
extern uint32_t aidb_next(aidb_iterator_t* iterator);

/** 记录跳转至上一条, 并返回上一条记录的长度
 * @param iterator   记录结构指针
 * @return          上一条记录长度, 返回0表示到了头部
*/
extern uint32_t aidb_prev(aidb_iterator_t* iterator);

/** 加载当前记录内容
 * @param iterator   记录结构指针
 * @param output    读取内容写入的地址
 * @param size      output的大小, 如果记录内容比size大, 最多只读取size个字节
 * @return          实际读取的字节数
*/
extern uint32_t aidb_fetch(aidb_iterator_t* iterator, void* output, uint32_t size);

/** 写入一条记录
 * @param aidb      aidb句柄
 * @param data      记录内容
 * @param size      记录长度(字节为单位)
*/
extern void aidb_put(aidb_t* aidb, const void* data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif // __AIDB_H__