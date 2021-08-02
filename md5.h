/** md5编码单元头文件
 * @file md5.h
 * @author Kiven Lee
 * @date 2019-06-22
 * @version 1.10
*/

#pragma once
#ifndef __MD5_H__
#define __MD5_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /** state (ABCD) 四个32bits数，用于存放最终计算得到的消息摘要。当消息长度〉512bits时，也用于存放每个512bits的中间结果 */
    uint32_t state[4];

    /** number of bits, modulo 2^64 (lsb first) 
     * 存储原始信息的bits数长度,不包括填充的bits，最长为 2^64 bits，因为2^64是一个64位数的最大值
     */
    uint32_t count[2];

    /** input buffer 存放输入的信息的缓冲区，512bits */
    uint8_t buffer[64];
} md5_ctx_t;

/** 初始化MD5结构
 * @param md5           md5结构
*/
void md5_init(md5_ctx_t* md5);

/** 计算MD5值，可多次调用进行分段计算
 * @param md5           md5的结构
 * @param input         输入内容
 * @param len           输入内容长度
*/
void md5_update(md5_ctx_t* md5, const void *input, size_t len);

/** 输出MD5结果，完成计算
 * @param md5           md5的结构
 * @param digest        输出结果的地址
*/
void md5_final(md5_ctx_t* md5, uint8_t digest[16]);

/** 计算md5值的快捷函数, 无需中间众多的转换步骤
 * @param dst           写入md5结果的地址
 * @param input         要计算的聂荣地址
 * @param len           要计算的长度(字节为单位)
 * @return              返回dst
 */
uint8_t* md5_bin(uint8_t dst[16], const void *input, size_t len);

/** 计算md5值的快捷函数, 无需中间众多的转换步骤
 * @param dst           写入md5结果的地址
 * @param input         要计算的聂荣地址
 * @param len           要计算的长度(字节为单位)
 * @return              返回dst
 */
char* md5_string(char dst[33], const void *input, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __MD5_H__ */
