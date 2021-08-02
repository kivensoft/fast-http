/** 日志单元, 支持windows/linux, 适用于控制台应用程序, 如果定义了NLOG, 将禁用所有日志单元的代码
 * 
 * @file log.h
 * @author Kiven Lee
 * @date 2018-04-03
 * @version 1.02
 */

#pragma once
#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#   define PATH_SEP '\\'
#else
#   define PATH_SEP '/'
#endif

// 日志级别宏定义
typedef enum { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_OFF } log_level_t;

/** log_dump函数回调接口, 回调函数负责写入buf, 并返回写入长度
 * 
 * @param arg 回调函数时传递的变量
 * @param dst 回写缓冲区地址
 * @param dstlen 回写缓冲区长度
 * 
eturn 回调函数写入的数据长度，0表示没有再多的数据
*/
typedef size_t (*LOG_DUMP_FUNC) (void* arg, char* dst, size_t dstlen);

/** 自定义字符串复制功能，返回值是复制的长度
 * 
 * @param dst       复制的目的地址
 * @param src       复制的源地址
 * @return          返回复制的字符串长度
*/
extern size_t log_strcpy(char* dst, const char* src);

/** 获取文件的名字，去掉路径, dst为NULL时，只计算文件名长度, 返回-1表示失败, 其它值表示成功 */
extern size_t get_basename(char* dst, size_t dstlen, const char* filename);

/** 获取文件的路径，去掉文件名, dst为NULL时，只计算路径长度, 返回-1表示失败, 其它值表示成功 */
extern size_t get_fielpath(char* dst, size_t dstlen, const char* filename);

#ifdef NLOG

#define log_set_level(...) ((void)0)
#define log_is_enabled(...) 0
#define log_is_trace_enabled(X) 0
#define log_is_debug_enabled(X) 0
#define log_disable_console(...) ((void)0)
#define log_start(...) ((void)0)
#define log_trace(...) ((void)0)
#define log_debug(...) ((void)0)
#define log_info(...) ((void)0)
#define log_warn(...) ((void)0)
#define log_error(...) ((void)0)
#define log_format(...) ((void)0)
#define log_hex(...) ((void)0)
#define log_text(...) ((void)0)
#define log_dump(...) ((void)0)

#else // !NLOG

extern log_level_t _log_level;

/** 获取日志级别
 * 
 * @param level         日志级别, trace/debug/info/warn/error/off
*/
static inline int log_get_level(const char* level) {
	if (level) {
		switch (*level) {
			case 't': return LOG_TRACE; break;
			case 'd': return LOG_DEBUG; break;
			case 'i': return LOG_INFO; break;
			case 'w': return LOG_WARN; break;
			case 'e': return LOG_ERROR; break;
			case 'o': return LOG_OFF; break;
		}
	}
	return LOG_DEBUG;
}

/** 设置日志级别, 支持运行期间动态设置
 * 
 * @param level         日志级别 LOG_DEBUG/LOG_INFO/LOG_WARN/LOG_ERROR/LOG_OFF
*/
static inline void log_set_level(log_level_t level) {
	_log_level = level;
}

/** 判断指定的日志级别是否被允许
 * 
 * @param level         日志级别
 * @return              true: 允许, false: 不允许
*/
static inline bool log_is_enabled(log_level_t level) {
	return level >= _log_level;
}

/** 返回是否允许trace级别日志的值, 0: 不允许, 1: 允许 */
static inline bool log_is_trace_enabled() {
	return LOG_TRACE >= _log_level;
}

/** 返回是否允许debug级别日志的值, 0: 不允许, 1: 允许 */
static inline bool log_is_debug_enabled() {
	return LOG_DEBUG >= _log_level;
}

/** 禁用控制台日志输出 */
extern void log_disable_console();

/** 设置日志服务，注册退出清理已分配内存函数
 * 
 * @param filename          日志文件名称, 为NULL时不记录到日志文件中，只在控制台显示
 * @param maxsize           日志文件允许的最大长度, 超过将备份当前日志文件, 并新建日志文件进行记录
*/
extern void log_start(const char* filename, size_t maxsize);

/** 记录debug级别日志
 * 
 * @param fmt               格式化字符串, 使用与printf同样的格式
 * @param ...               格式化参数
 */
#define log_trace(fmt, ...) log_format(LOG_TRACE, fmt, ##__VA_ARGS__)

/** 记录debug级别日志
 * 
 * @param fmt               格式化字符串, 使用与printf同样的格式
 * @param ...               格式化参数
 */
#define log_debug(fmt, ...) log_format(LOG_DEBUG, fmt, ##__VA_ARGS__)

/** 记录info级别日志
 * 
 * @param fmt               格式化字符串, 使用与printf同样的格式
 * @param ...               格式化参数
 */
#define log_info(fmt, ...) log_format(LOG_INFO, fmt, ##__VA_ARGS__)

/** 记录warn级别日志
 * 
 * @param fmt               格式化字符串, 使用与printf同样的格式
 * @param ...               格式化参数
 */
#define log_warn(fmt, ...) log_format(LOG_WARN, fmt, ##__VA_ARGS__)

/** 记录error级别日志
 * 
 * @param fmt               格式化字符串, 使用与printf同样的格式
 * @param ...               格式化参数
 */
#define log_error(fmt, ...) log_format(LOG_ERROR, fmt, ##__VA_ARGS__)

/** 记录指定级别的日志, 会根据当前系统设置的级别判断是否需要记录
 * 
 * @param level             日志级别 LOG_DEBUG/LOG_INFO/LOG_WARN/LOG_ERROR
 * @param fmt               格式化字符串, 使用与printf同样的格式
 * @param ...               格式化参数
 */
extern void log_format(log_level_t level, const char* fmt, ...);

/** 将data的内容以hex方式输出到日志中, 方便调试
 * 
 * @param title             转储标题
 * @param data              要转储的内容
 * @param size              要转储的内容长度, 字节为单位
 */
extern void log_hex(log_level_t level, const char *title, const void *data, size_t size);

/** 将data的内容以text方式输出到日志中, 方便调试
 * 
 * @param title             转储标/
 * @param data              要转储的内容
 * @param size              要转储的内容长度, 字节为单位
 */
extern void log_text(log_level_t level, const char *title, const char *data, size_t size);

/** 将data的内容以回调函数方式输出到日志中, 方便调试
 * 
 * @param title             转储标题
 * @param arg               调用回调函数时传递的参数
 * @param bufsize           回调函数使用的目标缓冲区大小
 * @param func              回调函数
 */
extern void log_dump(log_level_t level, const char *title, void* arg, LOG_DUMP_FUNC callback);

#endif // NLOG

#ifdef __cplusplus
}
#endif

#endif // __LOG_H__