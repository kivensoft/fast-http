#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "pool.h"

#define PBB 2
#define PB 5
#define PC (1 << PB)
#define PM (PC - 1)

typedef struct _pool_list_t {
    struct _pool_list_t     *prev, *next;
} _pool_list_t;

struct _pool_head_t {
    uint32_t        capacity;       // 内存池容量, 指定内存池总共有多少个分配对象, 容量总是以32倍数对齐
    uint32_t        size;           // 分配对象大小
    uint32_t        last;           // 最后操作的索引值，保存该值是为了加快分配速度
    _pool_list_t    head;           // 动态分配的对象链表
    uint8_t         data[];         // 内存池位图数组及内存池地址，内存池位图数组大小为capacity/32, 内存池大小为capacity*size
};

pool_t pool_malloc(uint32_t capacity, uint32_t size) {
    // bits位图需要的字节数, 分配的长度按32倍数取整
    uint32_t bits_cap = (capacity + PM) >> PB;
    capacity = bits_cap << PB;
    uint32_t bits_size = bits_cap << PBB;
    uint32_t ptr_size = capacity * size;
    uint32_t pool_size = sizeof(struct _pool_head_t) + bits_size + ptr_size;

    pool_t pool = (pool_t) calloc(pool_size, 1);
    pool->capacity = capacity;
    pool->size = size;
    pool->head.prev = &pool->head;
    pool->head.next = &pool->head;

    return pool;
}

void pool_free(pool_t self) {
    _pool_list_t *head = &self->head, *pos = head->prev, *tmp = pos->prev;
    for (; pos != head; pos = tmp, tmp = pos->prev) {
        free(pos);
    }
    free(self);
}

/** 从内存池获取可用对象, 内存池使用满了后，使用malloc进行分配 */
void* pool_get(pool_t self) {
    uint32_t *bits = (uint32_t*)(self->data), bits_cap = self->capacity >> PB;

    // 从位图检索可用的项
    for (uint32_t i = self->last; i < bits_cap; ++i) {
        uint32_t b = bits[i];
        if (b != 0xFFFFFFFF) {
            for (uint32_t c = 1, j = 0; j < PC; ++j, c <<= 1) {
                if (!(b & c)) {
                    self->last = i;
                    bits[i] |= c;
                    return self->data + (bits_cap << PBB) + ((i << PB) | j) * self->size;
                }
            }
        }
    }

    self->last = bits_cap; // 内存池已经全部被使用，设置last为无效索引

    // 使用malloc进行动态内存分配并加入到链表
    _pool_list_t *entry = malloc(sizeof(_pool_list_t) + self->size);
    _pool_list_t *prev = self->head.prev, *next = &self->head;

    next->prev = entry;
    entry->next = next;
    entry->prev = prev;
    prev->next = entry;

    return (void*)entry + sizeof(_pool_list_t);
}

/** 将使用完毕的对象放回内存池 */
void pool_put(pool_t self, void* entry) {
    uint32_t cap = self->capacity, size = self->size;
    uint32_t *bits = (uint32_t*)(self->data);
    void* ptr = (void*)bits + (cap >> (PB - PBB));
    // 如果要归还的项在缓冲范围，则设置位图，否则对链表进行释放
    if (entry >= ptr && entry < ptr + cap * size) {
        size_t i = (entry - ptr) / size;
        uint32_t last = i >> PB;
        if (self->last > last) self->last = last;
        bits[last] &= ~(1 << (i & PM));
    } else {
        _pool_list_t *head = &self->head, *pos = head->next, *node = entry - sizeof(_pool_list_t);
        for (; pos != head; pos = pos->next) {
            if (pos == node) {
                pos->next->prev = pos->prev;
                pos->prev->next = pos->next;
                free(pos);
                break;
            }
        }
    }
}

//==========================================================================
// #define TEST_POOL
#ifdef TEST_POOL
#include <stdio.h>
#include <assert.h>

void check_pool(char** bs) {
    for (int i = 0; i < 95; i++) {
        assert(bs[i] + 1 == bs[i + 1]);
        assert(*bs[i] == i);
    }
}

int main() {
    pool_t pool = pool_malloc(64, 1);
    assert(pool->capacity == 64);
    pool_free(pool);

    pool = pool_malloc(65, 1);
    assert(pool->capacity == 96);

    char *bs[96];
    for (int i = 0; i < 96; i++) {
        bs[i] = pool_get(pool);
        *bs[i] = i;
        assert(pool->last == i >> PB);
    }
    check_pool(bs);

    assert(pool->last == 95 >> PB);
    assert(pool_tryget(pool) == NULL);
    assert(pool->last == 96 >> PB);

    pool_put(pool, bs[38]);
    assert(pool->last == 38 >> PB);
    pool_put(pool, bs[3]);
    assert(pool->last == 3 >> PB);
    pool_put(pool, bs[94]);
    assert(pool->last == 3 >> PB);

    bs[3] = pool_get(pool);
    bs[38] = pool_get(pool);
    bs[94] = pool_get(pool);
    check_pool(bs);
    assert(pool_tryget(pool) == NULL);

    pool_free(pool);
    printf("test success");
}
#endif // TEST_POOL