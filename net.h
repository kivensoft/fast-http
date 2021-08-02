#pragma once
#ifndef __NET_H__
#define __NET_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>

#ifdef _WIN32
#  include <winsock2.h>
#else // no _WIN32
#  include <arpa/inet.h>
#  include <unistd.h>
#endif // _WIN32

typedef struct sockaddr_in sockaddr_in_t;
typedef struct sockaddr sockaddr_t;

#ifdef _WIN32
	typedef SOCKET socket_t;
	typedef int socklen_t;

	/** 关闭socket，换一个名字是为了跨平台统一 */
#	define socket_close(fd) closesocket(fd)

	/** 初始化加载ws2_32.dll */
	static inline void socket_init() {
		WSADATA wd;
		if (WSAStartup(MAKEWORD(2, 2), &wd) != 0) {
			printf("load ws2_32.dll error!\n");
		}
	}
#else // no _WIN32
#	define socket_init() ((void)0)
#	define socket_close(fd) close(fd)
	typedef int socket_t;
#endif //_WIN32

#ifndef NET_TIME
#   ifdef _WIN32
#       define NET_TIME(t, secs) uint32_t t = 1000 * secs
#   else
#       define NET_TIME(t, secs) struct timeval t = { .tv_sec = secs, .tv_usec = 0 }
#   endif
#endif

/** 设置socket读取超时时间
 * 
 * @param socket 
 * @param seconds 超时时间，秒为单位
 * @return true: 成功, false: 失败
 */
static inline bool socket_recv_timeout(socket_t socket, uint32_t seconds) {
	NET_TIME(t, seconds);
	return !setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)(&t), sizeof(t));
}

/** 设置socket写入超时时间
 * 
 * @param socket 
 * @param seconds 超时时间，秒为单位
 * @return true: 成功, false: 失败
 */
static inline bool socket_send_timeout(socket_t socket, uint32_t seconds) {
	NET_TIME(t, seconds);
	return !setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (char*)(&t), sizeof(t));
}

/** 32位整型ip转换为字符串样式 */
static inline const char* net_ip_tostring(uint32_t ip) {
	struct in_addr a = {.s_addr = ip};
	return inet_ntoa(a);
}

/** 字符串样式的ip转为32位整数形式 */
static inline uint32_t net_ip_fromstring(const char* ip) { return inet_addr(ip); }

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
// native endian is little-endian
	static inline uint16_t net_uint16(uint16_t a) { return (a >> 8) | (a << 8); }
	static inline uint32_t net_uint32(uint32_t a) { return (a >> 24) | (a >> 8 & 0xFF00) | (a << 8 & 0xFF0000 ) | (a << 24); }
	static inline uint64_t net_uint64(uint64_t a) {
		return (a >> 56) | (a >> 40 & 0xFF00) | (a >> 24 & 0xFF0000) | (a >> 8 & 0xFF000000)
			| (a << 8 & 0xFF00000000) | (a << 24 & 0xFF0000000000) | (a << 40 & 0xFF000000000000) | (a << 56);
	}
#else
// native endian is big-endian
#   define net_uint16(a) (a)
#   define net_uint32(a) (a)
#   define net_uint64(a) (a)
#endif

#endif // __NET_H__