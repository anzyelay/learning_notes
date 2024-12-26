#ifndef __AVL_TREE_H__
#define __AVL_TREE_H__

#define AVL_DEBUG_TEST 1
typedef struct avl_node {
    struct avl_node *left, *right;
    int height;
	struct avl_node *prev, *next;
}avl_node_t, *pavl_node_t;

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({ \
                            const typeof( ((type *)0)->member ) *__mptr = (ptr); \
                            (type *)(  (ptr) ? (char *)__mptr - offsetof(type,member) : NULL);})
#define node_entry(ptr, type, member)  container_of(ptr, type, member)
#define avl_node_next(n)	((n) ? (n)->next : NULL)
#define avl_node_prev(n)	((n) ? (n)->prev : NULL)

#define list_for_each_entry(pos, tree, member)   \
	for (pos=node_entry((tree)->first, typeof(*(pos)), member); \
			pos; \
			pos = node_entry(avl_node_next(&pos->member), typeof(*pos), member))

#define list_for_each_entry_safe(pos, tmp, tree, member)   \
	for (pos=node_entry((tree)->first, typeof(*(pos)), member), \
			tmp = pos ? node_entry(avl_node_next(&pos->member), typeof(*pos), member) : NULL; \
			pos; \
			pos = tmp,  \
			tmp = pos ? node_entry(avl_node_next(&pos->member), typeof(*pos), member) : NULL)

#define list_for_each_entry_reverse(pos, tree, member)   \
	for (pos=node_entry((tree)->last, typeof(*(pos)), member); \
			pos; \
			pos = node_entry(avl_node_prev(&pos->member), typeof(*pos), member))

#define list_for_each_entry_reverse_safe(pos, tmp, tree, member)   \
	for (pos=node_entry((tree)->last, typeof(*(pos)), member), \
			tmp = pos ? node_entry(avl_node_prev(&pos->member), typeof(*pos), member) : NULL; \
			pos; \
			pos = tmp, \
			tmp = pos ? node_entry(avl_node_prev(&pos->member), typeof(*pos), member) : NULL)


typedef int (*p_node_cmp_fun)(struct avl_node *n1, struct avl_node *n2, void* aux);
typedef void (*p_print_data_fun)(pavl_node_t);

typedef struct avl_tree {
    struct avl_node *root;
    // if n1 == n2 return 0, if n1 < n2 return positive , if n1 > n2 return negtive
    // return (n2 - n1)
    p_node_cmp_fun cmp_fun;
    p_print_data_fun print;
	struct avl_node *first, *last;
    void *aux;
}avl_tree_t, *pavl_tree_t;

pavl_tree_t avl_tree_new(p_node_cmp_fun f);
void avl_tree_free(pavl_tree_t);
void avl_tree_set_cmp_fun(pavl_tree_t tree, p_node_cmp_fun f, void *aux);
void avl_tree_set_print_fun(pavl_tree_t tree, p_print_data_fun f);
pavl_node_t avl_search(pavl_tree_t tree, pavl_node_t node);
int avl_insert(pavl_tree_t tree, pavl_node_t node);
int avl_height(pavl_tree_t tree);
void avl_preorder(pavl_tree_t tree);
void avl_postorder(pavl_tree_t tree);
void avl_midorder(pavl_tree_t tree);
void avl_travel_print(pavl_tree_t tree);
pavl_node_t avl_far_left(pavl_tree_t tree);
pavl_node_t avl_far_right(pavl_tree_t tree);
// pavl_node_t avl_node_prev(pavl_node_t node);
// pavl_node_t avl_node_next(pavl_node_t node);

#ifdef AVL_DEBUG_TEST
int avl_test(void);
#endif

#endif
