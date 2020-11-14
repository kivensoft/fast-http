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
typedef struct httpctx_pool_list_t {
    list_head_t         node;
    httpctx_pool_t*     data;
} httpctx_pool_list_t;

/** uv写入函数分配的扩展自uv_write_t的对象，增加记录httpctx上下文对象指针的字段 */
typedef struct resp_write_t {
    uv_write_t          write;
    httpctx_t*          httpctx;
} resp_write_t;

static httpctx_pool_list_t httpctx_pool_list = { LIST_HEAD_INIT(httpctx_pool_list.node), NULL };

static const char RESP_ERROR[] = "HTTP/1.1 %u %s\r\nContent-Length: %u\r\nContent-Type: text/plain\r\n%s\r\n%s";
static const char RESP_STATUS[] = "HTTP/1.1 200 OK\r\nContent-Length: %u\r\n";
static const char KEEP_ALIVE[] = "Connection: keep-alive\r\n";
static const char EMPTY[] = "";
static const char CONTENT_TYPE[] = "Content-Type: ";

// 函数预声明--------
static void on_write(uv_write_t *req, int status);

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
#else
#   define log_trace_req(x) ((void)0)
#endif

/** httpctx_pool退出释放内存函数 */
static void on_httpctx_pool_destroy() {
    log_trace("on_httpctx_pool_destroy, free httpctx_pool");
    httpctx_pool_list_t *pos;
    list_for_each_ex(pos, &httpctx_pool_list) {
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

/** 写入http回复的状态信息，格式为 HTTP/1.1 200 OK
 * @param buf           写入的缓冲区
 * @param http_status   http状态码
 * @return              写入的uv_buf_t项数量
*/
static uint32_t write_error_status(httpctx_t* pctx) {
    httpres_t* res = &pctx->res;
    const char* emsg, *ka = res->keep_alive ? KEEP_ALIVE : EMPTY;
    switch(res->status) {
        case 200: emsg = "OK";                      break;
        case 401: emsg = "Unauthorized";            break;
        case 403: emsg = "Forbidden";               break;
        case 404: emsg = "Not Found";               break;
        case 500: emsg = "Internal Server Error";   break;
        default:  emsg = "Unknown";
    }

    mem_array_t* pbuf = &res->data;
    char* pbuf_base = (char*) mem_array_next(pbuf);
    int w_len = snprintf(pbuf_base, pbuf->pagesize, RESP_ERROR, res->status, emsg, (uint32_t) strlen(emsg), ka, emsg);
    mem_array_inc_len(pbuf, w_len);

    // 生成uv_tcp_write需要的数据
    uv_buf_t* bufs = (uv_buf_t*) malloc(sizeof(uv_buf_t));
    bufs[0].base = pbuf_base;
    bufs[0].len = w_len;
    // uv_buf_t数组记录到上下文，以便完成写入后可以释放这些内存
    pctx->res.write_bufs = bufs;
    return 1;
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
    int prlen = snprintf((char*) mem_array_next(pbuf), mem_array_next_surplus(pbuf), RESP_STATUS, body_len);
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
    mem_array_t* pbuf = &res->data;
    uint32_t bufs_size, write_start = mem_array_align_len(pbuf);

    // 回复状态不是200（正常），则直接回复错误信息 401/403/404/500
    if (res->status != HTTP_STATUS_OK) {
        bufs_size = write_error_status(pctx);
    } else {
        uint32_t head_size = write_http_header(pctx);
        uint32_t ps = pbuf->pagesize;
        // 计算uv_buf_t数组长度
        bufs_size = (head_size + ps - 1) / ps;
        uint32_t body_size = res->body.len;
        bufs_size += (body_size + ps - 1) / ps;

        // 生成uv_buf_t数组, 作为uv_write写入函数的数据区
        uv_buf_t* bufs = malloc(sizeof(uv_buf_t) * bufs_size);
        res->write_bufs = bufs;

        uint32_t idx = 0;
        // 处理头部内容的uv_buf_t
        idx += fill_uv_buf_ts(bufs, pbuf, write_start, head_size);
        // 处理body的uv_buf_t
        idx += fill_uv_buf_ts(bufs + idx, pbuf, res->body.pos, res->body.len);
    }

    // 将生成的回复数据用uv_write写入到客户端
    resp_write_t *wri = malloc(sizeof(resp_write_t));
    wri->httpctx = pctx;
    uv_write((uv_write_t*) wri, (uv_stream_t*) pctx, res->write_bufs, bufs_size, on_write);
}

/** 调用uv_close时的自动回调函数 */
static void on_close(uv_handle_t* handle) {
    log_trace("http on close");

    // 如果在写入时非正常关闭，清理写入数据区
    uv_buf_t* bufs = ((httpctx_t*) handle)->res.write_bufs;
    if (bufs) free(bufs);

    httpctx_free((httpctx_t*) handle);
}

/** 调用uv_write进行一次性写入时的自动回调函数 */
static void on_write(uv_write_t *req, int status) {
    log_trace("http write success, status = %d", status);

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
static void on_alloc(uv_handle_t* client, size_t suggested_size, uv_buf_t* buf) {
    log_trace("http on alloc, suggested_size = %u", (uint32_t) suggested_size);
    uint32_t out_len = 0;
    mem_array_last_page(&((httpctx_t*)client)->req.data, (uint8_t**)&buf->base, &out_len);
    buf->len = out_len;
}

/** uv客户端每次读取完成后回调的函数 */
static void on_read(uv_stream_t* uv_stream, ssize_t nread, const uv_buf_t* buf) {
    log_trace("http on read, nread = %d", (int) nread);
    httpctx_t* client = (httpctx_t*) uv_stream;
    // 处理读取错误的情况
    if (nread < 0) {
        uv_close((uv_handle_t*) client, on_close);
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
        log_trace("on_read can't read end, continue...");
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
    // httpctx_reset(client);
    mem_array_destroy(&client->req.data);
    // buffer_init(&client->req.data, BUFFER_C1);
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
        uv_read_start((uv_stream_t*) client, on_alloc, on_read);
    } else {
        log_trace("uv_accept error");
        uv_close((uv_handle_t*) client, on_close);
    }
}

bool http_server(http_server_t* pserver, const char* listen, int backlog, on_http_serve_cb callback) {
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

    // 调用前先记录链表是否为空
    int pool_empty = list_empty((list_head_t*) &httpctx_pool_list);

    // 初始化内存池, 并注册退出时释放内存池的函数
    log_trace("httpctx_pool init, header count = %u, ctx count = %u",
            (uint32_t) HS_HEAD_POOL_SIZE, (uint32_t) HS_CTX_POOL_SIZE);
    pserver->pool = httpctx_pool_malloc(HS_HEAD_POOL_SIZE, HS_CTX_POOL_SIZE);

    // 把新创建的内存池加入到待释放的内存池链表中
    httpctx_pool_list_t* plist = malloc(sizeof(httpctx_pool_list_t));
    plist->data = pserver->pool;
    list_add_tail((list_head_t*) plist, (list_head_t*) &httpctx_pool_list);

    // 第一次加入到链表中，注册退出清理函数
    if (pool_empty) {
        atexit(on_httpctx_pool_destroy);
        log_trace("register on_httpctx_pool_destroy to atexit");
    }

    // 设置服务的回调处理函数
    pserver->serve_cb = callback;

    // 创建http服务
    struct sockaddr_in addr;
    uv_tcp_init(uv_default_loop(), (uv_tcp_t*) pserver);
    uv_ip4_addr(host, port, &addr);
    uv_tcp_bind((uv_tcp_t*) pserver, (const struct sockaddr*)&addr, 0);
    int r = uv_listen((uv_stream_t*) pserver, backlog, on_new_connection);
    if (r) {
        log_error("Listen %s error: %s", listen, uv_strerror(r));
        return false;
    }
    log_info("http server listen %s\n", listen);

    return true;
}