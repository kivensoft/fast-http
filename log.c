#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#include "log.h"

size_t log_strcpy(char* dst, const char* src) {
	const char *osrc = src;
	while ((*dst++ = *src++));
	return src - osrc - 1;
}

// 条件编译语句
#ifndef NLOG

#ifdef _WIN32
#   define localtime_r(x, y) localtime_s(y, x)
#endif

#define LOG_HEX_LINE  (4 + 16 * 3)

/** 当前日志级别 */
log_level_t g_log_level = LOG_DEBUG;

/** 是否允许输出到控制台 */
static bool g_disable_console = false;

/** 记录日志文件名, 0长度表示只记录到控制台 */
static char *g_log_name = NULL;
/** 日志文件指针 */
static FILE *g_log_fp = NULL;
/** 允许的最大日志文件长度, 超出将重建 */
static size_t g_log_max_size = 0;
/** 当前日志文件长度 */
static size_t g_log_cur_size = 0;

/** 日志输出级别对应的输出内容 */
static char g_log_levels[][9] = { "[DEBUG] ", "[INFO ] ", "[WARN ] ", "[ERROR] " };
/** 16进制转换常量 */
static const char HEX[] = "0123456789abcdef";

/** 日志文件超出长度, 重命名为bak文件 */
static void log_truncate() {
	fclose(g_log_fp);
	size_t flen = strlen(g_log_name);
	// 重命名
	char bak_name[flen + 5];
	strcpy(bak_name, g_log_name);
	strcpy(bak_name + flen, ".bak");
	// 删除旧的日志文件
	remove(bak_name);
	// 新建日志文件
	if (!rename(g_log_name, bak_name))
		g_log_fp = fopen(g_log_name, "wb");
}

static void log_deinit() {
	if (g_log_name)
		free(g_log_name);
	if (g_log_fp != NULL) {
		fflush(g_log_fp);
		fclose(g_log_fp);
	}
}

void log_disable_console() {
	g_disable_console = true;
}

void log_start(const char* filename, size_t maxsize) {
	// 注册应用程序退出时的关闭日志文件的操作
	atexit(log_deinit);

	if (!filename) return;
	// 校验参数
	if (g_log_fp != NULL) {
		printf("%s:%s error, can't call again!", __FILE__, __func__);
		return;
	}

	// 打开或创建日志文件, 并移动指针到文件末尾
	g_log_fp = fopen(filename, "rb+");
	if (g_log_fp == NULL)
		g_log_fp = fopen(filename, "wb");
	if (g_log_fp == NULL) {
		printf("%s:%s can't open log file %s\n", __FILE__, __func__, filename);
		return;
	}
	fseek(g_log_fp, 0, SEEK_END);

	// 设置日志单元的一些全局变量的初始值
	g_log_cur_size = ftell(g_log_fp);
	g_log_max_size = maxsize;

	// 复制文件名到本地
	size_t flen = strlen(filename);
	g_log_name = malloc(flen + 1);
	strcpy(g_log_name, filename);
}

/** 往日志中写入一个字符 */
inline static void log_putc(char c) {
	if (!g_disable_console)
		fputc(c, stdout);
	if (g_log_fp) {
		fputc(c, g_log_fp);
		++g_log_cur_size;
	}
}

/** 写入内容到日志中 */
inline static void log_write(const void* data, size_t size) {
	if (!g_disable_console)
		fwrite(data, 1, size, stdout);
	if (g_log_fp) {
		fwrite(data, 1, size, g_log_fp);
		g_log_cur_size += size;
	}
}

/** 刷新日志文件缓存 */
inline static void log_flush() {
	if (!g_disable_console)
		fflush(stdout);
	if (g_log_fp)
		fflush(g_log_fp);
}

/** 写入日志头部 */
inline static void log_header(log_level_t level) {
	char buf[64];
	time_t t = time(NULL);
	struct tm tm;
	localtime_r(&t, &tm);
	size_t c = strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S] ", &tm);
	c += log_strcpy(buf + c, g_log_levels[level]);
	log_write(buf, c);
}

inline static bool check_disabled(log_level_t level) {
	return level < g_log_level || (g_disable_console && !g_log_fp);
}

void log_log(log_level_t level, const char* fmt, ...) {
	if (check_disabled(level))
		return;

	if (g_log_fp && g_log_cur_size >= g_log_max_size)
		log_truncate();

	log_header(level);

	// 写入记录头部的记录时间和日志级别
	va_list args;

	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);

	va_start(args, fmt);
	if (g_log_fp != NULL)
		g_log_cur_size = vfprintf(g_log_fp, fmt, args);
	va_end(args);

	log_putc('\n');
	log_flush();
}

void log_hex(log_level_t level, const char *title, const void *data, size_t size) {
	if (check_disabled(level))
		return;

	log_header(LOG_DEBUG);
	log_write(title, strlen(title));
	log_putc('\n');

	char buf[LOG_HEX_LINE]; // 一行hex显示格式所需大小
	memset(buf, ' ', LOG_HEX_LINE);
	buf[LOG_HEX_LINE - 1] = '\n';

	for (const uint8_t *p = data, *pe = data + size; p < pe;) {
		int pos = 4;
		// 按一行输出
		for (; p < pe && pos < LOG_HEX_LINE; ++p, pos += 3) {
			buf[pos] = HEX[*p >> 4];
			buf[pos + 1] = HEX[*p & 0xf];
		}
		buf[pos - 1] = '\n';
		log_write(buf, pos);
	}
	log_flush();
}

void log_text(log_level_t level, const char *title, const char *data, size_t size) {
	if (check_disabled(level))
		return;

	log_header(level);
	log_write(title, strlen(title));
	log_write(data, size);
	log_putc('\n');
	log_flush();
}

void log_dump(log_level_t level, const char *title, const void* arg, size_t bufsize, LOG_DUMP_FUNC func) {
	if (check_disabled(level))
		return;

	log_header(level);
	log_write(title, strlen(title));

	char buf[bufsize];
	size_t len;
	while ((len = func(arg, buf, bufsize)))
		log_write(buf, len);

	log_putc('\n');
	log_flush();
}

#endif // NLOG