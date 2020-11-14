/** base64编解码单元头文件
 * @file base64.h
 * @author Kiven Lee
 * @date 2020-08-16
 * @version 1.00
 */

#pragma once
#ifndef __BASE64_H__
#define __BASE64_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 计算base64编码所需要的大小
 * @param srclen        原始内容大小
 * @param padding       是否附加'='号进行对齐
 * @param line_break    是否每76个字符换行
 * @return              编码需要的缓冲区长度
*/
extern size_t base64_encode_len(size_t srclen, bool padding, bool line_break);

/** 计算base64解码所需要的大小
 * @param src           编码内容
 * @param srclen        编码内容大小
 * @param line_break    是否每76个字符换行
 * @return              解码需要的缓冲区长度
*/
extern size_t base64_decode_len(uint8_t *src, size_t srclen);

/** 自定义base64编码
 * @param dst           写入编码内容的目标地址
 * @param src           原始内容地址
 * @param slen          原始内容长度
 * @param padding       是否添加‘=’号对其4字节
 * @param line_break    是否每76字节换行
 * @param map           用户定义的编码表
 * @return              实际编码写入的编码字节数量
*/
extern size_t base64_encode_custom(uint8_t* dst, const void* src, size_t slen, bool padding, bool line_break, const uint8_t *map);

/** 传统base64编码
 * @param dst           写入编码内容的目标地址
 * @param src           原始内容地址
 * @param slen          原始内容长度
 * @param padding       是否添加‘=’号对其4字节
 * @param line_break    是否每76字节换行
 * @return              实际编码写入的编码字节数量
*/
extern size_t base64_encode(uint8_t* dst, const void* src, size_t slen, bool padding, bool line_break);

/** URL变种base64编码
 * @param dst           写入编码内容的目标地址
 * @param src           原始内容地址
 * @param slen          原始内容长度
 * @param padding       是否添加‘=’号对其4字节
 * @param line_break    是否每76字节换行
 * @return              实际编码写入的编码字节数量
*/
extern size_t base64_url_encode(uint8_t* dst, const void* src, size_t slen, bool padding, bool line_break);

/** 自定义base64解码
 * @param dst           写入解码内容的目标地址
 * @param src           编码内容地址
 * @param slen          编码内容长度
 * @param map           用户定义的解码表
 * @return              实际解码写入的字节数量
*/
extern size_t base64_decode_custom(void* dst, const uint8_t* src, size_t slen, const uint8_t *map);

/** 传统base64解码
 * @param dst           写入解码内容的目标地址
 * @param src           编码内容地址
 * @param slen          编码内容长度
 * @return              实际解码写入的字节数量
*/
extern size_t base64_decode(void* dst, const uint8_t* src, size_t slen);

/** URL变种的base64解码
 * @param dst           写入解码内容的目标地址
 * @param src           编码内容地址
 * @param slen          编码内容长度
 * @return              实际解码写入的字节数量
*/
extern size_t base64_url_decode(void* dst, const uint8_t* src, size_t slen);

#ifdef __cplusplus
}
#endif

#endif // __BASE64_H__
