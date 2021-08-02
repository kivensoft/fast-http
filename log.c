#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "log.h"

size_t log_strcpy(char* dst, const char* src) {
	const char *osrc = src;
	while ((*dst++ = *src++));
	return src - osrc - 1;
}

size_t get_basename(char* dst, size_t dstlen, const char* filename) {
	// 处理null及空字符串情况
	if (!filename || !*filename) {
		if (dst) dst[0] = '\0';
		return 0;
	}

	// 找到文件名末尾位置，如果最后一个字符是路径分隔符，则跳过
	const char* fend = filename;
	while (*fend) ++fend;
	if (*(fend - 1) == PATH_SEP) --fend;

	// 往前查找，找到第一个路径分隔符位置后的字符位置
	const char* pos = fend - 1;
	while (pos >= filename && *pos != PATH_SEP) --pos;
	if (pos < filename) pos = filename;
	else ++pos;

	// 复制文件名到目标地址
	size_t cpylen = fend > pos ? fend - pos : 0;
	if (dst) {
		if (cpylen >= dstlen) return -1;
		if (cpylen) memcpy(dst, pos, cpylen);
		dst[cpylen] = '\0';
	}
	return cpylen;
}

size_t get_fielpath(char* dst, size_t dstlen, const char* filename) {
	if (!filename || !*filename) {
		if (dst) dst[0] = '\0';
		return 0;
	}

	const char* fend = filename;
	while (*fend) ++fend;
	if (*(fend - 1) == PATH_SEP) --fend;

	while(fend >= filename && *fend != PATH_SEP) --fend;
	if (fend < filename) fend = filename;

	// 复制文件名到目标地址
	size_t cpylen = fend - filename;
	if (dst && cpylen) {
		if (cpylen >= dstlen) return -1;
		if (cpylen) memcpy(dst, filename, cpylen);
		dst[cpylen] = '\0';
	}
	return cpylen;
}

// 条件编译语句
#ifndef NLOG

#ifdef _WIN32
#   ifndef localtime_r
#       define localtime_r(x, y) localtime_s(y, x)
#   endif
#endif

// 日志起始位置的长度, "[yyyy-MM-dd HH:mm:ss] [DEBUG] ", 共30字节
#define LOG_HEAD_LEN 30
// _HEX输出的每行长度
#define LOG_HEX_LINE  (4 + 16 * 3)

/** 当前日志级别 */
log_level_t _log_level = LOG_DEBUG;

/** 是否允许输出到控制台 */
static bool _disable_console = false;

/** 记录日志文件名, 0长度表示只记录到控制台 */
static char *_log_name = NULL;
/** 日志文件指针 */
static FILE *_log_fp = NULL;
/** 允许的最大日志文件长度, 超出将重建 */
static size_t _log_max_size = 0;
/** 当前日志文件长度 */
static size_t _log_cur_size = 0;

/** 日志输出级别对应的输出内容 */
static char _log_levels[][9] = {"[TRACE]", "[DEBUG] ", "[INFO ] ", "[WARN ] ", "[ERROR] ", "[OFF  ]"};
/** 16进制转换常量 */
static const char _HEX[] = "0123456789abcdef";

static inline bool _check_log_size() {
	return _log_fp && _log_cur_size >= _log_max_size;
}

/** 日志文件超出长度, 重命名为bak文件 */
static void _log_truncate() {
	fclose(_log_fp);
	size_t flen = strlen(_log_name);
	// 重命名
	char bak_name[flen + 5];
	strcpy(bak_name, _log_name);
	strcpy(bak_name + flen, ".bak");
	// 删除旧的日志文件
	remove(bak_name);
	// 新建日志文件
	if (!rename(_log_name, bak_name))
		_log_fp = fopen(_log_name, "wb");
}

// 程序退出时执行的函数
static void _log_deinit() {
	if (_log_fp != NULL)
		fclose(_log_fp);
	if (_log_name)
		free(_log_name);
}

// 关闭控制台输出
void log_disable_console() {
	_disable_console = true;
}

void log_start(const char* filename, size_t maxsize) {
	if (!filename) return;

	// 校验参数
	if (_log_fp != NULL) {
		printf("%s:%s:%d error, can't call again!", __FILE__, __func__, __LINE__);
		return;
	}

	// 打开或创建日志文件, 并移动指针到文件末尾
	_log_fp = fopen(filename, "rb+");
	if (_log_fp == NULL)
		_log_fp = fopen(filename, "wb");
	if (_log_fp == NULL) {
		printf("%s:%s:%d can't open log file %s\n", __FILE__, __func__, __LINE__, filename);
		return;
	}
	fseek(_log_fp, 0, SEEK_END);

	// 设置日志单元的一些全局变量的初始值
	_log_cur_size = ftell(_log_fp);
	_log_max_size = maxsize;

	// 复制文件名到本地
	size_t ls = strlen(filename) + 1;
	_log_name = malloc(ls);
	memcpy(_log_name, filename, ls);

	// 注册应用程序退出时的关闭日志文件的操作
	atexit(_log_deinit);
}

/** 往日志中写入一个字符 */
static inline void _log_putc(char c) {
	if (!_disable_console)
		fputc(c, stdout);
	if (_log_fp) {
		fputc(c, _log_fp);
		++_log_cur_size;
	}
}

/** 写入内容到日志中 */
static inline void _log_write(const void* data, size_t size) {
	if (!_disable_console)
		fwrite(data, 1, size, stdout);
	if (_log_fp) {
		fwrite(data, 1, size, _log_fp);
		_log_cur_size += size;
	}
}

/** 刷新日志文件缓存 */
static inline void _log_flush() {
	if (_log_fp) fflush(_log_fp);
}

static inline bool _check_disabled(log_level_t level) {
	return level < _log_level || (_disable_console && !_log_fp);
}

/** 写入日志头部 */
static void _log_head(log_level_t level) {
	time_t t = time(NULL);
	struct tm tm;
	localtime_r(&t, &tm);
	char buf[64];
	size_t c = strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S] ", &tm);
	memcpy(buf + c, _log_levels[level], sizeof(_log_levels[0]));
	_log_write(buf, c + sizeof(_log_levels[0]));
}

static inline void _log_write_head(log_level_t level, const char* title) {
	if (_check_log_size()) _log_truncate();
	_log_head(level);

	if (title) {
		int len = strlen(title);
		_log_write(title, len);
		_log_putc('\n');
	}
}

void log_format(log_level_t level, const char* fmt, ...) {
	if (_check_disabled(level)) return;
	_log_write_head(level, NULL);

	va_list args;
	va_start(args, fmt);

	char stack_buf[2048], *buf;
	// 计算日志内容长度, 结果加1，表示带上NULL结尾字符，到时候替换成换行符
	int len = vsnprintf(NULL, 0, fmt, args) + 1;
	// 本条目日志超过栈变量stack_buf可以容纳的大小，则从堆中分配内存
	if (len > sizeof(stack_buf)) {
		buf = malloc(len);
	} else
		buf = stack_buf;

	// 写入日志内容到缓冲区
	va_start(args, fmt);
	len = vsnprintf(buf, len, fmt, args);
	buf[len] = '\n';
	// 缓冲区内容写入日志
	_log_write(buf, len + 1);
	// 释放分配的内存
	if (buf != stack_buf) free(buf);

	_log_flush();
}

void log_hex(log_level_t level, const char *title, const void *data, size_t size) {
	if (_check_disabled(level)) return;
	_log_write_head(level, title);

	char buf[LOG_HEX_LINE]; // 一行hex显示格式所需大小
	memset(buf, ' ', LOG_HEX_LINE);
	buf[LOG_HEX_LINE - 1] = '\n';

	for (const uint8_t *p = data, *pe = data + size; p < pe;) {
		int pos = 4;
		// 按一行输出
		for (; p < pe && pos < LOG_HEX_LINE; ++p, pos += 3) {
			buf[pos] = _HEX[*p >> 4];
			buf[pos + 1] = _HEX[*p & 0xf];
		}
		buf[pos - 1] = '\n';
		_log_write(buf, pos);
	}

	_log_flush();
}

void log_text(log_level_t level, const char *title, const char *data, size_t size) {
	if (_check_disabled(level)) return;
	_log_write_head(level, title);

	_log_write(data, size);
	_log_putc('\n');
	_log_flush();
}

void log_dump(log_level_t level, const char *title, void* arg, LOG_DUMP_FUNC callback) {
	if (_check_disabled(level)) return;
	_log_write_head(level, title);

	char buf[2048];
	size_t len;
	while ((len = callback(arg, buf, sizeof(buf))))
		_log_write(buf, len);

	_log_putc('\n');
	_log_flush();
}

#endif // NLOG