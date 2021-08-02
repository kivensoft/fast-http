#include <string.h>
#include <memory.h>
#include "hashmap.h"

static void hmap_list_to_rbtree(const hmap_addr_t* addr, rb_keycmp_fn cmp) {
    if (addr->size >= HMAP_LIST_MAX) {
        list_head_t *head = &addr->head, *pos = head->next, *tmp = pos->next;
        rb_root_t *proot = &addr->root;
        rb_init_node(proot);
        for (; pos != head; pos = tmp, tmp = pos->next) {
            void* entry = (void*)pos + sizeof(list_head_t);
            rb_insert(proot, entry, cmp);
        }
    }
}

hmap_key_t* hmap_get(hmap_t* self, const void* key) {
    uint32_t hash = self->hash(key);
    rb_root_t* root = self->nodes + (hash % self->cap);
    return rb_search(root, key, self->cmp);
}

static void _realloc(hmap_t* self) {
    uint32_t max = HMAP_REQ(self->cap);
    if (self->len < max) return;
    uint32_t cap = self->cap << 1, nlen = sizeof(rb_root_t) * cap;

    rb_root_t* new_roots = malloc(nlen);
    memset(new_roots, 0, nlen);

    rb_root_t* root = self->nodes;
    hmap_key_t *p, *tmp;
    for (uint32_t i = 0, n = self->cap; i < n; ++i, ++root) {
        hmap_key_t* p = (hmap_key_t*) rb_first(root);
        
        list_for_each_safe_ex(p, tmp, head) {
            list_del((list_head_t*)p);
            list_head_t* cur = new_head + (p->hash % cap);
            list_add((list_head_t*)p, cur);
        }
    }

    free(self->nodes);
    self->nodes = new_head;
    self->cap = cap;
}

void hmap_put(hmap_t* self, const void* entry) {
    uint32_t hash = self->hash(entry);
    hmap_addr_t* addr = &self->addrs[hash % self->cap];

    if (addr->flag <= HMAP_LIST_MAX) {

    }
    key->hash = hash;
    list_head_t* head = self->nodes + (hash % self->cap);
    hmap_key_t* p;
    hmap_key_equals eq = self->equals;

    list_for_each_ex(p, head) {
        if (p->hash == hash && eq((void*)key + sizeof(hmap_key_t), (void*)p + sizeof(hmap_key_t)))
            break;
    }

    if ((list_head_t*)p != head) {
        self->free(p);
        list_del((list_head_t*)p);
        list_add_tail((list_head_t*)key, head);
    } else {
        self->length++;
        list_add_tail((list_head_t*)key, head);
        _realloc(self);
    }
}

_Bool hmap_del(hmap_t* self, const void* key) {
    hmap_key_t* r = hmap_get(self, key);
    if (r) {
        self->free(r);
        list_del((list_head_t*)r);
        self->len--;
    }
    return r;
}

static void _free_nodes(hmap_t* self) {
    list_head_t* head = self->nodes, *tmp, *p;
    void (*free_fn) (void*) = self->free;
    for (int i = 0, n = self->cap; i < n; ++i, head++) {
        if (head) {
            list_for_each_safe(p, tmp, head) {
                free_fn(p);
            }
        }
    }
}

void hmap_clear(hmap_t* self) {
    if (self->len)
        _free_nodes(self);
    self->len = 0;
    memset(self->nodes, 0, sizeof(list_head_t) * self->cap);
}

void hmap_free(hmap_t* self) {
    if (self->len)
        _free_nodes(self);
    self->len = 0;
    if (self->nodes)
        free(self->nodes);
    self->nodes = NULL;
}

bool hmap_key_int32_equals(const void* v1, const void* v2) {
    return *((int32_t*)v1) == *((int32_t*)v2);
}

bool hmap_key_str_equals(const void* v1, const void* v2) {
    return strcmp((const char*)v1, (const char*)v2) == 0;
}

uint32_t hmap_key_int32_hash(const void* v) {
    return *(uint32_t*)v;
}

uint32_t hmap_key_str_hash(const void* v) {
    uint32_t h = 0;
    const uint8_t* p = v;
    while(*p) h = 31 * h + *p++;
    return h;
}

#include <stdio.h>
#include <stdlib.h>

typedef struct my_data {
    hmap_key_t map_key;
    uint32_t key;
    char* data;
} my_data;

int main() {
    hmap_t int_map;
    hmap_init(&int_map, 3, hmap_key_int32_hash, hmap_key_int32_equals, free);
    printf("init after: cap = %d, len = %d\n", int_map.cap, int_map.len);

    for (uint32_t i = 0; i < 100; ++i) {
        my_data *d = malloc(sizeof(my_data) + 100);
        memset(d, 0, sizeof(my_data) + 100);
        d->key = (i + 1) * 4;
        d->data = (char*)d + sizeof(my_data);
        sprintf(d->data, "key: %d", i);
        hmap_put(&int_map, (hmap_key_t*)d);
    }

    printf("set after: cap = %d, len = %d\n", int_map.cap, int_map.len);

    for (uint32_t i = 10; i < 30; ++i) {
        my_data *a = (my_data*) hmap_get(&int_map, &i);
        if (a)
            printf("find %d: i = %d, data = %s\n", i, a->key, a->data);
        else
            printf("find %d is NULL\n", i);
    }
}