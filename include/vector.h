
#ifndef VECTOR_H
#define VECTOR_H

/** this structure are protected */
typedef struct vector_s vector_t;
/** this structure are protected */
typedef struct vector_iterator_s vector_iterator_t;

/** create vector_t struct */
vector_t* vector_create(int blks, void (*destroy_data_f)(void*));

/** destroy vector_t struct */
void vector_destroy(void* data);

/** set to vector */
int set_to_vector(vector_t* vector, void* data);

/** delete from vector */
int delete_from_vector(vector_t* vector, void* data);

/** delete node from vector */
int delete_node_from_vector(vector_t* vector, vector_iterator_t* it);

/** clear vector moode: 0-no use dstroy_data_t !0-use destroy_data_t*/
int vector_clear(vector_t* vector, int mode);

/** create vector_iterator_t struct */
vector_iterator_t* vector_iterator_create(vector_t* vector);

/** set vector_iterator_t struct */
int vector_iterator_set(vector_iterator_t* it, vector_t* vector);

/** iterate vector elements */
void* vector_iterate(vector_iterator_t* it);

/** destroy vector_iterator_t struct */
void vector_iterator_destroy(void* data);

/** resize vector */
int vector_resize(vector_t* vector, int size);

/** get vector used */
int vector_used(vector_t* vector);

/** get vector size */
int vector_size(vector_t* vector);

#endif // VECTOR_H
