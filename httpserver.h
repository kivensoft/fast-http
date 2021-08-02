#pragma once
#ifndef __HTTPSERVER_H__
#define __HTTPSERVER_H__

#include "uv.h"
#include "httpctx.h"
#include "str.h"
#include "rbtree.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef on_httpctx_serve_cb on_http_serve_cb;

// Http Server对象结构
typedef struct http_server_t {
    uv_tcp_t            tcp;            // libuv结构
    httpctx_pool_t*     pool;           // 上下文内存池
    on_http_serve_cb    serve_cb;       // 用户定义的回调处理函数
} http_server_t;

// 路由条目定义，采用嵌套式二叉树，效率应该会更高一些，即路由表结构为每一级为一个二叉树
typedef struct http_route_node_t {
    rb_node_t           node;           // 红黑树节点结构
    rb_root_t           tree;           // 该节点下的子树
    on_http_serve_cb    func;           // api处理回调函数
    uint32_t            plen;           // 路径长度
    const char*         path;           // api路径
} http_route_node_t;

// 路由定义
typedef struct http_route_t {
    rb_root_t           tree;           // 红黑树结构
    http_server_t       http_server;    // http服务
    on_http_serve_cb    static_func;    // 静态资源回调函数
    on_http_serve_cb    default_func;   // 缺省处理回调函数
} http_route_t;

/** 创建http服务，并进入uv的事件处理流程
 * @param puv_loop      uv的事件循环处理器, 为空时使用uv默认的处理器
 * @param pserver       http服务结构，由调用方进行内存分配
 * @param listen        监听地址, host:port 格式
 * @param backlog       允许的监听队列最大排队数量
 * @param callback      http服务回调地址, 每次收到http请求并解析成功后，由http服务进行回调
*/
extern bool http_server(uv_loop_t* puv_loop, http_server_t* pserver,
        const char* listen, int backlog, on_http_serve_cb callback);

/** 创建http服务，并进入uv的事件处理流程
 * @param puv_loop      uv的事件循环处理器, 为空时使用uv默认的处理器
 * @param route         路由结构
 * @param listen        监听地址, host:port 格式
 * @param backlog       允许的监听队列最大排队数量
 */
extern bool http_service(uv_loop_t* puv_loop, http_route_t* route, const char* listen, int backlog);

extern void http_route_init(http_route_t* route);

extern _Bool http_route_add(const http_route_t* self, const str_t path, on_http_serve_cb func);

extern _Bool http_route_del(const http_route_t* self, const str_t path);

#ifdef __cplusplus
}
#endif

#endif // __HTTPSERVER_H__