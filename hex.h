/** hex编解码单元头文件
 * @file hex.h
 * @author Kiven Lee
 * @date 2020-08-13
 * @version 1.00
 */

#pragma once
#ifndef __HEX_H__
#define  __HEX_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 计算编码需要的空间大小
 * @param buflen        要转换的字节数组大小
 * @param prelen        前缀字符串长度
 * @param speclen       分隔字符串长度
 * @return              编码需要的空间大小，不包括'\0'结尾的字符
*/
inline static size_t hex_encode_len(size_t srclen, size_t prelen, size_t speclen) {
    return srclen ? (2 + prelen + speclen) * srclen - speclen : 0;
}

/** 计算解码需要的空间大小
 * @param buflen        要转换的字节数组大小
 * @param prelen        前缀字符串长度
 * @param speclen       分隔字符串长度
 * @return              解码需要的空间大小
*/
inline static size_t hex_decode_len(size_t srclen, size_t prelen, size_t speclen) {
    return srclen ? (srclen + speclen) / (2 + prelen + speclen) : 0;
}

/** hex编码
 * @param dst           编码后写入的目的地址
 * @param src           编码的源内容地址
 * @param srclen        源内容地址长度
 * @param pre           每个字节前的前缀字符串, NULL表示没有前缀字符串
 * @param spec          每个字节之间的分隔符字符串，NULL表示没有分隔符
 * @return              编码实际写入长度
*/
extern size_t hex_encode(char *dst, const void *src, size_t srclen, char *pre, char *spec);

/** hex解码
 * @param dst           解码后写入的目的地址
 * @param src           解码的源内容地址
 * @param srclen        源内容地址长度
 * @param prelen        前缀字符串长度
 * @param speclen       分隔符长度
 * @return              解码实际写入长度
*/
extern size_t hex_decode(void* dst, const char *src, size_t srclen, size_t prelen, size_t speclen);

#ifdef __cplusplus
}
#endif

#endif // __HEX_H__