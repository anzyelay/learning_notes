#include <stdlib.h>
#include "avl_tree.h"
#include <stdio.h>
#include <stddef.h>

#define avl_log printf
#define MAX(a, b)    (((a) > (b)) ? (a) : (b))

void avl_tree_set_cmp_fun(pavl_tree_t tree, p_node_cmp_fun f, void *aux)
{
    tree->cmp_fun = f;
    tree->aux = aux;
}
void avl_tree_set_print_fun(pavl_tree_t tree, p_print_data_fun f)
{
    tree->print = f;
}

pavl_node_t avl_node_init(pavl_node_t node) {
    node->height = 1;
    node->left = node->right = NULL;
    node->prev = node->next = NULL;
    return node;
}

pavl_node_t _avl_search(pavl_tree_t tree, pavl_node_t cur, pavl_node_t node)
{
    if (cur == NULL) {
        return NULL;
    }

    int cmp = tree->cmp_fun(cur, node, tree->aux);
    if (cmp < 0) {
        return _avl_search(tree, cur->left, node);
    }
    else if (cmp > 0) {
        return _avl_search(tree, cur->right, node);
    }
    else {
        return cur;
    }
}

pavl_node_t avl_search(pavl_tree_t tree, pavl_node_t node)
{
    if (tree->cmp_fun == NULL) {
        avl_log("please implement the cmp_fun member of the avl_tree first.\n");
    }
    return _avl_search(tree, tree->root, node);
}

int _avl_height(pavl_node_t node)
{
    if (node == NULL)
        return 0;
    return node->height;
}

int avl_height(pavl_tree_t tree)
{
    if (tree->root == NULL)
        return 0;
    return _avl_height(tree->root);
}

/**
 * calculate balance factor
*/
int avl_calu_bf(pavl_node_t node)
{
    if (node == NULL)
        return 0;
    return _avl_height(node->left) - _avl_height(node->right);
}

int avl_update_height(pavl_node_t node)
{
    node->height = MAX(_avl_height(node->left), _avl_height(node->right)) + 1;
    return node->height;
}

/**
         n                          c
        / \                       /   \
       c   0                     0     n
      / \         --->          /     / \
     0   r                     i     r   0
    /
   i
*/
pavl_node_t avl_rotate_r(pavl_node_t node)
{
    pavl_node_t c = node->left;
    node->left = c->right;
    c->right = node;

    avl_update_height(node);
    avl_update_height(c);

    return c;
}

/**
         n                          c
        / \                       /   \
       0   c                     n     0
          / \     --->          / \     \
         l   0                 0   l     i
              \
               i
*/
pavl_node_t avl_rotate_l(pavl_node_t node)
{
    pavl_node_t c = node->right;
    node->right = c->left;
    c->left = node;

    avl_update_height(node);
    avl_update_height(c);

    return c;
}

// inline pavl_node_t avl_node_next(pavl_node_t node)
// {
//     if (!node)
//        return NULL;
//     return node->next;
// }

// inline pavl_node_t avl_node_prev(pavl_node_t node)
// {
//     if (!node)
//        return NULL;
//     return node->prev;
// }

int _avl_insert(pavl_tree_t tree, pavl_node_t *pcur, pavl_node_t node)
{
    pavl_node_t cur = *pcur;
    if (cur == NULL) {
        *pcur = avl_node_init(node);
        if (!tree->first) {
            tree->first = *pcur;
            tree->last = *pcur;
        }
        return 1;
    }
    int cmp = tree->cmp_fun(cur, node, tree->aux);
    if (cmp < 0) {
        if (_avl_insert(tree, &cur->left, node)) {
            if (cur == tree->first) {
                tree->first = cur->left;
            }
            pavl_node_t prev = cur->prev;
            if (prev) {
                prev->next = cur->left;
                cur->left->prev = prev;
            }
            cur->left->next = cur;
            cur->prev = cur->left;
        }
    }
    else if (cmp > 0) {
        if (_avl_insert(tree, &cur->right, node)) {
            if (cur == tree->last) {
                tree->last = cur->right;
            }
            pavl_node_t next = cur->next;
            cur->next = cur->right;
            cur->right->prev = cur;
            if (next) {
                cur->right->next = next;
                next->prev = cur->right;
            }
        }
    }
    else {
        // existed, no need to insert
        return 0;
    }

    // update height of cur
    avl_update_height(cur);

    int bf = avl_calu_bf(cur);

    //to balance the tree here
    if (bf < -1 ) {
        bf = avl_calu_bf(cur->right);
        //RL type
        if (bf > 0){
            cur->right = avl_rotate_r(cur->right);
        }
        //RR type
        *pcur = avl_rotate_l(cur);
    }
    else if (bf > 1){
        bf = avl_calu_bf(cur->left);
        //LR type
        if (bf < 0) {
            cur->left = avl_rotate_l(cur->left);
        }
        //LL type
        *pcur = avl_rotate_r(cur);
    }

    return 0;
}

int avl_insert(pavl_tree_t tree, pavl_node_t node)
{
    if (tree->cmp_fun == NULL) {
        avl_log("please implement the cmp_fun member of the avl_tree first.\n");
    }

    return _avl_insert(tree, &tree->root, node);
}

void _avl_preorder(pavl_tree_t tree, pavl_node_t node){
    if (node == NULL) {
        return;
    }
    tree->print(node);
	_avl_preorder(tree, node->left);
	_avl_preorder(tree, node->right);
}

void avl_preorder(pavl_tree_t tree)
{
	if(tree->root==NULL) {
		return;
    }
    avl_log("begin in preorder:\n\t");
    _avl_preorder(tree, tree->root);
    avl_log("\n");
}

void _avl_postorder(pavl_tree_t tree, pavl_node_t node)
{
    if (node == NULL) {
        return;
    }
	_avl_postorder(tree, node->left);
	_avl_postorder(tree, node->right);
    tree->print(node);
}
void avl_postorder(pavl_tree_t tree)
{
	if(tree->root==NULL) {
		return;
    }
    avl_log("begin in postorder:\n\t");
    _avl_postorder(tree, tree->root);
    avl_log("\n");
}
void _avl_midorder(pavl_tree_t tree, pavl_node_t node)
{
    if (node == NULL) {
        return;
    }
	_avl_midorder(tree, node->left);
    tree->print(node);
	_avl_midorder(tree, node->right);
}
void avl_midorder(pavl_tree_t tree)
{
	if(tree->root==NULL) {
		return;
    }
    avl_log("begin in midorder:\n\t");
    _avl_midorder(tree, tree->root);
    avl_log("\n");
}

void _avl_travel_print(pavl_tree_t tree, pavl_node_t n)
{
    if (n == NULL || (n->left == NULL && n->right == NULL)) {
        return;
    }
    avl_log("-- ");
    tree->print(n);
    if (n->left) {
        avl_log(" left node is ");
        tree->print(n->left);
        avl_log(",");
    }
    if (n->right) {
        avl_log("right node is ");
        tree->print(n->right);
    }
    avl_log("\n");

    _avl_travel_print(tree, n->left);
    _avl_travel_print(tree, n->right);
}

void avl_travel_print(pavl_tree_t tree)
{
    if (tree->root == NULL) {
        return;
    }
    tree->print(tree->root);
    avl_log(" is the root\n");
    _avl_travel_print(tree, tree->root);
    avl_log("\n");
}

pavl_node_t avl_node_left(pavl_node_t n)
{
    if (n->left == NULL) {
        return n;
    }
    return avl_node_left(n->left);
}

pavl_node_t avl_far_left(pavl_tree_t tree)
{
    if (tree->root == NULL) {
        return NULL;
    }
    return avl_node_left(tree->root);
}

pavl_node_t avl_node_right(pavl_node_t n)
{
    if (n->right == NULL) {
        return n;
    }
    return avl_node_right(n->right);
}

pavl_node_t avl_far_right(pavl_tree_t tree)
{
    if (tree->root == NULL) {
        return NULL;
    }
    return avl_node_right(tree->root);
}

pavl_tree_t avl_tree_new(p_node_cmp_fun f){
    pavl_tree_t tree = malloc(sizeof(avl_tree_t));
    tree->root = NULL;
    tree->aux = NULL;
    tree->cmp_fun = f;
    tree->print = NULL;
    tree->first = NULL;
    tree->last = NULL;
    return tree;
}

void avl_tree_free(pavl_tree_t tree)
{
    if (tree == NULL)
        return;
    free(tree);
}

#if AVL_DEBUG_TEST

struct mydata {
	int data;
	avl_node_t node;
};

int cmp_fun(pavl_node_t d1, pavl_node_t d2, void *aux)
{
	struct mydata *pd1 = node_entry(d1, struct mydata, node);
	struct mydata *pd2 = node_entry(d2, struct mydata, node);
	return pd2->data - pd1->data;
}
void print_data(pavl_node_t n)
{
	struct mydata *pd1 = node_entry(n, struct mydata, node);
	printf("%02d ", pd1->data);
}

#define array_size_of(m) sizeof((m))/sizeof((m)[0])

int _avl_test(int sample[], int size)
{
    printf("\n=====test begin with size %d=====\nthe raw data:", size);
	pavl_tree_t tree = avl_tree_new(cmp_fun);
	avl_tree_set_print_fun(tree, print_data);

	struct mydata *data = NULL;
	for (int i = 0; i< size; i++) {
        data = malloc(sizeof(struct mydata));
        data->data = sample[i];
		avl_insert(tree, &data->node);
        printf("%d ", sample[i]);
    }

	printf("\n\ntree height is :%d \n", avl_height(tree));

	struct mydata sear = { .data = 10 };
	pavl_node_t n =  avl_search(tree, &sear.node);
	if (n != NULL) {
		printf("the data %d is found by %d times, and its height is %d\n",
			node_entry(n, struct mydata, node)->data, avl_height(tree)-n->height+1, n->height);
	}
	n = avl_far_left(tree);
    if (n)
	    printf("the min data is %d\n", node_entry(n, struct mydata, node)->data);

	n = avl_far_right(tree);
    if (n)
	    printf("the max data is %d\n", node_entry(n, struct mydata, node)->data);

	avl_preorder(tree);
	avl_postorder(tree);
	avl_midorder(tree);
	avl_travel_print(tree);

    printf("begin to iterate data:\n");
    list_for_each_entry(data, tree, node) {
        printf("%d ", data->data);
    }
    printf("\n");

    printf("begin to reverse iterate data:\n");
    list_for_each_entry_reverse(data, tree, node) {
        printf("%d ", data->data);
    }
    printf("\n");

    printf("begin to free data:\n");
    struct mydata *tmp;
    // list_for_each_entry_reverse_safe(data, tmp, tree, node) {
    list_for_each_entry_safe(data, tmp, tree, node) {
        printf("%d ", data->data);
        free(data);
    }
    printf("\n=====end test=====\n");

	avl_tree_free(tree);
	tree = NULL;

	return 0;
}

int avl_test()
{
	int sample[] = {
            10, 13, 18, 32, 1,
            23, 12, 54, 46, 11,
            29, 02, 8, 88, 9,
            7
				};
    _avl_test(sample, 0);
    _avl_test(sample, 1);
    _avl_test(sample, array_size_of(sample));
    return 0;
}

#endif
