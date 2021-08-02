#ifndef NDEBUG
#   define NDEBUG      // 禁用log_trace调试输出，如需要调试，自行注释掉该句
#endif

#include "uv.h"
#include "httpserver.h"
#include "http_parser.h"
#include "log.h"
#include "list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =====用户定义编译的常量=====
// httpctx_t上 下文对象内存池大小
#ifndef HS_CTX_POOL_SIZE
#   define HS_CTX_POOL_SIZE 32
#endif
// http_headers_t 头部对象内存池大小
#ifndef HS_HEAD_POOL_SIZE
#   define HS_HEAD_POOL_SIZE   (HS_CTX_POOL_SIZE * 8)
#endif

/** 所有http服务使用的内存池，用于退出时集中释放 */
typedef struct _pool_list_node_t {
	list_head_t         node;
	httpctx_pool_t*     data;
} _pool_list_node_t;

/** uv写入函数分配的扩展自uv_write_t的对象，增加记录httpctx上下文对象指针的字段 */
typedef struct resp_write_t {
	uv_write_t          write;
	httpctx_t*          httpctx;
} resp_write_t;

static LIST_HEAD(httpctx_pool_list);
static _Bool httpctx_pool_destroy_registered = 0;

static const char RESP_STATUS[] = "HTTP/1.1 %u %s\r\nServer: khs/0.50\r\nContent-Length: %u\r\n";
static const char KEEP_ALIVE[] = "Connection: keep-alive\r\n";
static const char CONTENT_TYPE[] = "Content-Type: ";

// 函数预声明--------
static void on_writed(uv_write_t *req, int status);

#ifndef NDEBUG
static void log_trace_req(httpctx_t* pctx) {
	httpreq_t* preq = &pctx->req;
	mem_array_t* preqbuf = &preq->data;
	http_value_t* http_val;

	int buf_len = 32 + preq->url.len;

	http_headers_t *pos, *head = &preq->headers;
	list_for_each_ex(pos, head) {
		buf_len += pos->data.field.len + pos->data.value.len + 4;
	}
	
	char buf[buf_len], *p = buf;
	const char* method = httpctx_get_method(preq);
	int m_len = strlen(method);
	memcpy(p, method, m_len);
	p[m_len] = ' ';
	p += m_len + 1;
	p += mem_array_read(preqbuf, preq->url.pos, preq->url.len, p);
	*p++ = '\n';

	list_for_each_ex(pos, head) {
		http_val = &pos->data.field;
		p += mem_array_read(preqbuf, http_val->pos, http_val->len, p);
		*p++ = ':';
		*p++ = ' ';
		http_val = &pos->data.value;
		p += mem_array_read(preqbuf, http_val->pos, http_val->len, p);
		*p++ = '\n';
	}
	*p = '\0';

	log_trace("-------- http header info --------\n%s----------------------------------------------------------------", buf);
}

static void log_write_status(uv_write_t *req, int status) {
	uv_buf_t* pbufs = ((resp_write_t*) req)->httpctx->res.write_bufs;
	int count = 0;
	for (int i = 0; pbufs[i].base; ++i)
		count += pbufs[i].len;
	log_trace("http write success, size = %d, status = %d", count, status);
}

#else
#   define log_trace_req(x) ((void)0)
#   define log_write_status(x, y) ((void)0)
#endif

/** httpctx_pool退出释放内存函数 */
static void on_httpctx_pool_destroy() {
	log_trace("on_httpctx_pool_destroy, free httpctx_pool");
	_pool_list_node_t *pos;
	// 反向遍历，最后分配的最先释放，有利于内存合并
	list_for_reverse_each_ex(pos, &httpctx_pool_list) {
		httpctx_pool_free(pos->data);
		free(pos);
	}
}

/** 将缓冲区中的基于http_value_t指定的内容复制到缓冲区的末尾
 * @param pbuf          缓冲区对象
 * @param src           要复制的源，内容来自源指向的内容
*/
static void mem_array_copyfrom(mem_array_t* pbuf, http_value_t* src) {
	uint32_t slen = src->len;
	if (!slen) return;
	char tmp[slen];
	mem_array_read(pbuf, src->pos, slen, tmp);
	mem_array_append(pbuf, tmp, slen);
}

/** 填充uv_buf_t数组
 * @param bufs          写入的uv_buf_t数组地址
 * @param pbuf          缓冲区对象
 * @param start         缓冲区对象起始位置
 * @param size          缓冲区对象大小
*/
static uint32_t fill_uv_buf_ts(uv_buf_t* bufs, mem_array_t* pbuf, uint32_t offset, uint32_t len) {
	uv_buf_t* p = bufs;
	for (uint32_t omax = offset + len; offset < omax; ++p) {
		p->base = (char*) mem_array_get(pbuf, offset);
		uint32_t max_len = mem_array_surplus(pbuf, offset);
		p->len = len > max_len ? max_len : len;
		offset += p->len;
	}
	return p - bufs;
}

/** 获取http状态码对应的文本消息 */
static char* get_message_by_status(uint16_t status) {
	switch(status) {
		case 200: return "OK";                      // 请求成功。一般用于GET与POST请求
		case 201: return "Created";                 // 已创建。成功请求并创建了新的资源
		case 204: return "No Content";              // 无内容。服务器成功处理，但未返回内容。在未更新网页的情况下，可确保浏览器继续显示当前文档
		case 301: return "Moved Permanently";       // 永久移动。请求的资源已被永久的移动到新URI，返回信息会包括新的URI，浏览器会自动定向到新URI。今后任何新的请求都应使用新的URI代替
		case 302: return "Found";                   // 临时移动。与301类似。但资源只是临时被移动。客户端应继续使用原有URI
		case 304: return "Not Modified";            // 未修改。所请求的资源未修改，服务器返回此状态码时，不会返回任何资源。客户端通常会缓存访问过的资源，通过提供一个头信息指出客户端希望只返回在指定日期之后修改的资源
		case 400: return "Bad Request";             // 客户端请求的语法错误，服务器无法理解
		case 401: return "Unauthorized";            // 请求要求用户的身份认证
		case 403: return "Forbidden";               // 服务器理解请求客户端的请求，但是拒绝执行此请求
		case 404: return "Not Found";               // 服务器无法根据客户端的请求找到资源（网页）。通过此代码，网站设计人员可设置"您所请求的资源无法找到"的个性页面
		case 405: return "Method Not Allowed";      // 客户端请求中的方法被禁止
		case 500: return "Internal Server Error";   // 服务器内部错误，无法完成请求
		default:  return "Unknown";
	}
}

/** 写入正常消息的http头部信息
 * @param pctx          httpctx上下文
 * @return              写入的头部内容长度
*/
static uint32_t write_http_header(httpctx_t* pctx) {
	httpres_t* res = &pctx->res;
	mem_array_t* pbuf = &res->data;
	uint32_t write_start = mem_array_length(pbuf);

	uint32_t body_len = res->body_type ? res->content_length : res->body.len;

	// 生成http回复状态消息和消息长度
	char* status_message = get_message_by_status(res->status);
	int prlen = snprintf((char*) mem_array_next(pbuf), mem_array_next_surplus(pbuf), RESP_STATUS, res->status, status_message, body_len);
	mem_array_inc_len(pbuf, prlen);

	// 如果需要保持连接，写入保持连接的头部
	if (res->keep_alive)
		mem_array_append(pbuf, KEEP_ALIVE, sizeof(KEEP_ALIVE) - 1);
	// 如果content_type类型有设置，设置content_type
	if (res->content_type.len) {
		mem_array_append(pbuf, CONTENT_TYPE, sizeof(CONTENT_TYPE) - 1);
		mem_array_copyfrom(pbuf, &res->content_type);
		mem_array_append(pbuf, "\r\n", 2);
	}

	// 处理其他头部字段
	http_headers_t* hpos;
	list_for_each_ex(hpos, &res->headers) {
		http_header_t* p = &hpos->data;
		mem_array_copyfrom(pbuf, &p->field);
		mem_array_append(pbuf, ": ", 2);
		mem_array_copyfrom(pbuf, &p->value);
		mem_array_append(pbuf, "\r\n", 2);
	}

	mem_array_append(pbuf, "\r\n", 2);

	return mem_array_length(pbuf) - write_start;
}

// 向客户端写入回复内容
static void write_http_resp(httpctx_t* pctx) {
	httpres_t* res = &pctx->res;
	mem_array_t* pres_data = &res->data;
	uint32_t write_start = mem_array_align_len(pres_data);

	// 写入回复头部信息
	uint32_t head_size = write_http_header(pctx);
	// 计算uv_buf_t数组长度: header长度 + body长度
	uint32_t body_size = res->body.len;
	uint32_t ps = pres_data->pagesize;
	uint32_t bufs_size = (head_size + ps - 1) / ps + (body_size + ps - 1) / ps;

	// 生成uv_buf_t数组, 作为uv_write写入函数的数据区
	uv_buf_t* bufs = malloc(sizeof(uv_buf_t) * (bufs_size + 1));
	bufs[bufs_size].base = NULL;
	res->write_bufs = bufs;

	uint32_t idx = 0;
	// 处理头部内容的uv_buf_t
	idx += fill_uv_buf_ts(bufs, pres_data, write_start, head_size);
	// 处理body的uv_buf_t
	idx += fill_uv_buf_ts(bufs + idx, pres_data, res->body.pos, res->body.len);

	// 将生成的回复数据用uv_write写入到客户端
	resp_write_t *wri = malloc(sizeof(resp_write_t));
	wri->httpctx = pctx;
	uv_write((uv_write_t*) wri, (uv_stream_t*) pctx, res->write_bufs, bufs_size, on_writed);
}

/** 调用uv_close时的自动回调函数 */
static void on_closed(uv_handle_t* handle) {
	log_trace("http on close");

	// 如果在写入时非正常关闭，清理写入数据区
	uv_buf_t* bufs = ((httpctx_t*) handle)->res.write_bufs;
	if (bufs) free(bufs);

	httpctx_free((httpctx_t*) handle);
}

/** 调用uv_write进行一次性写入时的自动回调函数 */
static void on_writed(uv_write_t *req, int status) {
	log_write_status(req, status);

	// 写入完成后释放内存
	uv_buf_t** pbufs = &((resp_write_t*) req)->httpctx->res.write_bufs;
	if (*pbufs) free(*pbufs);
	*pbufs = NULL;

	// 重置httpctx上下文对象，为下一次读取做准备
	httpctx_reset(((resp_write_t*) req)->httpctx);

	// 释放为写入申请的resp_write_t类型的对象
	free(req);
}

/** uv每次读取客户端数据前回调的内存分配函数 */
static void on_allocing(uv_handle_t* client, size_t suggested_size, uv_buf_t* buf) {
	log_trace("http on alloc, suggested_size = %u", (uint32_t) suggested_size);
	uint32_t out_len = 0;
	mem_array_last_page(&((httpctx_t*)client)->req.data, (uint8_t**)&buf->base, &out_len);
	buf->len = out_len;
}

/** uv客户端每次读取完成后回调的函数 */
static void on_readed(uv_stream_t* uv_stream, ssize_t nread, const uv_buf_t* buf) {
	log_trace("http on read, nread = %d", (int) nread);
	httpctx_t* client = (httpctx_t*) uv_stream;
	// 处理读取错误的情况
	if (nread < 0) {
		uv_close((uv_handle_t*) client, on_closed);
		return;
	} else if (nread == 0) {
		return;
	}

	// 对收到的数据进行解析
	httpctx_parser_execute(client, buf->base, buf->len);
	// 设置读取缓冲区的当前长度
	mem_array_inc_len(&client->req.data, buf->len);

	// 读取的请求数据尚未结束
	if (client->req.parser_state != HTTP_PARSER_COMPLETE) {
		log_trace("on_readed can't read end, continue...");
		return;
	}

	// 输出调试信息
	log_trace_req(client);

	// 调用回调函数进行处理
	client->serve_cb(client);
	// 向客户端写入回复信息
	write_http_resp(client);

	// 写入是异步操作，这里为了充分利用内存，先行将请求对象占用的内存进行释放
	// 回复对象占用的内存及其它小内存占用，等到写入完成再释放
	mem_array_destroy(&client->req.data);
}

/** http服务每次有新连接时的回调函数 */
static void on_new_connection(uv_stream_t* server, int status) {
	log_trace("http on new connection");
	if (status < 0) {
		log_info("http new connection error: %s", uv_strerror(status));
		return;
	}

	httpctx_t* client = httpctx_malloc(((http_server_t*) server)->pool, ((http_server_t*) server)->serve_cb);
	// uv_tcp_t *client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
	uv_tcp_init(uv_default_loop(), (uv_tcp_t*) client);
	if (uv_accept(server, (uv_stream_t*) client) == 0) {
		log_trace("http connection ok");
		uv_read_start((uv_stream_t*) client, on_allocing, on_readed);
	} else {
		log_trace("uv_accept error");
		uv_close((uv_handle_t*) client, on_closed);
	}
}

bool http_server(uv_loop_t* puv_loop, http_server_t* pserver, const char* listen, int backlog, on_http_serve_cb callback) {
	// 解析监听地址, host:port 格式
	size_t len = strlen(listen);
	char host[len + 1];
	strcpy(host, listen);
	char *port_str = strchr(host, ':');
	if (!port_str) {
		log_error("listen address is invalid: %s", listen);
		return false;
	}
	*port_str++ = '\0';
	int port = atoi(port_str);

	// 首次运行，注册退出清理函数
	if (!httpctx_pool_destroy_registered) atexit(on_httpctx_pool_destroy);

	// 初始化服务关联的上下文内存池
	pserver->pool = httpctx_pool_malloc(HS_HEAD_POOL_SIZE, HS_CTX_POOL_SIZE);
	// 设置服务的回调处理函数
	pserver->serve_cb = callback;

	// 把新创建的内存池加入到待释放的内存池链表中
	_pool_list_node_t* plist = malloc(sizeof(_pool_list_node_t));
	plist->data = pserver->pool;
	list_add_tail(&plist->node, &httpctx_pool_list);

	// 创建http服务
	struct sockaddr_in addr;
	if (puv_loop == NULL)
		puv_loop = uv_default_loop();
	uv_tcp_init(puv_loop, (uv_tcp_t*) pserver);
	uv_ip4_addr(host, port, &addr);

	// 侦听新的TCP连接时，启用/禁用操作系统排队的同步异步接受请求
	// uv_tcp_simultaneous_accepts((uv_tcp_t*) pserver, 1);
	uv_tcp_bind((uv_tcp_t*) pserver, (const struct sockaddr*)&addr, 0);
	int r = uv_listen((uv_stream_t*) pserver, backlog, on_new_connection);
	if (r) {
		log_error("Listen %s error: %s", listen, uv_strerror(r));
		return false;
	}
	log_info("http server listen %s\n", listen);

	return true;
}

static int http_static_serve(httpctx_t* ctx) {
	//
}

static int http_default_serve(httpctx_t* ctx) {
	//
}

static int http_route_serve(httpctx_t* ctx) {
	//
}

bool http_service(uv_loop_t* puv_loop, http_route_t* route, const char* listen, int backlog) {
	return http_server(puv_loop, &route->http_server, listen, backlog, http_route_serve);
}

void http_route_init(http_route_t* route) {
	memset(route, 0, sizeof(route));
	route->static_func = http_static_serve;
	route->default_func = http_default_serve;
}

#define NODE_LEN(len) (sizeof(http_route_node_t) + len + 1)

static int _route_cmp(const void* v1, const void* v2) {
	return strcmp(((http_route_node_t*)v1)->path, ((http_route_node_t*)v2)->path);
}

inline static void _path_copy(char* dst, const char* src, uint32_t len) {
	strcpy(dst, src);
	if (dst[len - 1] != '/') {
		dst[len] = '/';
		dst[len + 1] = '\0';
	}
}

inline static http_route_node_t* _get_route(rb_root_t* root, const char* str, uint32_t len) {
	http_route_node_t tmp;
	tmp.path = str;
	http_route_node_t* node = rb_search(root, &tmp, _route_cmp);
	if (!node) {
		uint32_t node_size = sizeof(http_route_node_t) + len + 1;
		node = (http_route_node_t*) malloc(node_size);
		memset(node, 0, node_size);
		node->plen = len;
		node->path = (char*)node + sizeof(http_route_node_t);
		strcpy(node->path, str);
		rb_insert(root, node, _route_cmp);
	}
	return node;
}

_Bool http_route_add(const http_route_t* self, const fast_string_t* path, on_http_serve_cb func) {
	uint32_t len = path->len;
	char tmp_path[len + 2];
	_path_copy(tmp_path, path->data, len);

	char *start = tmp_path, *pos = tmp_path;
	rb_root_t* root = &self->tree;
	http_route_node_t* node;
	for (; *pos; ++pos) {
		if (*pos == '/') {
			*pos = '\0';
			node = _get_route(root, start, pos - start);
			root = &node->tree;
			start = pos + 1;
		}
	}

	if (node->func) return 0;
	node->func = func;
	return 1;
}

_Bool http_route_del(const http_route_t* self, const fast_string_t* path) {
	uint32_t len = path->len;
	char tmp_path[len + 2];
	_path_copy(tmp_path, path->data, len);

	char *start = tmp_path, *pos = tmp_path;
	rb_root_t* root = &self->tree;
	http_route_node_t *node, tmp;
	for (; *pos; ++pos) {
		if (*pos == '/') {
			*pos = '\0';
			tmp.path = start;
			node = rb_search(root, &tmp, _route_cmp);
			if (!node) return 0;
			root = &node->tree;
			start = pos + 1;
		}
	}

	if (!node->func) return 0;

	node->func = NULL;
	// TODO: 删除要考虑上级无效节点的级联删除
	while (!node->func) {
		rb_erase(node, )
	}
	return 1;
}

void http_route_match(const http_route_t* self, const char* path) {
	for (rb_node_t *node = root->rb_node; node;) {
		int cmp_ret = keycmp((const void*)key + sizeof(rb_node_t), (const void*)node + sizeof(rb_node_t));

		if (cmp_ret < 0)
			node = node->rb_left;

		else if (cmp_ret > 0)
			node = node->rb_right;
		else
			return node;
	}
	return NULL;
}