#pragma once
#ifndef __HTTPCTX_H__
#define __HTTPCTX_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "uv.h"
#include "list.h"
#include "memarray.h"
#include "mempool.h"
#include "http_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

/** http请求解析完成的状态值 http_req_t.parser_state */
#define HTTP_PARSER_COMPLETE 100

#ifndef HI_UINT32
#   define HI_UINT32(x) ((uint32_t) ((uint64_t) x >> 32))
#endif
#ifndef LO_UINT32
#   define LO_UINT32(x) ((uint32_t) x)
#endif
#ifndef TO_UINT64
#   define TO_UINT64(hi, lo) (((uint64_t) x1 << 32) | ((uint32_t) x2))
#endif

typedef enum { HC_HTTP10, HC_HTTP11, HC_HTTP20 } hc_http_version_t ;
typedef enum { HC_HTTP_GET, HC_HTTP_POST, HC_HTTP_PUT, HC_HTTP_DELETE } hc_http_method_t;
typedef enum { HC_BODY_MEMORY, HC_BODY_CALLBACK } hc_body_type_t;

/** http服务回调处理函数, 返回0表示处理成功, 非0表示处理失败 */
typedef struct httpctx_t httpctx_t;
typedef int (*on_httpctx_serve_cb) (httpctx_t*);

/** 内存池对象 */
typedef struct httpctx_pool_t {
    mempool_t*     headers_pool;
    mempool_t*     ctx_pool;
} httpctx_pool_t;

/** 字符串对象 */
typedef struct http_value_t {
    uint32_t        pos;
    uint32_t        len;
} http_value_t;

/** http头部对象 */
typedef struct http_header_t {
    http_value_t    field;
    http_value_t    value;
} http_header_t;

/** 所有头部对象的链表集合 */
typedef struct http_headers_t {
    list_head_t     node;
    http_header_t   data;
} http_headers_t;

/** http 请求对象 */
typedef struct httpreq_t {
    uint8_t         version     : 2;    // 协议版本：hc_http_version_t: 1.0/1.1/2.0
    uint8_t         method      : 2;    // 请求类型, hc_http_method_t：GET/POST/PUT/DELETE
    uint8_t         keep_alive  : 1;    // 保持连接请求标志

    http_value_t    url;                // 请求url
    http_value_t*   host;               // 主机名，没有设置时为NULL
    http_value_t*   content_type;       // 请求内容类型，没有设置时为NULL
    http_value_t    path;               // 解析url得到的path（去除参数）
    http_value_t    param;              // 解析url得到的param
    uint32_t        content_length;     // 请求内容长度
    http_headers_t  headers;            // 请求头部字段链表
    http_value_t    body;               // 请求内容
    mem_array_t     data;               // 原始请求内容

    http_parser     parser;             // 解析器对象
    uint8_t         parser_state;       // 当前解析状态

    void*           userdata;           // 用户自定义数据，用于用户回调处理请求时链式处理的上下文传递
} httpreq_t;

/** http 回复对象 */
typedef struct httpres_t {
    uint16_t        status;             // 回复状态 200/401/403/404/500
    uint8_t         keep_alive;         // 包含保持连接的标志
    uint32_t        content_length;     // 回复内容长度，
    http_value_t    content_type;       // 回复内容类型
    http_headers_t  headers;            // 回复头部内容链表
    hc_body_type_t  body_type;          // 回复内容类型，直接指向data的内存地址/通过回调实现的分段内容
    union {
        http_value_t    body;           // 回复内容body域的值
        uint32_t (*on_body_cb) (char* buf, uint32_t len); // body回调函数
    };
    mem_array_t     data;               // 回复内容关联的缓冲区
    uv_buf_t*       write_bufs;         // 回复内容数据区数组，.base = NULL结尾，由tcp服务自行管理内存
} httpres_t;

/** http 上下文对象 */
struct httpctx_t {
    uv_tcp_t        tcp;                // tcp连接对象, 兼容 uv_tcp_t, uv_stream_t, uv_handle_t
    httpreq_t       req;                // 请求对象
    httpres_t       res;                // 回复对象
    httpctx_pool_t* pool;               // 内存池对象
    on_httpctx_serve_cb serve_cb;       // 服务回调处理函数
};

/** 创建httpctx内存池分配对象
 * @param headers_count     http_headers_t头部链表对象预分配数量
 * @param ctx_count         httpctx_t上下文对象预分配数量
 * @return                  新建的内存池对象
*/
inline static httpctx_pool_t* httpctx_pool_malloc(uint32_t headers_count, uint32_t ctx_count) {
    httpctx_pool_t* pool = malloc(sizeof(httpctx_pool_t));
    pool->headers_pool = mempool_malloc(headers_count, sizeof(http_headers_t));
    pool->ctx_pool = mempool_malloc(ctx_count, sizeof(httpctx_t));
    return pool;
}

/** 释放httpctx内存池对象
 * @param pool              内存池对象
*/
inline static void httpctx_pool_free(httpctx_pool_t* pool) {
    mempool_free(pool->headers_pool);
    mempool_free(pool->ctx_pool);
    free(pool);
}

/** 初始化httpctx内存池对象
 * @param pctx              内存池对象
*/
extern void httpctx_init(httpctx_t* pctx);

/** 分配一个新的httpctx上下文对象
 * @param pool              httpctx内存池对象
 * @param cb                http服务回调处理函数
*/
inline static httpctx_t* httpctx_malloc(httpctx_pool_t* pool, on_httpctx_serve_cb cb) {
    httpctx_t* pctx = (httpctx_t*) mempool_get(pool->ctx_pool);
    memset(pctx, 0, sizeof(httpctx_t));
    pctx->pool = pool;
    pctx->serve_cb = cb;
    httpctx_init(pctx);
    return pctx;
}

/** 释放httpctx上下文对象内部对象的内存空间，包含缓冲区和头部链表指针(不释放pctx对象本身)
 * @param pctx              httpctx上下文对象
*/
extern void httpctx_free_data(httpctx_t* pctx);

/** 释放httpctx上下文对象(连带pctx对象也被释放)
 * @param pctx              httpctx上下文对象
*/
inline static void httpctx_free(httpctx_t* pctx) {
    httpctx_free_data(pctx);
    mempool_put(pctx->pool->ctx_pool, pctx);
}

/** 重置httpctx上下文对象状态(释放内部对象占有的内存空间，其他对象初始化，适用于本次请求处理完毕后复用该对象进行下次请求处理)
 * @param pctx              httpctx上下文对象
*/
inline static void httpctx_reset(httpctx_t* pctx) {
    httpctx_free_data(pctx);
    memset(&pctx->req, 0, sizeof(httpreq_t) + sizeof(httpres_t));
    httpctx_init(pctx);
}

/** 从内存池中获取一个新的http_headers_t对象
 * @param pctx              httpctx上下文对象
 * @return                  新的http_headers_t对象
*/
inline static http_headers_t* httpctx_headers_get(httpctx_t* pctx) {
    return mempool_get(pctx->pool->headers_pool);
}

/** 将http_headers_t对象放回内存池
 * @param pctx              httpctx上下文对象
 * @param headers           要放回内存池的http_headers_t对象
*/
inline static void httpctx_headers_put(httpctx_t* pctx, http_headers_t* headers) {
    list_del((list_head_t*) headers);
    mempool_put(pctx->pool->headers_pool, headers);
}

/** 解析http协议内容(当收到新的数据时调用，分段接收时可多次调用)
 * @param pctx              httpctx上下文对象
 * @param buf               收到的数据缓冲区
 * @param len               数据缓冲区长度 
*/
extern size_t httpctx_parser_execute(httpctx_t* pctx, char* buf, size_t len);

/** 设置回复消息的内容类型
 * @param pctx              httpctx上下文对象
 * @param value             Comtent-Type值，字符串形式
*/
inline static void httpctx_set_content_type(httpctx_t* pctx, const char* value) {
    http_value_t* ct = &pctx->res.content_type;
    mem_array_t* pbuf = &pctx->res.data;
    ct->pos = mem_array_length(pbuf);
    ct->len = mem_array_append(pbuf, value, strlen(value));
}

/** 增加回复消息的头部字段
 * @param pctx              httpctx上下文对象
 * @param field             头部字段名，字符串
 * @param value             头部字段值，字符串
*/
inline static void httpctx_add_header(httpctx_t* pctx, const char* field, const char* value) {
    mem_array_t* pbuf = &pctx->res.data;
    http_headers_t* h = httpctx_headers_get(pctx);
    h->data.field.pos = mem_array_length(pbuf);
    h->data.field.len = mem_array_append(pbuf, field, strlen(field));
    h->data.value.pos = mem_array_length(pbuf);
    h->data.value.len = mem_array_append(pbuf, value, strlen(value));
    list_add_tail((list_head_t*) h, (list_head_t*) &pctx->res.headers);
}

/** 获取请求的method字符串
 * @param req               请求对象
 * @return                  请求字符串
*/
extern const char* httpctx_get_method(httpreq_t* req);

/** 路径匹配
 * @param req               请求对象
 * @param path              要比较的路径
 * @return                  true：匹配成功，false：匹配失败
*/
extern bool httpctx_pathcmp(httpreq_t* req, const char* path);

#ifdef __cplusplus
}
#endif

#endif // __HTTPCTX_H__