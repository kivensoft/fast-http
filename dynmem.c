#include "dynmem.h"
#include <string.h>

// 字符查找回调函数的用户自定义参数结构
typedef struct chr_arg_t {
    char        ch;     // 需要查找的字符
    uint32_t    pos;    // 返回值，查找到的位置，找不到返回结束位置的下一个位置
} chr_arg_t;

// 双向指针链表初始化
inline static void list_init(_dynmem_head_t *head) {
    head->prev = (_dynmem_node_t*) head;
    head->next = (_dynmem_node_t*) head;
}

/** 往双向链表尾部插入一个节点
 * @param head      指针列表头部节点
 * @param node      要插入的节点
*/
inline static void list_add(_dynmem_head_t *head, _dynmem_node_t *node) {
    _dynmem_node_t *prev = head->prev;
    node->prev = prev;
    node->next = (_dynmem_node_t*) head;
    prev->next = node;
    head->prev = node;
}

/** 双向链表删除一个节点
 * @param node      要删除的节点
*/
inline static void list_del(_dynmem_node_t *node) {
    _dynmem_node_t *prev = node->prev, *next = node->next;
    prev->next = next;
    next->prev = prev;
}

/** 往双向链表尾部插入一个节点(调用malloc分配节点内存，并返回节点)
 * @param head      指针列表头部节点
*/
inline static _dynmem_node_t* list_new(_dynmem_head_t *head) {
    _dynmem_node_t *ret = malloc(sizeof(_dynmem_node_t));
    list_add(head, ret);
    return ret;
}

// 计算相对于偏移和长度的真实有效长度
inline static uint32_t check_len(uint32_t cap, uint32_t off, uint32_t len) {
    if (off > cap) return 0;
    if (off + len > cap)
        len = cap - off;
    return len;
}

/** dynmem_write默认实现的函数 */
static uint32_t on_write(void* arg, void *data, uint32_t len) {
    memcpy(data, *(char**)arg, len);
    *(char**)arg += len; // 源地址指针前移
    return len;
}

/** dynmem_read模式实现的回调函数 */
static uint32_t on_read(void* arg, void *data, uint32_t len) {
    memcpy(*(char**)arg, data, len);
    *(char**)arg += len;
    return len;
}

/** dynmem_chr模式实现的回调函数 */
static uint32_t on_find_char(void* arg, void *data, uint32_t len) {
    char* p = (char*)memchr(data, ((chr_arg_t*)arg)->ch, len);
    if (!p) {
        ((chr_arg_t*)arg)->pos += len;
        return len;
    } else {
        ((chr_arg_t*)arg)->pos += p - (char*)data;
        return 0;
    }
}

/** dynmem_equal模式实现的回调函数 */
static uint32_t on_equal(void *arg, void *data, uint32_t len) {
    if (memcmp(data, *(char**)arg, len)) {
        *(char**)arg = NULL;
        return 0;
    } else {
        *(char**)arg += len;
        return len;
    }
}

/** 缓冲区扩展1页大小
 * @param self      缓冲区指针
 * @return          新分配的内存页地址
*/
static uint8_t* dynmem_grow(dynmem_t *self) {
    uint8_t *buf = malloc(self->page);
    _dynmem_node_t *node = list_new(&self->head);
    node->data = buf;
    self->cap += self->page;
    return buf;
}

/** 获取指定位置所在的页节点
 * @param self      缓冲区指针
 * @param pos       位置信息
 * @return          所在页的节点地址
*/
static _dynmem_node_t* dynmem_getpage(dynmem_t *self, uint32_t pos) {
    _dynmem_node_t *node = self->head.next;
    uint32_t ps = self->page;
    while (pos >= ps) {
        pos -= ps;
        node = node->next;
    }
    return node;
}

void dynmem_init(dynmem_t *self, uint32_t page) {
    uint32_t p = 1;
    while (p < page) p <<= 1;
    self->page = p;
    self->len = 0;
    self->cap = 0;
    list_init(&self->head);
}

dynmem_t* dynmem_malloc(uint32_t page) {
    dynmem_t *ret = malloc(sizeof(dynmem_t));
    dynmem_init(ret, page);
    return ret;
}

void dynmem_clear(dynmem_t *self) {
    // 释放链表指向的内存页和链表自身节点的内存
    _dynmem_node_t *pos, *tmp, *head = (_dynmem_node_t*)(&self->head);
    // 分配是从开头分配，释放的时候从结尾开始释放，方便内存管理器合并内存，减少内存碎片
    for (pos = head->prev, tmp = pos->prev; pos != head; pos = tmp, tmp = pos->prev) {
        free(pos->data);
        free(pos);
    }

    self->len = 0;
    self->cap = 0;
    list_init((_dynmem_head_t*)head);
}

dynmem_t* dynmem_copy(dynmem_t *self) {
    uint32_t len = self->len, page = self->page;

    dynmem_t *ret = malloc(sizeof(dynmem_t));
    ret->page = page;
    ret->cap = 0;
    ret->len = len;

    _dynmem_head_t *ret_head = &ret->head;
    list_init(ret_head);

    _dynmem_node_t *pos, *head = (_dynmem_node_t*)(&self->head);
    for (pos = head->next; len && pos != head; pos = pos->next) {
        uint32_t count = len > page ? page : len;
        uint8_t *buf = dynmem_grow(ret);
        memcpy(buf, pos->data, count);
        len -= count;
    }

    return ret;
}

void dynmem_set_len(dynmem_t *self, uint32_t newlen) {
    // 新的长度大于已有容量，则进行扩充
    while (newlen > self->cap)
        dynmem_grow(self);
    self->len = newlen;
}

void dynmem_set_cap(dynmem_t *self, uint32_t newcap) {
    if (self->cap < newcap) {
        while (self->cap < newcap)
            dynmem_grow(self);
    } else {
        uint32_t page = self->page;
        newcap += page;
        _dynmem_head_t *head = &self->head;
        while (self->cap >= newcap) {
            _dynmem_node_t *pos = head->prev;
            free(pos->data);
            list_del(pos);
            free(pos);
            self->cap -= page;
        }
    }

    if (self->len > self->cap) self->len = self->cap;
}

uint32_t dynmem_align(dynmem_t *self) {
    uint32_t mask = self->page - 1, new_len = (self->len + mask) & ~mask;
    if (new_len >= self->cap)
        dynmem_grow(self);
    self->len = new_len;
    return new_len;
}

uint8_t* dynmem_get(dynmem_t *self, uint32_t offset) {
    if (offset >= self->cap) return NULL;
    return dynmem_getpage(self, offset)->data + (offset & (self->page - 1));
}

void dynmem_lastpage(dynmem_t *self, uint8_t **out_ptr, uint32_t *out_len) {
    uint32_t len = self->len;

    if (len == self->cap)
        *out_ptr = dynmem_grow(self);
    else if (len + self->page >= self->cap)
        *out_ptr = self->head.prev->data;
    else
        *out_ptr = dynmem_get(self, len);

    *out_len = dynmem_surplus(self, len);
}

uint32_t dynmem_offset(dynmem_t *self, const void *pointer) {
    _dynmem_head_t *head = &self->head;
    _dynmem_node_t *node = head->next;
    uint8_t *p_end = (uint8_t*)pointer - self->page;
    for (uint32_t i = 0; node != (_dynmem_node_t*) head; ++i, node = node->next) {
        uint8_t *data = node->data;
        if ((uint8_t*) pointer >= data && p_end < data)
            return (uint8_t*)pointer - data;
    }
    return 0xFFFFFFFF;
}

uint32_t dynmem_foreach(dynmem_t *self, void *arg, uint32_t off, uint32_t len, dynmem_on_foreach callback) {
    len = check_len(self->cap, off, len);
    if (!len) return 0;

    // 找到off对应的page
    uint32_t ps = self->page;
    struct _dynmem_node_t *node = dynmem_getpage(self, off);
    off &= ps - 1;

    // 实际可供循环的长度, 新启变量以便保留原有len作为返回值
    uint32_t loop_len = len;

    // 处理偏移非页起始的情况
    if (off) {
        uint32_t size = ps - off;
        if (size > loop_len)
            size = loop_len;
        if (!callback(arg, node->data + off, size))
            goto _return;
        loop_len -= size;
        node = node->next;
    }

    // 处理对齐页
    while (loop_len) {
        // 当前页可供循环的大小
        uint32_t size = loop_len > ps ? ps : loop_len;
        if (!callback(arg, node->data, size))
            goto _return;
        loop_len -= size;
        node = node->next;
    }

_return:
    return len;
}

uint32_t dynmem_write(dynmem_t *self, uint32_t off, const void *src, uint32_t len) {
    uint32_t end = off + len;
    if (end > self->cap) dynmem_set_cap(self, end);
    dynmem_foreach(self, &src, off, len, on_write);
    if (self->len < end) self->len = end;
    return len;
}

uint32_t dynmem_read(dynmem_t *self, uint32_t off, uint32_t len, void *dst) {
    uint32_t slen = self->len;
    if (off >= slen) return 0;
    if (off + len > slen) len = slen - off;
    dynmem_foreach(self, &dst, off, len, on_read);
    return len;
}

int dynmem_chr(dynmem_t *self, uint32_t off, uint32_t len, uint8_t ch) {
    if (off >= self->len) return -1;
    chr_arg_t arg = { .pos = off, .ch = ch };
    dynmem_foreach(self, &arg, off, len, on_find_char);
    return arg.pos < off + len ? arg.pos : -1;
}

int dynmem_rchr(dynmem_t *self, uint32_t off, uint32_t len, uint8_t ch) {
    len = check_len(self->cap, off, len);
    if (!len) return 0;

    uint32_t ps = self->page, end = off + len - 1, loop_len = len;
    _dynmem_node_t *node = dynmem_getpage(self, end);
    end &= ps - 1;

    // 处理最后一页，页不对齐的情况
    if (end != ps -1) {
        uint32_t c = end + 1;
        c = c <= loop_len ? 0 : c - loop_len;
        for (uint8_t *b = node->data + end, *e = node->data + c; b >= e; --b) {
            if (*b == ch)
                return off + loop_len - 1 - (node->data + end - b);
        }
        if (loop_len > end + 1) {
            loop_len -= end + 1;
            node = node->prev;
        } else {
            return -1;
        }
    }
    while (loop_len) {
        uint32_t c = loop_len > ps ? 0 : ps - loop_len;
        for (uint8_t *b = node->data + ps - 1, *e = node->data + c; b >= e; --b)
            if (*b == ch)
                return off + loop_len - (ps - c) + (b - e);
        loop_len -= ps - c;
        node = node->prev;
    }
    return -1;
}

bool dynmem_equal(dynmem_t *self, uint32_t off, uint32_t len, const void *data) {
    if (off >= self->len || off + len > self->len) return 0;
    const char* arg = data;
    dynmem_foreach(self, &arg, off, len, on_equal);
    return arg;
}

//======================================================================
// #define TEST_DYNMEM
#ifdef TEST_DYNMEM
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

int main() {
    const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    uint8_t data[5000], buf[5000];
    // 初始化data，用随机值填充
    srand((unsigned)time(NULL));
    for (int i = 0; i < sizeof(data); i++) {
        data[i] = base64[rand() & 63];
    }

    // 测试初始内存分配函数
    dynmem_t mem;
    dynmem_init(&mem, 3);
    assert(mem.page == 4);
    dynmem_init(&mem, 4);
    assert(mem.page == 4);
    assert(mem.cap == 0);
    assert(mem.len == 0);

    // 测试写入函数
    dynmem_write(&mem, 0, data, 10);
    assert(mem.len == 10);
    assert(mem.cap == 12);
    assert(!memcmp(dynmem_get(&mem, 0), data, 4));
    assert(!memcmp(dynmem_get(&mem, 4), data + 4, 4));
    assert(!memcmp(dynmem_get(&mem, 8), data + 8, 2));

    // 测试读取函数
    memset(buf, '@', sizeof(buf));
    uint32_t rn = dynmem_read(&mem, 3, 10, buf);
    assert(rn == 7);
    assert(!memcmp(buf, data + 3, 7));

    // 测试clone函数
    dynmem_t* new_mem = dynmem_copy(&mem);
    dynmem_set_cap(&mem, 7);
    assert(mem.cap == 8);
    assert(mem.len == 8);

    // 测试设置长度函数
    dynmem_set_len(new_mem, 7);
    assert(new_mem->len == 7);
    assert(new_mem->cap == 12);

    dynmem_write(&mem, 0, data, 8);
    assert(mem.cap == 8);
    assert(mem.len == 8);
    memset(buf, '@', sizeof(buf));
    dynmem_read(&mem, 0, 8, buf);
    assert(!memcmp(data, buf, 8));

    // 测试字符查找功能
    dynmem_set_len(&mem, 0);
    dynmem_write(&mem, 0, base64, sizeof(base64) - 1);
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < sizeof(base64) - 1; i++) {
            int pos = dynmem_chr(&mem, 4 + j, 40 + j, base64[i]);
            if (i < 4 + j || i >= 44 + j + j) {
                if (pos != -1)
                    printf("test chr fail: i = %d, pos = %d\n", i, pos);
            } else {
                if (pos != i)
                    printf("test chr fail: i = %d, pos = %d\n", i, pos);
            }
        }
        for (int i = 0; i < sizeof(base64) - 1; i++) {
            int pos = dynmem_rchr(&mem, 4 + j, 40 + j, base64[i]);
            if (i < 4 + j || i >= 44 + j + j) {
                if (pos != -1)
                    printf("test lastchr fail: i = %d, j = %d, pos = %d\n", i, j, pos);
            } else {
                if (pos != i)
                    printf("test lastchr fail: i = %d, j = %d, pos = %d\n", i, j, pos);
            }
        }
    }

    // 测试内容比较功能
    for (int i = 0; i < 64; i++) {
        int j = i + 10 + i > 64 ? 64 - i : 10 + i;
        bool b = dynmem_equal(&mem, i, j, base64 + i);
        if (!b) {
            printf("equal fail: i = %d, j = %d\n", i, j);
        }
    }

    // 测试复制功能
    dynmem_t m1;
    dynmem_init(&m1, 8);
    dynmem_write(&m1, 0, "012", 3);
    uint8_t* p1 = dynmem_next_pos(&m1);
    uint32_t n1;
    dynmem_lastpage(&m1, &p1, &n1);
    assert(n1 == 5);
    dynmem_write(&m1, 3, "0123456789abcdef", 16);
    printf("m1.len = %d\n", m1.len);
    assert(m1.len == 19);

    dynmem_t *m2 = dynmem_copy(&m1);
    dynmem_clear(&m1);
    assert(m1.len == 0);
    assert(m1.head.prev == m1.head.next && m1.head.prev == (_dynmem_node_t*)&m1.head);
    printf("m2.len = %d, m2.cap = %d, m2.page = %d\n", m2->len, m2->cap, m2->page);
    assert(m2->len == 19);
    p1 = (uint8_t*)"0120123456789abcdef";
    for (int i = 0; i < 19; i++) assert(*dynmem_get(m2, i) == p1[i]);
    dynmem_free(m2);

    printf("memarray test complete!\n");
}
#endif