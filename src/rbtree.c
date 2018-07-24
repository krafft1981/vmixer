
#include <string.h>
#include <stdlib.h>
#include "vector.h"
#include "rbtree.h"

#define RED_COLOR 'r'
#define BLACK_COLOR 'b'

typedef struct rbtree_entry_s rbtree_entry_t;

struct rbtree_s {

	rbtree_entry_t* root;

	void (*destroy_key_f) (void* key);
	void (*destroy_data_f) (void* data);
	int size;
};

struct rbtree_entry_s {

	char* key;
	void* data;

	rbtree_entry_t* left;
	rbtree_entry_t* right;
	rbtree_entry_t* parrent;

	char colour;
};

struct rbtree_iterator_s {

	rbtree_t* rbtree;
	vector_t* nodes;
	vector_iterator_t* it;
};

rbtree_entry_t RBTREE_NODE_INITIALIZER = { "", NULL, &RBTREE_NODE_INITIALIZER, &RBTREE_NODE_INITIALIZER, NULL, BLACK_COLOR };

void rbtree_recursive_destroy(rbtree_t *tree, rbtree_entry_t* entry, int mode) {

	if (entry != &RBTREE_NODE_INITIALIZER) {
		if (entry->left != &RBTREE_NODE_INITIALIZER)
			rbtree_recursive_destroy(tree, entry->left, mode);

		if (entry->right != &RBTREE_NODE_INITIALIZER)
			rbtree_recursive_destroy(tree, entry->right, mode);

		if (mode) {
			if (tree->destroy_key_f)
				tree->destroy_key_f(entry->key);

			if (tree->destroy_data_f)
				tree->destroy_data_f(entry->data);
		}

		free(entry);
	}
}

int rbtree_clear(rbtree_t* tree, int mode) {

	if (!tree)
		return -1;

	rbtree_recursive_destroy(tree, tree->root, mode);
	tree->root = &RBTREE_NODE_INITIALIZER;
	tree->size = 0;

	return 0;
}

static void rotate_left_rbtree(rbtree_t* tree, rbtree_entry_t* node) {

	rbtree_entry_t* x = node->right;
	node->right = x->left;

	if (x->left != &RBTREE_NODE_INITIALIZER)
		x->left->parrent = node;

	if (x != &RBTREE_NODE_INITIALIZER)
		x->parrent = node->parrent;

	if (node->parrent) {
		if (node == node->parrent->left)
			node->parrent->left = x;
		else	node->parrent->right = x;
	}

	else
		tree->root = x;

	x->left = node;

	if (node != &RBTREE_NODE_INITIALIZER)
		node->parrent = x;
}

static void rotate_right_rbtree(rbtree_t* tree, rbtree_entry_t* node) {

	rbtree_entry_t* x = node->left;
	node->left = x->right;

	if (x->right != &RBTREE_NODE_INITIALIZER)
		x->right->parrent = node;

	if (x != &RBTREE_NODE_INITIALIZER)
		x->parrent = node->parrent;

	if (node->parrent) {
		if (node == node->parrent->right)
			node->parrent->right = x;
		else	node->parrent->left = x;
	}

	else
		tree->root = x;

	x->right = node;

	if (node != &RBTREE_NODE_INITIALIZER)
		node->parrent = x;
}

static int rbtree_walker(rbtree_t* tree, rbtree_entry_t* entry, vector_t* vector) {

	if (!tree)
		return -1;

	if (!entry)
		entry = tree->root;

	if (entry == &RBTREE_NODE_INITIALIZER)
		return 0;

	if (entry->left != &RBTREE_NODE_INITIALIZER)
		rbtree_walker(tree, entry->left, vector);

	if (entry->right != &RBTREE_NODE_INITIALIZER)
		rbtree_walker(tree, entry->right, vector);

	return set_to_vector(vector, entry);
}

rbtree_t* rbtree_create(void (*destroy_key_f) (void*), void (*destroy_data_f)(void*)) {

	rbtree_t* tree = calloc(1, sizeof(*tree));
	if (tree) {
		tree->root = &RBTREE_NODE_INITIALIZER;
		tree->destroy_key_f  = destroy_key_f;
		tree->destroy_data_f = destroy_data_f;
	}
	return tree;
}

void rbtree_destroy(void* data) {

	if (!data)
		return;

	rbtree_t* tree = data;
	rbtree_recursive_destroy(tree, tree->root, !0);
	free(data);
}

int set_to_rbtree(rbtree_t* tree, char* key, void* data) {

	if (!tree || !key)
		return -1;

	rbtree_entry_t *current = tree->root;
	rbtree_entry_t *parrent = NULL;

	while (current != &RBTREE_NODE_INITIALIZER) {
		if (!strcmp(key, current->key)) {

			if (tree->destroy_key_f)
				tree->destroy_key_f(current->key);

			if (tree->destroy_data_f)
				tree->destroy_data_f(current->data);

			current->key = key;
			current->data = data;
			return 0;
		}

		parrent = current;
		current = strcmp(key, current->key) < 0 ? current->left : current->right;
	}

	rbtree_entry_t *node = malloc(sizeof(*node));
	if (!node)
		return -1;

	node->key = key;
	node->data = data;

	node->left = &RBTREE_NODE_INITIALIZER;
	node->right = &RBTREE_NODE_INITIALIZER;
	node->parrent = parrent;

	node->colour = RED_COLOR;

	if (parrent)
		if (strcmp(key, parrent->key) < 0)
			parrent->left = node;
		else	parrent->right = node;

	else
		tree->root = node;

	while (node != tree->root && node->parrent->colour == RED_COLOR) {
		if (node->parrent == node->parrent->parrent->left) {
			rbtree_entry_t *y = node->parrent->parrent->right;

			if (y->colour == RED_COLOR) {
				node->parrent->colour = BLACK_COLOR;
				y->colour = BLACK_COLOR;
				node->parrent->parrent->colour = RED_COLOR;
				node = node->parrent->parrent;
			}

			else {
				if (node == node->parrent->right) {
					node = node->parrent;
					rotate_left_rbtree(tree, node);
				}

				node->parrent->colour = BLACK_COLOR;
				node->parrent->parrent->colour = RED_COLOR;
				rotate_right_rbtree(tree, node->parrent->parrent);
			}
		}

		else {
			rbtree_entry_t *y = node->parrent->parrent->left;

			if (y->colour == RED_COLOR) {
				node->parrent->colour = BLACK_COLOR;
				y->colour = BLACK_COLOR;
				node->parrent->parrent->colour = RED_COLOR;
				node = node->parrent->parrent;
			}

			else {
				if (node == node->parrent->left) {
					node = node->parrent;
					rotate_right_rbtree(tree, node);
				}

				node->parrent->colour = BLACK_COLOR;
				node->parrent->parrent->colour = RED_COLOR;
				rotate_left_rbtree(tree, node->parrent->parrent);
			}
		}
	}

	tree->root->colour = BLACK_COLOR;
	tree->size++;
	return 0;
}

void* get_from_rbtree(rbtree_t* tree, const char* key) {

	if (!tree || !key)
		return NULL;

	rbtree_entry_t* current = tree->root;
	while (current != &RBTREE_NODE_INITIALIZER) {

		if (!strcmp(key, current->key))
			return current->data;

		current = strcmp(key, current->key) < 0 ? current->left : current->right;
	}

	return NULL;
}

int delete_from_rbtree(rbtree_t* tree, const char* key) {

	if (!tree || !key)
		return -1;

	rbtree_entry_t* node = get_from_rbtree(tree, key);
	if (!node)
		return -1;

	rbtree_entry_t *x, *y;

	if (node->left == &RBTREE_NODE_INITIALIZER || node->right == &RBTREE_NODE_INITIALIZER)
		y = node;

	else {
		y = node->right;
		while (y->left != &RBTREE_NODE_INITIALIZER)
			y = y->left;
	}

	if (y->left != &RBTREE_NODE_INITIALIZER)
		x = y->left;
	else	x = y->right;

	x->parrent = y->parrent;

	if (y->parrent)
		if (y == y->parrent->left)
			y->parrent->left = x;
		else	y->parrent->right = x;

	else
		tree->root = x;

	if (y != node)
		node->data = y->data;

	if (y->colour == BLACK_COLOR) {
		while (x != tree->root && x->colour == BLACK_COLOR) {
			if (x == x->parrent->left) {
				rbtree_entry_t *w = x->parrent->right;
				if (w->colour == RED_COLOR) {
					w->colour = BLACK_COLOR;
					x->parrent->colour = RED_COLOR;
					rotate_left_rbtree (tree, x->parrent);
					w = x->parrent->right;
				}

				if (w->left->colour == BLACK_COLOR && w->right->colour == BLACK_COLOR) {
					w->colour = RED_COLOR;
					x = x->parrent;
				}

				else {
					if (w->right->colour == BLACK_COLOR) {
						w->left->colour = BLACK_COLOR;
						w->colour = RED_COLOR;
						rotate_right_rbtree (tree, w);
						w = x->parrent->right;
					}

					w->colour = x->parrent->colour;
					x->parrent->colour = BLACK_COLOR;
					w->right->colour = BLACK_COLOR;
					rotate_left_rbtree(tree, x->parrent);
					x = tree->root;
				}
			}

			else {
				rbtree_entry_t *w = x->parrent->left;
				if (w->colour == RED_COLOR) {
					w->colour = BLACK_COLOR;
					x->parrent->colour = RED_COLOR;
					rotate_right_rbtree (tree, x->parrent);
					w = x->parrent->left;
				}

				if (w->right->colour == BLACK_COLOR && w->left->colour == BLACK_COLOR) {
					w->colour = RED_COLOR;
					x = x->parrent;
				}

				else {
					if (w->left->colour == BLACK_COLOR) {
						w->right->colour = BLACK_COLOR;
						w->colour = RED_COLOR;
						rotate_left_rbtree (tree, w);
						w = x->parrent->left;
					}

					w->colour = x->parrent->colour;
					x->parrent->colour = BLACK_COLOR;
					w->left->colour = BLACK_COLOR;
					rotate_right_rbtree (tree, x->parrent);
					x = tree->root;
				}
			}
		}

		x->colour = BLACK_COLOR;
	}

	if (tree->destroy_key_f)
		tree->destroy_key_f(y->key);

	if (tree->destroy_data_f)
		tree->destroy_data_f(y->data);

	tree->size--;
	free(y);
	return 0;
}

int rbtree_size(rbtree_t* rbtree) {

	return rbtree->size;
}

rbtree_iterator_t* rbtree_iterator_create(rbtree_t* rbtree) {

	if (!rbtree)
		return NULL;

	rbtree_iterator_t* it = calloc(1, sizeof(*it));
	if (it)
		rbtree_iterator_set(it, rbtree);

	return it;
}

int rbtree_iterator_set(rbtree_iterator_t* it, rbtree_t* rbtree) {

	if (!it || !rbtree)
		return -1;

	it->rbtree = rbtree;
	if (it->nodes)
		vector_destroy(it->nodes);
	it->nodes = vector_create(1, NULL);

	rbtree_walker(rbtree, NULL, it->nodes);

	if (it->it)
		vector_iterator_destroy(it->it);
	it->it = vector_iterator_create(it->nodes);

	return 0;
}

void rbtree_iterator_destroy(void* data) {

	if (data) {
		vector_iterator_destroy(((rbtree_iterator_t*)data)->it);
		vector_destroy(((rbtree_iterator_t*)data)->nodes);
		free(data);
	}
}

void* rbtree_iterate(rbtree_iterator_t* it, const char* *key, void* *data) {

	if (!it)
		return NULL;

	rbtree_entry_t* entry = vector_iterate(it->it);
	if (entry) {
		if (key) *key = entry->key;
		if (data) *data = entry->data;
	}

	return entry;
}
