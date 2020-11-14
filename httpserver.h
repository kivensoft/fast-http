#pragma once
#ifndef __HTTPSERVER_H__
#define __HTTPSERVER_H__

#include "uv.h"
#include "httpctx.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef on_httpctx_serve_cb on_http_serve_cb;

typedef struct {
    uv_tcp_t            tcp;
    httpctx_pool_t*     pool;
    on_http_serve_cb    serve_cb;
} http_server_t;


/** 创建http服务，并进入uv的事件处理流程
 * @param pserver       http服务结构，由调用方进行内存分配
 * @param listen        监听地址, host:port 格式
 * @param backlog       允许的监听队列最大排队数量
 * @param callback      http服务回调地址, 每次收到http请求并解析成功后，由http服务进行回调
*/
extern bool http_server(http_server_t* pserver, const char* listen,
        int backlog, on_http_serve_cb callback);

#ifdef __cplusplus
}
#endif

#endif // __HTTPSERVER_H__