#include "memarray.h"
#include <string.h>

// 字符查找回调函数的用户自定义参数结构
typedef struct {
    char        ch;     // 需要查找的字符
    uint32_t    pos;    // 返回值，查找到的位置，找不到返回结束位置的下一个位置
} chr_arg_t;

/** 双向指针链表初始化
 * @param list      指针列表头部节点
*/
static void _list_init(struct _mem_list_t *list) {
    list->prev = list;
    list->next = list;
    list->data = NULL;
}

/** 往双向链表尾部插入一个节点
 * @param list      指针列表头部节点
 * @param node      要插入的节点
*/
static void _list_add(struct _mem_list_t *list, struct _mem_list_t *node) {
    struct _mem_list_t *prev = list->prev;
    node->prev = prev;
    node->next = list;
    prev->next = node;
    list->prev = node;
}

/** 双向链表删除一个节点
 * @param node      要插入的节点
*/
static void _list_del(struct _mem_list_t *node) {
    struct _mem_list_t *prev = node->prev, *next = node->next;
    prev->next = next;
    next->prev = prev;
}

/** 往双向链表尾部插入一个节点(调用malloc分配节点内存，并返回节点)
 * @param list      指针列表头部节点
*/
static struct _mem_list_t* _list_new(struct _mem_list_t* list) {
    struct _mem_list_t *ret = malloc(sizeof(struct _mem_list_t));
    _list_add(list, ret);
    return ret;
}

static uint32_t _fix_len(uint32_t cap, uint32_t off, uint32_t len) {
    if (off > cap) return 0;
    if (off + len > cap)
        len = cap - off;
    return len;
}

/** mem_array_write默认实现的函数 */
static uint32_t _on_write(void* arg, void* data, uint32_t len) {
    memcpy(data, *(char**)arg, len);
    *(char**)arg += len; // 源地址指针前移
    return len;
}

/** mem_array_read模式实现的回调函数 */
static uint32_t _on_read(void* arg, void* data, uint32_t len) {
    memcpy(*(char**)arg, data, len);
    *(char**)arg += len;
    return len;
}

/** mem_array_chr模式实现的回调函数 */
static uint32_t _on_find_char(void* arg, void* data, uint32_t len) {
    char* p = (char*)memchr(data, ((chr_arg_t*)arg)->ch, len);
    if (!p) {
        ((chr_arg_t*)arg)->pos += len;
        return len;
    } else {
        ((chr_arg_t*)arg)->pos += p - (char*)data;
        return 0;
    }
}

/** mem_array_equal模式实现的回调函数 */
static uint32_t _on_equal(void* arg, void* data, uint32_t len) {
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
static uint8_t* mem_array_expand(mem_array_t* self) {
    uint8_t *buf = malloc(self->pagesize);
    _list_new(&self->list)->data = buf;
    self->cap += self->pagesize;
    return buf;
}

/** 获取指定位置所在的页节点
 * @param self      缓冲区指针
 * @param pos       位置信息
 * @return          所在页的节点地址
*/
static struct _mem_list_t* _get_page(mem_array_t* self, uint32_t pos) {
    struct _mem_list_t *node = self->list.next;
    uint32_t ps = self->pagesize;
    while (pos >= ps) {
        pos -= ps;
        node = node->next;
    }
    return node;
}

void mem_array_init(mem_array_t* self, uint32_t pagesize) {
    if (!self) return;

    self->ref = 1;
    self->len = 0;
    self->cap = 0;
    
    uint32_t ps_align = 1, bits = 0;
    while (ps_align < pagesize) {
        ps_align <<= 1;
        ++bits;
    }
    self->pagesize = ps_align;

    _list_init(&self->list);
}

void mem_array_destroy(mem_array_t* self) {
    if (self && self->ref && !--self->ref) {
        // 释放链表指向的内存页和链表自身节点的内存
        struct _mem_list_t *pos, *tmp, *list = &self->list;
        for (pos = list->next, tmp = pos->next; pos != list; pos = tmp, tmp = pos->next) {
            free(pos->data);
            free(pos);
        }

        self->len = 0;
        self->cap = 0;
        _list_init(list);
    }
}

mem_array_t* mem_array_clone(mem_array_t* self) {
    uint32_t len = self->len, pagesize = self->pagesize;

    mem_array_t* ret = malloc(sizeof(mem_array_t));
    ret->ref = 1;
    ret->pagesize = pagesize;

    struct _mem_list_t *ret_list = &ret->list;
    _list_init(ret_list);

    struct _mem_list_t *pos, *list = &self->list;
    for (pos = list->next; len; pos = pos->next) {
        uint32_t c = len > pagesize ? pagesize : len;
        uint8_t *buf = mem_array_expand(ret);
        memcpy(buf, pos->data, c);
        ret->len += c;
        len -= c;
    }

    return ret;
}

void mem_array_set_length(mem_array_t* self, uint32_t newlen) {
    // 新的长度大于已有容量，则进行扩充
    while (newlen > self->cap)
        mem_array_expand(self);
    self->len = newlen;
}

void mem_array_set_capicity(mem_array_t* self, uint32_t new_capicity) {
    if (self->cap < new_capicity) {
        while (self->cap < new_capicity)
            mem_array_expand(self);
    } else {
        struct _mem_list_t *list = &self->list;
        new_capicity += self->pagesize;
        while (self->cap >= new_capicity) {
            struct _mem_list_t *pos = list->prev;
            free(pos->data);
            _list_del(pos);
            free(pos);
            self->cap -= self->pagesize;
        }
    }

    if (self->len > self->cap)
        self->len = self->cap;
}

uint32_t mem_array_align_len(mem_array_t* self) {
    uint32_t mask = self->pagesize - 1, len = self->len;
    uint32_t new_len = (len + mask) & ~mask;
    if (new_len >= self->cap)
        mem_array_expand(self);
    self->len = new_len;
    return new_len;
}

uint8_t* mem_array_get(mem_array_t* self, uint32_t offset) {
    if (offset >= self->cap) return NULL;
    return _get_page(self, offset)->data + (offset & (self->pagesize - 1));
}

void mem_array_last_page(mem_array_t* self, uint8_t** out_ptr, uint32_t* out_len) {
    uint32_t len = self->len;

    if (len == self->cap)
        *out_ptr = mem_array_expand(self);
    else if (len + self->pagesize >= self->cap)
        *out_ptr = self->list.prev->data;
    else
        *out_ptr = mem_array_get(self, len);

    *out_len = mem_array_surplus(self, len);
}

uint32_t mem_array_offset(mem_array_t* self, const void* pointer) {
    uint32_t ps = self->pagesize;
    struct _mem_list_t *head = &self->list, *node = head->next;
    for (uint32_t i = 0; node != head; ++i, node = node->next) {
        uint8_t* d = node->data;
        if ((uint8_t*)pointer >= d && (uint8_t*)pointer < d + ps)
            return (uint8_t*)pointer - d;
    }
    return 0xFFFFFFFF;
}

uint32_t mem_array_foreach(mem_array_t* self, void* arg, uint32_t off, uint32_t len, mem_array_on_foreach callback) {
    len = _fix_len(self->cap, off, len);
    if (!len) return 0;

    // 找到off对应的page
    uint32_t ps = self->pagesize;
    struct _mem_list_t *node = _get_page(self, off);
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

uint32_t mem_array_write(mem_array_t* self, uint32_t off, uint32_t len, const void* src) {
    uint32_t end = off + len;
    if (end > self->cap) mem_array_set_capicity(self, end);
    mem_array_foreach(self, &src, off, len, _on_write);
    if (self->len < end) self->len = end;
    return len;
}

uint32_t mem_array_read(mem_array_t* self, uint32_t off, uint32_t len, void* dst) {
    uint32_t slen = self->len;
    if (off >= slen) return 0;
    if (off + len > slen) len = slen - off;
    mem_array_foreach(self, &dst, off, len, _on_read);
    return len;
}

int mem_array_chr(mem_array_t* self, uint32_t off, uint32_t len, uint8_t ch) {
    if (off >= self->len)
        return -1;
    chr_arg_t arg = { .pos = off, .ch = ch };
    mem_array_foreach(self, &arg, off, len, _on_find_char);
    return arg.pos < off + len ? arg.pos : -1;
}

int mem_array_lastchr(mem_array_t* self, uint32_t off, uint32_t len, uint8_t ch) {
    len = _fix_len(self->cap, off, len);
    if (!len) return 0;

    uint32_t ps = self->pagesize, end = off + len - 1, loop_len = len;
    struct _mem_list_t *node = _get_page(self, end);
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

bool mem_array_equal(mem_array_t* self, uint32_t off, uint32_t len, const void* data) {
    if (off >= self->len || off + len > self->len)
        return 0;
    const char* arg = data;
    mem_array_foreach(self, &arg, off, len, _on_equal);
    return arg;
}

//======================================================================
#ifdef TEST_MEMARRAY
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>

#define ASSERT(x) if (!(x)) printf("%s:%d -- memarray test fail!\n", __FILE__, __LINE__)

int main() {
    const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    uint8_t data[5000], buf[5000];
    // 初始化data，用随机值填充
    srand((unsigned)time(NULL));
    for (int i = 0; i < sizeof(data); i++) {
        data[i] = base64[rand() & 63];
    }

    // 测试初始内存分配函数
    mem_array_t mem;
    mem_array_init(&mem, 3);
    ASSERT(mem.pagesize == 4);
    mem_array_init(&mem, 4);
    ASSERT(mem.pagesize == 4);
    ASSERT(mem.cap == 0);
    ASSERT(mem.len == 0);

    // 测试写入函数
    mem_array_write(&mem, 0, 10, data);
    ASSERT(mem.len == 10);
    ASSERT(mem.cap == 12);
    ASSERT(!memcmp(mem_array_get(&mem, 0), data, 4));
    ASSERT(!memcmp(mem_array_get(&mem, 4), data + 4, 4));
    ASSERT(!memcmp(mem_array_get(&mem, 8), data + 8, 2));

    // 测试读取函数
    memset(buf, '@', sizeof(buf));
    uint32_t rn = mem_array_read(&mem, 3, 10, buf);
    ASSERT(rn == 7);
    ASSERT(!memcmp(buf, data + 3, 7));

    // 测试clone函数
    mem_array_t* new_mem = mem_array_clone(&mem);
    ASSERT(mem.ref == 1);
    mem_array_set_capicity(&mem, 7);
    ASSERT(mem.cap == 8);
    ASSERT(mem.len == 8);

    // 测试设置长度函数
    mem_array_set_length(new_mem, 7);
    ASSERT(new_mem->len == 7);
    ASSERT(new_mem->cap == 12);

    mem_array_write(&mem, 0, 8, data);
    ASSERT(mem.cap == 8);
    ASSERT(mem.len == 8);
    memset(buf, '@', sizeof(buf));
    mem_array_read(&mem, 0, 8, buf);
    ASSERT(!memcmp(data, buf, 8));

    // 测试字符查找功能
    mem_array_set_length(&mem, 0);
    mem_array_write(&mem, 0, sizeof(base64) - 1, base64);
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < sizeof(base64) - 1; i++) {
            int pos = mem_array_chr(&mem, 4 + j, 40 + j, base64[i]);
            if (i < 4 + j || i >= 44 + j + j) {
                if (pos != -1)
                    printf("test chr fail: i = %d, pos = %d\n", i, pos);
            } else {
                if (pos != i)
                    printf("test chr fail: i = %d, pos = %d\n", i, pos);
            }
        }
        for (int i = 0; i < sizeof(base64) - 1; i++) {
            int pos = mem_array_lastchr(&mem, 4 + j, 40 + j, base64[i]);
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
        bool b = mem_array_equal(&mem, i, j, base64 + i);
        if (!b) {
            printf("equal fail: i = %d, j = %d\n", i, j);
        }
    }
    printf("memarray test complete!\n");
}
#endif