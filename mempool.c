#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "mempool.h"

#define PB 3
#define PC (1 << PB)
#define PM (PC - 1)

mempool_t* mempool_malloc(uint32_t len, uint32_t size) {
    // bits位图需要的字节数, 分配的长度按8倍数取整
    len = ((len + PM) >> PB) << PB;
    size_t bc = len >> PB;
    mempool_t* pool = (mempool_t*) malloc(sizeof(mempool_t) + bc);
    pool->len = len;
    pool->size = size;
    pool->pointer = malloc(len * size);
    memset(pool->bits, -1, bc);

    return pool;
}

void mempool_free(mempool_t* pool) {
    free(pool->pointer);
    free(pool);
    pool->pointer = NULL;
}

void* mempool_get(mempool_t* pool) {
    // 从位图检索可用的项，找不到的时候，使用系统的malloc分配内存
    for (unsigned i = 0, imax = pool->len >> PB; i < imax; ++i) {
        uint8_t b = pool->bits[i];
        if (b) {
            uint8_t c = 1;
            for (unsigned j = 0; j < PC; ++j) {
                if (b & c) {
                    pool->bits[i] &= ~c;
                    return (void*) (pool->pointer + ((i << PB) | j) * pool->size);
                }
                c <<= 1;
            }
        }
    }

    return malloc(pool->size);
}

void mempool_put(mempool_t* pool, void* entry) {
    // 如果要归还的项在缓冲范围，则设置位图，否则调用系统free进行释放
    uint8_t *p = pool->pointer;
    if ((uint8_t*) entry >= p && (uint8_t*)entry < p + pool->len * pool->size) {
        size_t idx = ((uint8_t*) entry - p) / pool->size;
        pool->bits[idx >> PB] |= 1 << (idx & PM);
    } else {
        free(entry);
    }
}

#ifdef TEST_MEMPOOL
#include <stdio.h>
#define ASSERT(x) if (!(x)) printf("%s:%d -- memarray test fail!\n", __FILE__, __LINE__)
int main() {
    mempool_t* pool = mempool_malloc(7, 1);
    ASSERT(pool->len == 8);
    mempool_free(pool);

    pool = mempool_malloc(10, 1);
    ASSERT(pool->len == 16);
    ASSERT(pool->size == 1);

    mempool_get(pool);
    ASSERT(pool->bits[0] == 0xFE);
    uint8_t* buf1 = mempool_get(pool);
    ASSERT(pool->bits[0] == 0xFC);
    mempool_get(pool);
    ASSERT(pool->bits[0] == 0xF8);

    mempool_put(pool, buf1);
    ASSERT(pool->bits[0] == 0xFA);
    buf1 = mempool_get(pool);
    ASSERT(pool->bits[0] == 0xF8);
    for (int i = 0; i < 7; i++)
        mempool_get(pool);
    ASSERT(pool->bits[0] == 0);
    ASSERT(pool->bits[1] == 0xFC);

    for (int i = 0; i < 6; i++)
        buf1 = mempool_get(pool);
    ASSERT(pool->bits[0] == 0);
    ASSERT(pool->bits[1] == 0);
    ASSERT(buf1 == pool->pointer + 15);

    buf1 = mempool_get(pool);
    ASSERT(buf1 < pool->pointer || buf1 >= pool->pointer + 16);
    mempool_put(pool, buf1);
    ASSERT(pool->bits[0] == 0);
    ASSERT(pool->bits[1] == 0);

    printf("mempool test complete!\n");
}
#endif