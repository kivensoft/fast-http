/*
  Red Black Trees
  (C) 1999  Andrea Arcangeli <andrea@suse.de>
  (C) 2002  David Woodhouse <dwmw2@infradead.org>
  
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

  linux/lib/rbtree.c
*/

#include <string.h>
#include "rbtree.h"

static void __rb_rotate_left(rb_node_t *node, rb_root_t *root) {
	rb_node_t *right = node->rb_right;
	rb_node_t *parent = rb_parent(node);

	if ((node->rb_right = right->rb_left))
		rb_set_parent(right->rb_left, node);
	right->rb_left = node;

	rb_set_parent(right, parent);

	if (parent) {
		if (node == parent->rb_left)
			parent->rb_left = right;
		else
			parent->rb_right = right;
	}
	else
		root->rb_node = right;
	rb_set_parent(node, right);
}

static void __rb_rotate_right(rb_node_t *node, rb_root_t *root) {
	rb_node_t *left = node->rb_left;
	rb_node_t *parent = rb_parent(node);

	if ((node->rb_left = left->rb_right))
		rb_set_parent(left->rb_right, node);
	left->rb_right = node;

	rb_set_parent(left, parent);

	if (parent) {
		if (node == parent->rb_right)
			parent->rb_right = left;
		else
			parent->rb_left = left;
	}
	else
		root->rb_node = left;
	rb_set_parent(node, left);
}

void rb_insert_color(rb_node_t *node, rb_root_t *root) {
	rb_node_t *parent, *gparent;

	while ((parent = rb_parent(node)) && rb_is_red(parent)) {
		gparent = rb_parent(parent);

		if (parent == gparent->rb_left) {
			{
				register rb_node_t *uncle = gparent->rb_right;
				if (uncle && rb_is_red(uncle)) {
					rb_set_black(uncle);
					rb_set_black(parent);
					rb_set_red(gparent);
					node = gparent;
					continue;
				}
			}

			if (parent->rb_right == node) {
				register rb_node_t *tmp;
				__rb_rotate_left(parent, root);
				tmp = parent;
				parent = node;
				node = tmp;
			}

			rb_set_black(parent);
			rb_set_red(gparent);
			__rb_rotate_right(gparent, root);
		} else {
			{
				register rb_node_t *uncle = gparent->rb_left;
				if (uncle && rb_is_red(uncle)) {
					rb_set_black(uncle);
					rb_set_black(parent);
					rb_set_red(gparent);
					node = gparent;
					continue;
				}
			}

			if (parent->rb_left == node) {
				register rb_node_t *tmp;
				__rb_rotate_right(parent, root);
				tmp = parent;
				parent = node;
				node = tmp;
			}

			rb_set_black(parent);
			rb_set_red(gparent);
			__rb_rotate_left(gparent, root);
		}
	}

	rb_set_black(root->rb_node);
}

static void __rb_erase_color(rb_node_t *node, rb_node_t *parent, rb_root_t *root) {
	rb_node_t *other;

	while ((!node || rb_is_black(node)) && node != root->rb_node) {
		if (parent->rb_left == node) {
			other = parent->rb_right;
			if (rb_is_red(other)) {
				rb_set_black(other);
				rb_set_red(parent);
				__rb_rotate_left(parent, root);
				other = parent->rb_right;
			}
			if ((!other->rb_left || rb_is_black(other->rb_left)) &&
				(!other->rb_right || rb_is_black(other->rb_right))) {
				rb_set_red(other);
				node = parent;
				parent = rb_parent(node);
			} else {
				if (!other->rb_right || rb_is_black(other->rb_right)) {
					rb_set_black(other->rb_left);
					rb_set_red(other);
					__rb_rotate_right(other, root);
					other = parent->rb_right;
				}
				rb_set_color(other, rb_color(parent));
				rb_set_black(parent);
				rb_set_black(other->rb_right);
				__rb_rotate_left(parent, root);
				node = root->rb_node;
				break;
			}
		} else {
			other = parent->rb_left;
			if (rb_is_red(other)) {
				rb_set_black(other);
				rb_set_red(parent);
				__rb_rotate_right(parent, root);
				other = parent->rb_left;
			}
			if ((!other->rb_left || rb_is_black(other->rb_left)) &&
				(!other->rb_right || rb_is_black(other->rb_right))) {
				rb_set_red(other);
				node = parent;
				parent = rb_parent(node);
			} else {
				if (!other->rb_left || rb_is_black(other->rb_left)) {
					rb_set_black(other->rb_right);
					rb_set_red(other);
					__rb_rotate_left(other, root);
					other = parent->rb_left;
				}
				rb_set_color(other, rb_color(parent));
				rb_set_black(parent);
				rb_set_black(other->rb_left);
				__rb_rotate_right(parent, root);
				node = root->rb_node;
				break;
			}
		}
	}
	if (node)
		rb_set_black(node);
}

void rb_erase(rb_node_t *node, rb_root_t *root) {
	rb_node_t *child, *parent;
	int color;

	if (!node->rb_left)
		child = node->rb_right;
	else if (!node->rb_right)
		child = node->rb_left;
	else {
		rb_node_t *old = node, *left;

		node = node->rb_right;
		while ((left = node->rb_left) != NULL)
			node = left;

		if (rb_parent(old)) {
			if (rb_parent(old)->rb_left == old)
				rb_parent(old)->rb_left = node;
			else
				rb_parent(old)->rb_right = node;
		} else
			root->rb_node = node;

		child = node->rb_right;
		parent = rb_parent(node);
		color = rb_color(node);

		if (parent == old) {
			parent = node;
		} else {
			if (child)
				rb_set_parent(child, parent);
			parent->rb_left = child;

			node->rb_right = old->rb_right;
			rb_set_parent(old->rb_right, node);
		}

		node->rb_parent_color = old->rb_parent_color;
		node->rb_left = old->rb_left;
		rb_set_parent(old->rb_left, node);

		goto color;
	}

	parent = rb_parent(node);
	color = rb_color(node);

	if (child)
		rb_set_parent(child, parent);
	if (parent) {
		if (parent->rb_left == node)
			parent->rb_left = child;
		else
			parent->rb_right = child;
	}
	else
		root->rb_node = child;

 color:
	if (color == RB_BLACK)
		__rb_erase_color(child, parent, root);
}

static void rb_augment_path(rb_node_t *node, rb_augment_f func, void *data) {
	rb_node_t *parent;

up:
	func(node, data);
	parent = rb_parent(node);
	if (!parent)
		return;

	if (node == parent->rb_left && parent->rb_right)
		func(parent->rb_right, data);
	else if (parent->rb_left)
		func(parent->rb_left, data);

	node = parent;
	goto up;
}

/*
 * after inserting @node into the tree, update the tree to account for
 * both the new entry and any damage done by rebalance
 */
void rb_augment_insert(rb_node_t *node, rb_augment_f func, void *data) {
	if (node->rb_left)
		node = node->rb_left;
	else if (node->rb_right)
		node = node->rb_right;

	rb_augment_path(node, func, data);
}

/*
 * before removing the node, find the deepest node on the rebalance path
 * that will still be there after @node gets removed
 */
rb_node_t *rb_augment_erase_begin(rb_node_t *node) {
	rb_node_t *deepest;

	if (!node->rb_right && !node->rb_left)
		deepest = rb_parent(node);
	else if (!node->rb_right)
		deepest = node->rb_left;
	else if (!node->rb_left)
		deepest = node->rb_right;
	else {
		deepest = rb_next(node);
		if (deepest->rb_right)
			deepest = deepest->rb_right;
		else if (rb_parent(deepest) != node)
			deepest = rb_parent(deepest);
	}

	return deepest;
}

/*
 * after removal, update the tree to account for the removed entry
 * and any rebalance damage.
 */
void rb_augment_erase_end(rb_node_t *node, rb_augment_f func, void *data) {
	if (node)
		rb_augment_path(node, func, data);
}

/*
 * This function returns the first node (in sort order) of the tree.
 */
rb_node_t *rb_first(const rb_root_t *root) {
	rb_node_t *node = root->rb_node;
	if (node)
		while (node->rb_left)
			node = node->rb_left;
	return node;
}

rb_node_t *rb_last(const rb_root_t *root) {
	rb_node_t *node = root->rb_node;
	if (node)
		while (node->rb_right)
			node = node->rb_right;
	return node;
}

rb_node_t *rb_next(const rb_node_t *node) {
	rb_node_t *parent;

	if (rb_parent(node) == node)
		return NULL;

	/* If we have a right-hand child, go down and then left as far
	   as we can. */
	if (node->rb_right) {
		node = node->rb_right; 
		while (node->rb_left)
			node=node->rb_left;
		return (rb_node_t *)node;
	}

	/* No right-hand children.  Everything down and left is
	   smaller than us, so any 'next' node must be in the general
	   direction of our parent. Go up the tree; any time the
	   ancestor is a right-hand child of its parent, keep going
	   up. First time it's a left-hand child of its parent, said
	   parent is our 'next' node. */
	while ((parent = rb_parent(node)) && node == parent->rb_right)
		node = parent;

	return parent;
}

rb_node_t *rb_prev(const rb_node_t *node) {
	rb_node_t *parent;

	if (rb_parent(node) == node)
		return NULL;

	/* If we have a left-hand child, go down and then right as far
	   as we can. */
	if (node->rb_left) {
		node = node->rb_left; 
		while (node->rb_right)
			node=node->rb_right;
		return (rb_node_t *)node;
	}

	/* No left-hand children. Go up till we find an ancestor which
	   is a right-hand child of its parent */
	while ((parent = rb_parent(node)) && node == parent->rb_left)
		node = parent;

	return parent;
}

void rb_replace_node(rb_node_t *victim, rb_node_t *node, rb_root_t *root) {
	rb_node_t *parent = rb_parent(victim);

	/* Set the surrounding nodes to point to the replacement */
	if (parent) {
		if (victim == parent->rb_left)
			parent->rb_left = node;
		else
			parent->rb_right = node;
	} else {
		root->rb_node = node;
	}
	if (victim->rb_left)
		rb_set_parent(victim->rb_left, node);
	if (victim->rb_right)
		rb_set_parent(victim->rb_right, node);

	/* Copy the pointers/colour from the victim to the replacement */
	*node = *victim;
}

#define _MY_CMP(type, v1, v2) *(type*)(v1) < *(type*)(v2) ? -1 : *(type*)(v1) > *(type*)(v2) ? 1 : 0
int rb_int8mp(const void* v1, const void* v2) { return _MY_CMP(int8_t, v1, v2); }
int rb_uint8cmp(const void* v1, const void* v2) { return _MY_CMP(uint8_t, v1, v2); }
int rb_int16cmp(const void* v1, const void* v2) { return _MY_CMP(int16_t, v1, v2); }
int rb_uint16cmp(const void* v1, const void* v2) { return _MY_CMP(uint16_t, v1, v2); }
int rb_int32cmp(const void* v1, const void* v2) { return _MY_CMP(int32_t, v1, v2); }
int rb_uint32cmp(const void* v1, const void* v2) { return _MY_CMP(uint32_t, v1, v2); }
int rb_int64cmp(const void* v1, const void* v2) { return _MY_CMP(int64_t, v1, v2); }
int rb_uint64cmp(const void* v1, const void* v2) { return _MY_CMP(uint64_t, v1, v2); }
int rb_strcmp(const void* v1, const void* v2) { return strcmp(v1, v2); }
int rb_stricmp(const void* v1, const void* v2) { return stricmp(v1, v2); }

rb_node_t* rb_search(rb_root_t* root, const void* key, rb_keycmp_fn keycmp) {
	for (rb_node_t *node = root->rb_node; node;) {
		int r = keycmp(key, (void*)node + sizeof(rb_node_t));
		if (r < 0)
			node = node->rb_left;
		else if (r > 0)
			node = node->rb_right;
		else
			return node;
	}
	return NULL;
}

bool rb_insert(rb_root_t *root, rb_node_t *node, rb_keycmp_fn keycmp) {
	rb_node_t **cur = &(root->rb_node), *parent = NULL;
	void* key = (void*)node + sizeof(rb_node_t);

	/* Figure out where to put new node */
	while (*cur) {
		int cmp_ret = keycmp(key, (void*)(*cur) + sizeof(rb_node_t));
		parent = *cur;

		if (cmp_ret < 0)
			cur = &(*cur)->rb_left;
		else if (cmp_ret > 0)
			cur = &(*cur)->rb_right;
		else
			return false;
	}

	/* Add new node and rebalance tree. */
	rb_link_node(node, parent, cur);
	rb_insert_color(node, root);

	return true;
}

#ifdef RB_TREE_TEST
//===============================================
#include <stdlib.h>
#include <string.h>
#define NUM_NODES 32

typedef struct mynode {
	rb_node_t node;
	char* key;
	char* value;
	uint32_t age;
} mynode;

rb_root_t mytree;

static int _keycmp(const void* v1, const void* v2) {
	return rb_uint32cmp(v1, v2 + offsetof(mynode, age) - sizeof(rb_node_t));
}

void print_node(rb_node_t *node) {
	int k = -1, p = -1, l = -1, r = -1;
	if (node) {
		k = ((mynode*)node)->age;
		if (rb_parent(node))
			p = ((mynode*) rb_parent(node))->age;
		if (node->rb_left)
			l = ((mynode*)node->rb_left)->age;
		if (node->rb_right)
			r = ((mynode*)node->rb_right)->age;
	}
	printf("key = %d, parent = %d, left = %d, right = %d\n", k, p, l, r);
}

void print_tree() {
	rb_node_t *node = mytree.rb_node;
	printf("root---------\n");
	print_node(node);
	printf("=============\n");
    for (node = rb_first(&mytree); node; node = rb_next(node))
		print_node(node);
	printf("printf end <<<<<<<<<<<<<<<<<<<<\n\n");
}

int main() {
	mytree.rb_node = NULL;
    struct mynode *mn[NUM_NODES];

    /* *insert */
    int i = 0;
    for (; i < NUM_NODES - 1; i++) {
        mn[i] = (mynode*) malloc(sizeof(mynode));
        mn[i]->key = (char*) malloc(6);
		sprintf(mn[i]->key, "key%d", i);
		mn[i]->value = (char*) malloc(8);
		sprintf(mn[i]->value, "value%d", i);
		mn[i]->age = i;
        rb_insert(&mytree, (rb_node_t*) mn[i], _keycmp);
    }
    
	mn[NUM_NODES - 1] = (mynode*) malloc(sizeof(mynode));
	mn[NUM_NODES - 1]->key = (char*) malloc(6);
	sprintf(mn[NUM_NODES - 1]->key, "key%d", 33);
	mn[NUM_NODES - 1]->value = (char*) malloc(8);
	sprintf(mn[NUM_NODES - 1]->value, "value%d", 34);
	mn[NUM_NODES - 1]->age = 15;
	_Bool b = rb_insert(&mytree, (rb_node_t*) mn[NUM_NODES - 1], _keycmp);
	printf("repeat insert 15 is %d\n", b);

    /* *search */
    rb_node_t *node;
	printf("...............\n");
	node = mytree.rb_node;
	printf("root: key = %s, value = %s, age = %d\n", ((mynode*)node)->key, ((mynode*)node)->value, ((mynode*)node)->age);
	printf("===============\n");
    for (node = rb_first(&mytree); node; node = rb_next(node))
        printf("key = %s, value = %s, age = %d\n", ((mynode*)node)->key, ((mynode*)node)->value, ((mynode*)node)->age);

    /* *delete */
    printf("delete node key3: \n");
	uint32_t age = 3;
    mynode *data = (mynode*) rb_search(&mytree, &age, _keycmp);
    if (data) {
        rb_erase(&data->node, &mytree);
        //my_free(data);
    }

    /* *delete again*/
    printf("delete node key11: \n");
	age = 11;
    data = (mynode*) rb_search(&mytree, &age, _keycmp);
    if (data) {
        rb_erase(&data->node, &mytree);
        //my_free(data);
    }


    /* *delete once again*/
    printf("delete node key15: \n");
	age = 15;
    data = (mynode*) rb_search(&mytree, &age, _keycmp);
    if (data) {
        rb_erase(&data->node, &mytree);
        //my_free(data);
    }

    /* *search again*/
    printf("search again:\n");
	node = mytree.rb_node;
	printf("root: key = %s, value = %s, age = %d\n", ((mynode*)node)->key, ((mynode*)node)->value, ((mynode*)node)->age);
	printf("===============\n");
    for (node = rb_first(&mytree); node; node = rb_next(node))
        printf("key = %s, value = %s, age = %d\n", ((mynode*)node)->key, ((mynode*)node)->value, ((mynode*)node)->age);

    return 0;
}

#endif