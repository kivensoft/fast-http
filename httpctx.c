#include "httpctx.h"
#include "log.h"

// 编译参数 -- 消息处理缓冲区分页大小
#ifndef HTTPCTX_PAGE_SIZE
#   define HTTPCTX_PAGE_SIZE 2048
#endif

#define PARSER_OF_CTX(ptr) ((httpctx_t*) ((char*) ptr - (size_t) &(((httpctx_t*)0)->req.parser)))

// 解析状态枚举值
typedef enum { P_BEGIN, P_URL, P_HEAD_FIELD, P_HEAD_VALUE, P_BODY } hc_parser_state_t;

void httpctx_init(httpctx_t* pctx) {
    mem_array_init(&pctx->req.data, HTTPCTX_PAGE_SIZE);
    list_head_init((list_head_t*) &pctx->req.headers);
    mem_array_init(&pctx->res.data, HTTPCTX_PAGE_SIZE);
    list_head_init((list_head_t*) &pctx->res.headers);
    http_parser_init(&pctx->req.parser, HTTP_REQUEST);
    pctx->res.status = HTTP_STATUS_OK;
}

void httpctx_free_data(httpctx_t* pctx) {
    mem_array_destroy(&pctx->req.data);
    mem_array_destroy(&pctx->res.data);

    list_head_t* pos, *pos_tmp, *head = (list_head_t*) &pctx->req.headers;
    list_for_each_safe(pos, pos_tmp, head) {
        mempool_put(pctx->pool->headers_pool, (http_headers_t*) pos);
    }
    list_head_init(head);

    head = (list_head_t*) &pctx->res.headers;
    list_for_each_safe(pos, pos_tmp, head) {
        mempool_put(pctx->pool->headers_pool, (http_headers_t*) pos);
    }
    list_head_init(head);
}

// 解析http协议版本
inline static hc_http_version_t parse_http_version(uint16_t http_major, uint16_t http_minor) {
    switch (http_major) {
        case 0: return HC_HTTP10;
        case 1: return http_minor ? HC_HTTP11 : HC_HTTP10;
        case 2: return HC_HTTP20;
        default: return HC_HTTP10;
    }
}

// 解析http method请求类型
inline static hc_http_method_t parse_http_method(unsigned int http_method) {
    switch (http_method) {
        case HTTP_GET:      return HC_HTTP_GET;
        case HTTP_POST:     return HC_HTTP_POST;
        case HTTP_PUT:      return HC_HTTP_PUT;
        case HTTP_DELETE:   return HC_HTTP_DELETE;
        default:            return HC_HTTP_POST;
    }
}

// 查表转换法，速度杠杠滴, 生成的字符映射表，对A-Z的字符用a-z替换
static uint8_t CHAR_IGNORE_CASE_MAP[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff};

/** 获取请求url中的参数起始位置
 * @param pctx              httpctx上下文对象
 * @return                  参数起始位置，即url中问号(?)后的位置，未找到返回-1
*/
inline static int32_t httpctx_get_param(mem_array_t* pbuf, http_value_t* url) {
    int find_pos = mem_array_chr(pbuf, url->pos, url->len, '?');
    return find_pos == -1 ? -1 : find_pos + 1;
}

/** 字符串比较，忽略大小写 */
inline static bool equal_ignore_case(const uint8_t* src, const uint8_t* dst, size_t len) {
    for (const uint8_t *src_end = src + len; src < src_end; ) {
        if (CHAR_IGNORE_CASE_MAP[*src++] != CHAR_IGNORE_CASE_MAP[*dst++])
            return false;
    }
    return true;
}

/** 缓冲区字符串不区分大小写比较回调函数 */
static uint32_t on_equal_ignore_case (void* arg, void* data, uint32_t len) {
    if (equal_ignore_case((const uint8_t*)data, *(uint8_t**)arg, len)) {
        *(uint8_t**)arg += len;
        return len;
    } else {
        *(uint8_t**)arg = NULL;
        return 0;
    }
}

/** buffer_t字符串比较 */
static bool mem_array_equal_ignore_case(mem_array_t* pbuf, http_value_t* val, const char* text) {
    size_t len = strlen(text);
    if (val->len != len)
        return false;
    const char* arg = text;
    mem_array_foreach(pbuf, &arg, val->pos, len, on_equal_ignore_case);
    return arg;
}

/** 获取指定头部字段内容 */
static http_value_t* get_http_header(http_headers_t* headers, mem_array_t* pbuf, const char* field) {
    http_headers_t* pos;
    size_t flen = strlen(field);
    list_for_each_ex(pos, headers) {
        http_value_t* f = &pos->data.field;
        if (f->len == flen && mem_array_equal_ignore_case(pbuf, f, field))
            return &pos->data.value;
    }
    return NULL;
}

/** 转换http头部键对应值成整数 */
static uint32_t get_http_header_uint(http_headers_t* headers, mem_array_t* pbuf, const char* field) {
    http_value_t* cl = get_http_header(headers, pbuf, field);
    if (!cl) return 0;
    uint32_t len = cl->len;
    char tmp[len + 1];
    mem_array_read(pbuf, cl->pos, len, tmp);
    tmp[len] = '\0';
    return atoi(tmp);
}

// http报文解析--起始回调函数
static int on_message_begin(http_parser* parser) {
    // log_trace("***HTTP_PARSER MESSAGE BEGIN***");
    return 0;
}

// http报文解析--头部解析完成回调函数
static int on_headers_complete(http_parser* parser) {
    // log_trace("***HTTP_PARSER HEADERS COMPLETE***");
    return 0;
}

// http报文解析--报文解析完成回调函数
static int on_message_complete(http_parser* parser) {
    // log_trace("***HTTP_PARSER MESSAGE COMPLETE***");
    httpctx_t* pctx = PARSER_OF_CTX(parser);
    httpreq_t* req = &pctx->req;
    http_headers_t* headers = &req->headers;
    mem_array_t* reqbuf = &req->data;

    // 设置请求对象的属性
    req->version = parse_http_version(parser->http_major, parser->http_minor);
    req->method = parse_http_method(parser->method);
    http_value_t* ka = get_http_header(headers, reqbuf, "Connection");
    req->keep_alive = ka && mem_array_equal_ignore_case(reqbuf, ka, "Keep-Alive");
    req->host = get_http_header(headers, reqbuf, "Host");
    req->content_type = get_http_header(headers, reqbuf, "Content-Type");
    req->content_length = get_http_header_uint(headers, reqbuf, "Content-Length");

    req->path.pos = req->url.pos;
    int32_t param_pos = httpctx_get_param(reqbuf, &req->url);
    if (param_pos != -1) {
        req->path.len = param_pos - 1;
        req->param.pos = param_pos;
        req->param.len = req->url.len - param_pos;
    } else {
        req->path.len = req->url.len;
        req->param.pos = 0;
        req->param.len = 0;
    }

    // 设置解析完成标志
    req->parser_state = HTTP_PARSER_COMPLETE;
    return 0;
}

// http报文解析--请求地址解析回调函数
static int on_url(http_parser* parser, const char* at, size_t length) {
    // log_trace("HTTP_PARSER url: %.*s", length, at);
    httpctx_t* pctx = PARSER_OF_CTX(parser);
    if (pctx->req.parser_state != P_URL) {
        pctx->req.parser_state = P_URL;
        pctx->req.url.pos = mem_array_offset(&pctx->req.data, at);
        pctx->req.url.len = length;
    } else {
        pctx->req.url.len += length;
    }

    return 0;
}

// http报文解析--报文头键名解析回调函数
static int on_header_field(http_parser* parser, const char* at, size_t length) {
    // log_trace("HTTP_PARSER header field: %.*s", length, at);
    httpctx_t* pctx = PARSER_OF_CTX(parser);
    http_headers_t* head_node;
    if (pctx->req.parser_state != P_HEAD_FIELD) {
        pctx->req.parser_state = P_HEAD_FIELD;

        // 创建新的headers节点并加入到链表末尾
        head_node = mempool_get(pctx->pool->headers_pool);
        head_node->data.field.pos = mem_array_offset(&pctx->req.data, at);
        head_node->data.field.len = length;
        list_add_tail((list_head_t*)head_node, (list_head_t*)(&pctx->req.headers));
    } else {
        // 找到最后的headers节点，修改其长度
        head_node = (http_headers_t*) pctx->req.headers.node.prev;
        head_node->data.field.len += length;
    }

    return 0;
}

// http报文解析--报文头变量解析回调函数
static int on_header_value(http_parser* parser, const char* at, size_t length) {
    // log_trace("HTTP_PARSER header data: %.*s", length, at);
    httpctx_t* pctx = PARSER_OF_CTX(parser);
    http_headers_t* last = (http_headers_t*) pctx->req.headers.node.prev;

    // 上次解析了field，对节点的data值进行全新处理
    if (pctx->req.parser_state == P_HEAD_FIELD) {
        pctx->req.parser_state = P_HEAD_VALUE;
        last->data.value.pos = mem_array_offset(&pctx->req.data, at);
        last->data.value.len = length;
    // 上次解析了data，对节点的data长度进行增加
    } else if (pctx->req.parser_state == P_HEAD_VALUE) {
        last->data.value.len += length;
    } else {
        log_trace("HTTP_PARSER parser error: find head data before head field!");
    }

    return 0;
}

// http报文解析--请求体解析回调函数
static int on_body(http_parser* parser, const char* at, size_t length) {
    // log_trace("HTTP_PARSER body: %.*s", length, at);
    httpctx_t* pctx = PARSER_OF_CTX(parser);
    if (pctx->req.parser_state != P_BODY) {
        pctx->req.parser_state = P_BODY;
        pctx->req.body.pos = mem_array_offset(&pctx->req.data, at);
        pctx->req.body.len = length;
    } else {
        pctx->req.body.len += length;
    }
    return 0;
}

// http报文解析设置--设置解析过程的回调函数地址
static http_parser_settings parser_settings = {
    .on_message_begin = on_message_begin,
    .on_url = on_url,
    .on_status = NULL,
    .on_header_field = on_header_field,
    .on_header_value = on_header_value,
    .on_headers_complete = on_headers_complete,
    .on_body = on_body,
    .on_message_complete = on_message_complete,
    .on_chunk_header = NULL,
    .on_chunk_complete = NULL,
};

size_t httpctx_parser_execute(httpctx_t* pctx, char* buf, size_t len) {
    return http_parser_execute(&pctx->req.parser, &parser_settings, buf, len);
}

const char* httpctx_get_method(httpreq_t* req) {
    static const char* HTTP_METHODS[] = {"GET", "POST", "PUT", "DELETE" };
    uint8_t method = req->method;
    return (method >=0 && method < sizeof(HTTP_METHODS)) ? HTTP_METHODS[method] : "UNKNOWN";
}

bool httpctx_pathcmp(httpreq_t* req, const char* path) {
    mem_array_t* pbuf = &req->data;
    http_value_t* value = &req->path;
    size_t len = strlen(path);
    bool sflag = path[len - 1] == '/';
    if (sflag) --len;

    // 需要匹配的路径比请求路径更长，直接返回匹配不成功
    if (len > value->len)
        return false;

    if (!mem_array_equal(pbuf, value->pos, len, path))
        return false;

    // 要匹配的路径带'/'路径符号
    if (sflag) {
        // 请求路径没有路径分隔符，匹配成功
        if (value->len == len)
            return true;

        // 比较请求路径下一个字符是否路径分隔符或者参数分隔符
        char ch = *mem_array_get(pbuf, value->pos + len);
        return ch == '/' || ch == '?';
    } else {
        return true;
    }
}