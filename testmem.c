#include <string.h>
#include "uv.h"
#include "memwatch.h"
#include "httpserver.h"
#include "httpctx.h"
#include "log.h"

typedef struct {
    list_head_t         node;
    httpctx_pool_t*     data;
} httpctx_pool_list_t;

typedef struct {
    uv_write_t          write;
    httpctx_t*          httpctx;
} resp_write_t;

static httpctx_pool_list_t httpctx_pool_list = { LIST_HEAD_INIT(httpctx_pool_list.node), NULL };

static void on_httpctx_pool_destroy() {
    log_trace("on_httpctx_pool_destroy, free httpctx_pool");
    httpctx_pool_list_t *pos;
    list_for_each_ex(pos, &httpctx_pool_list) {
        httpctx_pool_free(pos->data);
        free(pos);
    }
}

static void buffer_copyfrom(buffer_t* pbuf, http_value_t* src) {
    uint32_t slen = src->len;
    if (!slen) return;
    uint32_t spos = src->pos, send = slen + spos;
    char tmp[1024], *copy;
    if (slen <= sizeof(tmp)) {
        copy = tmp;
    } else {
        copy = malloc((slen + BUFFER_M1) & BUFFER_M1);
    }
    buffer_read(pbuf, copy, spos, send);
    buffer_write(pbuf, copy, slen);
    if (copy != tmp)
        free(copy);
}

static uint32_t fill_uv_buf_ts(uv_buf_t* bufs, buffer_t* pbuf, uint32_t start, uint32_t size) {
    uv_buf_t* p = bufs;
    for (uint32_t end = start + size; start < end; ++p) {
        uint32_t blen = end - start;
        if (blen > BUFFER_C1) blen = BUFFER_C1;
        p->base = buffer_get(pbuf, start);
        p->len = blen;
        start += blen;
    }
    return p - bufs;
}

static uint32_t write_http_header(httpctx_t* pctx) {
    httpres_t* res = &pctx->res;
    buffer_t* pbuf = &res->data;
    uint32_t write_start = pbuf->len;
    
    uint32_t body_len = res->body_type ? res->content_length : res->body.len;
    // 生成http回复状态消息和消息长度
    int prlen = snprintf(buffer_get(pbuf, write_start), BUFFER_C1, "HTTP/1.1 200 OK\r\nContent-Length: %u\r\n", body_len);
    buffer_set_length(pbuf, write_start + prlen);
    // 如果需要保持连接，写入保持连接的头部
    if (res->keep_alive)
        buffer_write(pbuf, "Connection: keep-alive\r\n", 26);
    // 如果content_type类型有设置，设置content_type
    if (res->content_type.len) {
        buffer_write(pbuf, "Content-Type: ", 14);
        buffer_copyfrom(pbuf, &res->content_type);
        buffer_write(pbuf, "\r\n", 2);
    }

    // 处理其他头部字段
    http_headers_t* hpos;
    list_for_each_ex(hpos, &res->headers) {
        http_header_t* p = &hpos->data;
        buffer_copyfrom(pbuf, &p->field);
        buffer_write(pbuf, ": ", 2);
        buffer_copyfrom(pbuf, &p->value);
        buffer_write(pbuf, "\r\n", 2);
    }

    buffer_write(pbuf, "\r\n", 2);
    return pbuf->len - write_start;
}

int main() {
    mwInit();

    http_server_t server;

    server.serve_cb = NULL;
    server.pool = httpctx_pool_malloc(256, 32);
    httpctx_pool_list_t* plist = malloc(sizeof(httpctx_pool_list_t));
    plist->data = server.pool;
    list_add_tail((list_head_t*) plist, (list_head_t*) &httpctx_pool_list);

    httpctx_t* client = httpctx_malloc(server.pool, server.serve_cb);

    buffer_t* pbuf = &client->req.data;
    uint32_t len = pbuf->len, surplus = 1024 - (len & BUFFER_M1);
    buffer_expand_capacity(pbuf, len + surplus);

    httpres_t* pres = &client->res;
    pres->body.pos = buffer_length(&pres->data);
    pres->body.len = buffer_write(&pres->data, "Hello World!", 12);

    uint32_t head_size = write_http_header(client);

    // 计算uv_buf_t数组长度
    uint32_t bufs_size = (head_size + BUFFER_M1) >> BUFFER_B1;
    uint32_t body_size = client->res.body.len;
    bufs_size += (body_size + BUFFER_M1) >> BUFFER_B1;

    // 生成uv_buf_t数组, 作为uv_write写入函数的数据区
    uv_buf_t* bufs = malloc(sizeof(uv_buf_t) * (bufs_size + 1));
    bufs[bufs_size].base = NULL;
    bufs[bufs_size].len = 0;
    client->res.write_bufs = bufs;

    uint32_t idx = 0;
    
    // 处理头部内容的uv_buf_t
    pbuf = &client->res.data;
    uint32_t write_start = pbuf->len;
    idx += fill_uv_buf_ts(&bufs[idx], &client->res.data, write_start, head_size);
    // 处理body的uv_buf_t
    idx += fill_uv_buf_ts(&bufs[idx], &client->res.data, client->res.body.pos, client->res.body.len);

    resp_write_t *wri = malloc(sizeof(resp_write_t));
    wri->httpctx = client;

    uv_buf_t** pbufs = &wri->httpctx->res.write_bufs;
    if (*pbufs) free(*pbufs);
    *pbufs = NULL;
    httpctx_reset(wri->httpctx);
    free(wri);

    bufs = client->res.write_bufs;
    if (bufs) free(bufs);
    httpctx_free(client);

    on_httpctx_pool_destroy();

    mwTerm();
}