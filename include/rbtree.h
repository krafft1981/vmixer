
#ifndef RBTREE_H
#define RBTREE_H

/** this structure are protected */
typedef struct rbtree_s rbtree_t;

/** this structure are protected */
typedef struct rbtree_iterator_s rbtree_iterator_t;

/** create rbtree object. return NULL if error, rbtree_t* if success */
rbtree_t* rbtree_create(void(*destroy_key_f)(void*), void(*destroy_data_f)(void*));

/** destroy rbtree_t object */
void rbtree_destroy(void* data);

/** clear rbtree_t object */
int rbtree_clear(rbtree_t* tree, int mode);

/** set element to rbtree object. return -1 if error, 0 if success */
int set_to_rbtree(rbtree_t* tree, char* key, void* data);

/** delete from rbtree. return -1 if error, 0 if success */
int delete_from_rbtree(rbtree_t* tree, const char* key);

/** search in rbtree by key. return -1 if error, 0 if success */
void* get_from_rbtree(rbtree_t* tree, const char* key);

/** create rbtree_iterator_t struct */
rbtree_iterator_t* rbtree_iterator_create(rbtree_t* rbtree);

/** set rbtree_iterator_t struct */
int rbtree_iterator_set(rbtree_iterator_t* it, rbtree_t* rbtree);

/** iterate rbtree elements */
void* rbtree_iterate(rbtree_iterator_t* it, const char* *key, void* *data);

/** destroy rbtree_iterator_t struct */
void rbtree_iterator_destroy(void* data);

/** get rbtree size */
int rbtree_size(rbtree_t* rbtree);

#endif // RBTREE_H
