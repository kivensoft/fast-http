#pragma once
#ifndef __LIST_H__
#define __LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#define LIST_FIELDS struct list_head_t *prev, *next

/** 双向链表节点 */
typedef struct list_head_t {
    LIST_FIELDS;
} list_head_t;

/** 初始化节点：设置name节点的前继节点和后继节点都是指向name本身 */
#define LIST_HEAD_INIT(name) { &(name), &(name) }

/* 定义表头(节点)：新建双向链表表头name，并设置name的前继节点和后继节点都是指向name本身 */
#define LIST_HEAD(name) list_head_t name = LIST_HEAD_INIT(name)

/** 初始化节点：将list节点的前继节点和后继节点都是指向list本身
 * @param list      list对象
*/
static inline void list_head_init(list_head_t* list) {
    list->next = list;
    list->prev = list;
}

/** 添加节点：将new插入到prev和next之间
 * @param entry      要插入的节点
 * @param prev      上级节点位置
 * @param next      下级节点位置
*/
static inline void __list_add(list_head_t* entry, list_head_t* prev, list_head_t* next) {
    next->prev = entry;
    entry->next = next;
    entry->prev = prev;
    prev->next = entry;
}

/** 添加节点：将新节点添加到头部
 * @param entry     要插入的节点
 * @param head      插入节点的位置
*/
static inline void list_add(list_head_t* entry, list_head_t* head) {
    __list_add(entry, head, head->next);
}

/** 添加节点：将节点添加到尾部
 * @param entry     要插入的节点
 * @param head      插入节点的位置
 * 
*/
static inline void list_add_tail(list_head_t* entry, list_head_t* head) {
    __list_add(entry, head->prev, head);
}

/** 从双链表中删除节点，将prev与next做首尾相连
 * @param prev      上一个节点
 * @param next      下一个节点
*/
static inline void __list_del(list_head_t* prev, list_head_t* next) {
    next->prev = prev;
    prev->next = next;
}

/** 从双链表中删除entry节点
 * @param entry     要删除的节点
*/
static inline void list_del(list_head_t* entry) {
    __list_del(entry->prev, entry->next);
}

/** 从双链表中删除entry节点，并将entry节点的前继节点和后继节点都指向entry本身
 * @param entry     要删除的节点
*/
static inline void list_del_init(list_head_t* entry) {
    list_del(entry);
    list_head_init(entry);
}

/** 替换节点
 * @param old       被替换的节点
 * @param entry     进行替换的新节点
*/
static inline void list_replace(list_head_t* old, list_head_t* entry) {
    entry->next = old->next;
    entry->next->prev = entry;
    entry->prev = old->prev;
    entry->prev->next = entry;
}

/** 判断双链表是否为空
 * @param head      双向链表的首指针
*/
static inline _Bool list_empty(const list_head_t* head) {
    return head->next == head;
}

/** 获取链表第一项
 * @param head      双向链表的首指针
 * @return 第一项指针，返回void*类型是为了方便转换，使用时要小心
*/
static inline void* list_first(const list_head_t* head) {
    return head->next;
}

/** 获取链表最后一项
 * @param head      双向链表的首指针
 * @return 最后一项指针，返回void*类型是为了方便转换，使用时要小心
*/
static inline void* list_last(const list_head_t* head) {
    return head->prev;
}

/** 获取"MEMBER成员"在"结构体TYPE"中的位置偏移 */
#ifndef offsetof
#   define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

/** 根据"结构体(type)变量"中的"域成员变量(member)的指针(ptr)"来获取指向整个结构体变量的指针 */
#ifndef container_of
#   define container_of(ptr, type, member) ((type *)((char*) ptr - (size_t) &(((type *)0)->member)))
#endif

/** 遍历双向链表 */
#define list_foreach(pos, head) \
    for (pos = (__typeof(pos)) ((head)->next); pos != (__typeof(pos)) (head); pos = (__typeof(pos)) (((list_head_t*)pos)->next))

/** 反向遍历双向链表 */
#define list_foreach_reverse(pos, head) \
    for (pos = (__typeof(pos)) ((head)->prev); pos != (__typeof(pos)) (head); pos = (__typeof(pos)) (((list_head_t*)pos)->prev))

/** 安全的遍历双向链表, 增加一个临时变量用于循环时提前获取下一项，避免pos被改动后获取失败 */
#define list_foreach_safe(pos, tmp, head) \
    for (pos = (__typeof(pos)) ((head)->next), tmp = (__typeof(pos)) (((list_head_t*)pos)->next); \
            pos != (__typeof(pos)) (head); pos = tmp, tmp = (__typeof(pos)) (((list_head_t*)pos)->next))

/** 安全的反向遍历双向链表, 增加一个临时变量用于循环时提前获取下一项，避免pos被改动后获取失败 */
#define list_foreach_reverse_safe(pos, tmp, head) \
    for (pos = (__typeof(pos)) ((head)->prev), tmp = (__typeof(pos)) (((list_head_t*)pos)->prev); \
            pos != (__typeof(pos)) (head); pos = tmp, tmp = (__typeof(pos)) (((list_head_t*)pos)->prev))

/** 根据节点成员变量获取指向双向链表的指针 */
#define list_entry(ptr, type, member) container_of(ptr, type, member)

#ifdef __cplusplus
}
#endif

#endif // __LIST_H__
