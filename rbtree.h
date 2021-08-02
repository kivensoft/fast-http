/*
  Red Black Trees
  (C) 1999  Andrea Arcangeli <andrea@suse.de>
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  linux/include/linux/rbtree.h

  To use rbtrees you'll have to implement your own insert and search cores.
  This will avoid us to use callbacks and to drop drammatically performances.
  I know it's not the cleaner way,  but in C (not in C++) to get
  performances and genericity...

  Some example of insert and search follows here. The search is a plain
  normal search over an ordered tree. The insert instead must be implemented
  in two steps: First, the code must insert the element in order as a red leaf
  in the tree, and then the support library function rb_insert_color() must
  be called. Such function will do the not trivial work to rebalance the
  rbtree, if necessary.

*/

#ifndef __RBTREE_H__
#define __RBTREE_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RB_RED		0
#define RB_BLACK    1

// 节点结构
typedef struct rb_node_t {
	size_t  rb_parent_color;
	struct rb_node_t *rb_right;
	struct rb_node_t *rb_left;
} __attribute__((aligned(sizeof(size_t)))) rb_node_t;

// 根节点结构
typedef struct rb_root_t {
	rb_node_t *rb_node;
} rb_root_t;


#define rb_parent(r)   ((rb_node_t*)((r)->rb_parent_color & ~3))
#define rb_color(r)   ((r)->rb_parent_color & 1)
#define rb_is_red(r)   (!rb_color(r))
#define rb_is_black(r) rb_color(r)
#define rb_set_red(r)  do { (r)->rb_parent_color &= ~1; } while (0)
#define rb_set_black(r)  do { (r)->rb_parent_color |= 1; } while (0)

/** 设置父节点
 * @param dst 要进行设置的节点
 * @param parnet 父节点
*/
inline static void rb_set_parent(rb_node_t *dst, rb_node_t *parent) {
	dst->rb_parent_color = (dst->rb_parent_color & 3) | (size_t) parent;
}

/** 设置父节点颜色
 * @param dst 要进行设置的节点
 * @param color 颜色
*/
inline static void rb_set_color(rb_node_t *dst, int color) {
	dst->rb_parent_color = (dst->rb_parent_color & ~1) | color;
}

#ifndef offsetof
#   define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
#   define container_of(ptr, type, member) ({          	 \
		const typeof(((type*)0)->member) *__mptr = (ptr);    \
		(type*)((char*)__mptr - offsetof(type, member));})
#endif

#define RB_ROOT    (struct rb_root) { NULL, }
#define rb_entry(ptr, type, member) container_of(ptr, type, member)

#define RB_EMPTY_ROOT(root)    ((root)->rb_node == NULL)
#define RB_EMPTY_NODE(node)    (rb_parent(node) == node)
#define RB_CLEAR_NODE(node)    (rb_set_parent(node, node))

// 初始化节点
inline static void rb_init_node(rb_node_t* rb) {
	rb->rb_parent_color = 0;
	rb->rb_right = NULL;
	rb->rb_left = NULL;
	RB_CLEAR_NODE(rb);
}

extern void rb_insert_color(rb_node_t* node, rb_root_t* root);
extern void rb_erase(rb_node_t* node, rb_root_t* root);

typedef void (*rb_augment_f)(rb_node_t* node, void *data);

extern void rb_augment_insert(rb_node_t* node, rb_augment_f func, void *data);
extern rb_node_t *rb_augment_erase_begin(rb_node_t *node);
extern void rb_augment_erase_end(rb_node_t *node, rb_augment_f func, void *data);

/* Find logical next and previous nodes in a tree */
extern rb_node_t *rb_next(const rb_node_t* node);
extern rb_node_t *rb_prev(const rb_node_t* node);
extern rb_node_t *rb_first(const rb_root_t* root);
extern rb_node_t *rb_last(const rb_root_t* root);

/* Fast replacement of a single node without remove/rebalance/add/rebalance */
extern void rb_replace_node(rb_node_t* victim, rb_node_t* node, rb_root_t* root);

inline static void rb_link_node(rb_node_t* node, rb_node_t* parent, rb_node_t** rb_link) {
	node->rb_parent_color = (size_t) parent;
	node->rb_left = node->rb_right = NULL;
	*rb_link = node;
}

// 比较判断回调函数，src < dst 返回-1，src > dst 返回1, 相等返回0
/** 比较判断回调函数, 由用户实现针对key的比较方法
 * @param v1        要进行比较的来源key
 * @param v2        要进行比较的树形节点
 * @return if (v1 < v2) return -1; else if (v1 > v2) return 1; else if (v1 == v2) return 0;
*/
typedef int (*rb_keycmp_fn) (const void* v1, const void* v2);

/** 根据key查找
 * @param root      根
 * @param key       键
 * @param keycmp    比较回调函数
*/
rb_node_t* rb_search(rb_root_t* root, const void* key, rb_keycmp_fn keycmp);

/** 插入新的node
 * @param root 根
 * @param node 新的节点
 * @param keycmp 比较回调函数
*/
bool rb_insert(rb_root_t* root, rb_node_t* node, rb_keycmp_fn keycmp);

int rb_int8cmp(const void* v1, const void* v2);
int rb_uint8cmp(const void* v1, const void* v2);
int rb_int16cmp(const void* v1, const void* v2);
int rb_uint16cmp(const void* v1, const void* v2);
int rb_int32cmp(const void* v1, const void* v2);
int rb_uint32cmp(const void* v1, const void* v2);
int rb_int64cmp(const void* v1, const void* v2);
int rb_uint64cmp(const void* v1, const void* v2);
int rb_strcmp(const void* v1, const void* v2);
int rb_stricmp(const void* v1, const void* v2);

#ifdef __cplusplus
}
#endif

#endif /* __RBTREE_H__ */
