#include <string.h>
#include "uv.h"
#include "buffer.h"
#include "httpserver.h"
#include "httpctx.h"
#include "log.h"

typedef struct {
    list_head_t         node;
    httpctx_pool_t*     data;
} _pool_list_node_t;

typedef struct {
    uv_write_t          write;
    httpctx_t*          httpctx;
} resp_write_t;

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
    http_header_node_t* hpos;
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
    http_server_t server;


}