#pragma once
#ifndef __HTTPCTX_H__
#define __HTTPCTX_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "list.h"
#include "dynmem.h"
#include "pool.h"
#include "str.h"
#include "http_parser.h"
#include "uv.h"

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

typedef enum { HC_HTTP10, HC_HTTP11, HC_HTTP20 } hc_http_version_t; // HTTP 版本枚举
typedef enum { HC_HTTP_GET, HC_HTTP_POST, HC_HTTP_PUT, HC_HTTP_DELETE } hc_http_method_t; // HTTP请求类型枚举
typedef enum { HC_BODY_MEMORY, HC_BODY_CALLBACK } hc_body_type_t; // 回调函数生成body的类型

typedef struct httpctx_t httpctx_t;

// http服务回调处理函数, 返回0表示处理成功, 非0表示处理失败
typedef int (*on_httpctx_serve_cb) (httpctx_t*);

// 内存池对象, 每个独立的http服务创建1个
typedef struct httpctx_pool_t {
    pool_t     headers_pool;            // 头部对象池，用于设置头部内容时，从池中分配
    pool_t     ctx_pool;                // 请求上下文对象池, 每次新连接可从池中分配1个上下文对象
} httpctx_pool_t;

// 字符串对象
typedef struct http_value_t {
    uint32_t        pos;                // 相对于内部缓冲区的位置
    uint32_t        len;                // 长度
} http_value_t;

// http头部对象
typedef struct http_header_t {
    http_value_t    field;              // header键名, 例如:Content-Type, Accept等
    http_value_t    value;              // header键值, 例如: text/html, www.baidu.com
} http_header_t;

// 头部对象链表节点定义
typedef struct http_header_node_t {
    LIST_FIELDS;                        // 链表节点结构，包含前节点, 后节点
    http_header_t   data;               // 自定义的节点数据
} http_header_node_t;

// http 请求对象
typedef struct httpreq_t {
    http_value_t    url;                // 请求url
    http_value_t*   host;               // 主机名，没有设置时为NULL
    http_value_t*   content_type;       // 请求内容类型，没有设置时为NULL
    http_value_t    path;               // 解析url得到的path（去除参数）
    http_value_t    url_param;          // 解析url得到的param
    uint32_t        content_length;     // 请求内容长度
    list_head_t     headers;            // 请求头部字段链表, 指向http_header_node_t结构
    http_value_t    body;               // 请求内容
    dynmem_t        data;               // 原始请求内容

    http_parser     parser;             // 解析器对象
    uint8_t         parser_state;       // 当前解析状态

    uint8_t         version     : 2;    // 协议版本：hc_http_version_t: 1.0/1.1/2.0
    uint8_t         method      : 2;    // 请求类型, hc_http_method_t：GET/POST/PUT/DELETE
    uint8_t         keep_alive  : 1;    // 保持连接请求标志

    void*           userdata;           // 用户自定义数据，用于用户回调处理请求时链式处理的上下文传递
} httpreq_t;

// http 回复对象
typedef struct httpres_t {
    uint16_t        status;             // 回复状态 200/401/403/404/500
    uint8_t         keep_alive;         // 包含保持连接的标志
    uint32_t        content_length;     // 回复内容长度，
    http_value_t    content_type;       // 回复内容类型
    list_head_t     headers;            // 回复头部内容链表, 指向http_header_node_t结构
    hc_body_type_t  body_type;          // 回复内容类型，直接指向data的内存地址/通过回调实现的分段内容
    union {
        http_value_t    body;           // 回复内容body域的值
        uint32_t (*on_body_cb) (char* buf, uint32_t len); // body回调函数
    };
    dynmem_t        data;               // 回复内容关联的缓冲区
    uv_buf_t*       write_bufs;         // 回复内容数据区数组，.base = NULL结尾，由tcp服务自行管理内存
} httpres_t;

// http 上下文对象, 每个http连接创建1个
struct httpctx_t {
    uv_tcp_t        tcp;                // tcp连接对象, 兼容 uv_tcp_t, uv_stream_t, uv_handle_t
    httpreq_t       req;                // 请求对象
    httpres_t       res;                // 回复对象
    httpctx_pool_t* pool;               // 内存池对象，指向为自身分配内存的内存池对象，释放内存时使用
    on_httpctx_serve_cb serve_cb;       // 服务回调处理函数
};

/** 创建httpctx内存池分配对象
 * @param headers_count     http_header_node_t头部链表对象预分配数量
 * @param ctx_count         httpctx_t上下文对象预分配数量
 * @return                  新建的内存池对象
*/
inline static httpctx_pool_t* httpctx_pool_malloc(uint32_t headers_count, uint32_t ctx_count) {
    httpctx_pool_t* pool = malloc(sizeof(httpctx_pool_t));
    pool->headers_pool = pool_malloc(headers_count, sizeof(http_header_node_t));
    pool->ctx_pool = pool_malloc(ctx_count, sizeof(httpctx_t));
    return pool;
}

/** 释放httpctx内存池对象
 * @param self              内存池对象
*/
inline static void httpctx_pool_free(httpctx_pool_t *self) {
    pool_free(self->ctx_pool);
    pool_free(self->headers_pool);
    free(self);
}

/** 初始化httpctx内存池对象
 * @param self              内存池对象
*/
extern void httpctx_init(httpctx_t* self);

/** 分配一个新的httpctx上下文对象
 * @param pool              httpctx内存池对象
 * @param cb                http服务回调处理函数
*/
inline static httpctx_t* httpctx_pool_get(httpctx_pool_t* self, on_httpctx_serve_cb cb) {
    httpctx_t* pctx = (httpctx_t*) pool_get(self->ctx_pool);
    memset(pctx, 0, sizeof(httpctx_t));
    pctx->pool = self;
    pctx->serve_cb = cb;
    httpctx_init(pctx);
    return pctx;
}

/** 释放httpctx上下文对象内部对象的内存空间，包含缓冲区和头部链表指针(不释放pctx对象本身)
 * @param self              httpctx上下文对象
*/
extern void httpctx_free_data(httpctx_t* self);

/** 释放httpctx上下文对象(连带pctx对象也被释放)
 * @param self              httpctx上下文对象
*/
inline static void httpctx_free(httpctx_t* self) {
    httpctx_free_data(self);
    pool_put(self->pool->ctx_pool, self);
}

/** 重置httpctx上下文对象状态(释放内部对象占有的内存空间，其他对象初始化，适用于本次请求处理完毕后复用该对象进行下次请求处理)
 * @param self              httpctx上下文对象
*/
inline static void httpctx_reset(httpctx_t* self) {
    httpctx_free_data(self);
    memset(&self->req, 0, sizeof(httpreq_t) + sizeof(httpres_t));
    httpctx_init(self);
}

/** 将http_header_node_t对象放回内存池
 * @param self              httpctx上下文对象
 * @param headers           要放回内存池的http_header_node_t对象
*/
inline static void httpctx_headers_free(httpctx_t* self, http_header_node_t* headers) {
    list_del((list_head_t*) headers);
    pool_put(self->pool->headers_pool, headers);
}

/** 解析http协议内容(当收到新的数据时调用，分段接收时可多次调用)
 * @param self              httpctx上下文对象
 * @param buf               收到的数据缓冲区
 * @param len               数据缓冲区长度 
*/
extern size_t httpctx_parser_execute(httpctx_t* self, char* buf, size_t len);

/** 设置回复消息的内容类型
 * @param self              httpctx上下文对象
 * @param value             Comtent-Type值，字符串形式
*/
inline static void httpctx_set_content_type(httpctx_t* self, const str_t value) {
    self->res.content_type.pos = dynmem_len(&self->res.data);
    self->res.content_type.len = dynmem_append(&self->res.data, value, str_len(value));
}

/** 增加回复消息的头部字段
 * @param self              httpctx上下文对象
 * @param field             头部字段名，字符串
 * @param value             头部字段值，字符串
*/
inline static void httpctx_add_header(httpctx_t* self, const str_t field, const str_t value) {
    dynmem_t* pbuf = &self->res.data;
    http_header_node_t* h = pool_get(self->pool->headers_pool); // 从链表缓冲池中取一个元素
    h->data.field.pos = dynmem_len(pbuf);
    h->data.field.len = dynmem_append(pbuf, field, str_len(field));
    h->data.value.pos = dynmem_len(pbuf);
    h->data.value.len = dynmem_append(pbuf, value, str_len(value));
    // 添加到头部链表末尾
    list_add_tail((list_head_t*) h, &self->res.headers);
}

/** 设置回复内容, 一次性设置body，调用该函数前必须完成http header的设置，调用后body也设置完成
 * @param self              请求上下文对象
*/
inline static void httpctx_set_body(httpctx_t* self, const void* src, uint32_t len) {
    self->res.body.pos = dynmem_len(&self->res.data);
    self->res.body.len = dynmem_append(&self->res.data, src, len);
}

/** 开始回复内容的写入，调用该函数后，不允许再写入http header信息, 否则会出现不可预期的错误
 * @param self              请求上下文对象
*/
inline static void httpctx_body_begin(httpctx_t* self) {
    self->res.body.pos = dynmem_len(&self->res.data);
}

/** 回复内容的写入(可多次调用)，调用该函数前，必须使用httpctx_body_begin进行初始化
 * @param self              请求上下文对象
*/
inline static void httpctx_body_append(httpctx_t* self, const void* src, uint32_t len) {
    self->res.body.len += dynmem_append(&self->res.data, src, len);
}

/** 结束回复内容的写入，该函数为保留函数，暂时不实现任何功能
 * @param self              请求上下文对象
*/
inline static void httpctx_body_end(httpctx_t* self) { }

/** 获取请求的method字符串
 * @param self              请求上下文对象
 * @return                  请求字符串
*/
extern const char* httpctx_get_method(httpctx_t* self);

/** 获取请求的method字符串
 * @param self              请求上下文对象
 * @return                  找到的头部对象
*/
extern const http_value_t* httpctx_get_header(httpctx_t* self, const str_t name);

/** 路径匹配, 路径匹配返回true
 * @param self              请求上下文对象
 * @param path              要比较的路径
 * @return                  true：匹配成功，false：匹配失败
*/
extern bool httpctx_path_equal(httpctx_t* self, const str_t path);

/** 路径匹配, 前缀路径匹配上就返回true, 例如: path = "/x", 不匹配 "/x1", 匹配 "/x", "/x/", "/x?t=", "/x/?t=", "/x/y/z", "/x/y?t="
 * @param self              请求上下文对象
 * @param path              要比较的路径
 * @return                  true：匹配成功，false：匹配失败
*/
extern bool httpctx_path_prefix(httpctx_t* self, const str_t path);

#ifdef __cplusplus
}
#endif

#endif // __HTTPCTX_H__