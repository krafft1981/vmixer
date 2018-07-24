
#include <stdlib.h>
#include <string.h>
#include "vector.h"

typedef struct vector_entry_s vector_entry_t;
typedef enum vector_entry_used_e vector_entry_used_t;

enum vector_entry_used_e {

	VECTOR_ENTRY_FREE = 0,
	VECTOR_ENTRY_USED = 1
};

struct vector_entry_s {

	void* data;
	vector_entry_used_t used;
};

struct vector_s {

	vector_entry_t* data;
	void (*destroy_data_f) (void*);

	int blks;
	int used;
	int size;
};

struct vector_iterator_s {

	vector_t* vector;
	int id;
};

vector_t* vector_create(int blks, void (*destroy_data_f)(void*)) {

	vector_t* vector = calloc(1, sizeof(*vector));
	if (vector) {
		vector->destroy_data_f = destroy_data_f;
		if (blks <= 1)
			vector->blks = 1;
		else	vector->blks = blks;
	}

	return vector;
}

void vector_destroy(void* data) {

	if (!data)
		return;

	vector_t* vector = data;

	if (vector->destroy_data_f) {
		while (vector->size --) {
			if (vector->data[vector->size].used == VECTOR_ENTRY_USED)
				vector->destroy_data_f(vector->data[vector->size].data);
		}
	}

	if (vector->data)
		free(vector->data);

	free(vector);
}

int vector_clear(vector_t* vector, int mode) {

	if (!vector)
		return -1;

	int size = vector_size(vector);
	while (size --) {
		if (vector->data[size].used == VECTOR_ENTRY_USED) {
			vector->data[size].used = VECTOR_ENTRY_FREE;
			if (mode)
				vector->destroy_data_f(vector->data[size].data);
		}
	}

	vector->used = 0;
	vector_resize(vector, 0);
	return 0;
}

int set_to_vector(vector_t* vector, void* data) {

	if (!vector || !data)
		return -1;

	if (vector->size == vector->used) {
		if (vector_resize(vector, vector->size + vector->blks))
			return -1;
	}

	int id = vector->size;
	while (id --) {
		if (vector->data[id].used == VECTOR_ENTRY_FREE) {
			vector->data[id].used = VECTOR_ENTRY_USED;
			vector->data[id].data = data;
			vector->used++;
			break;
		}
	}

	return 0;
}

int delete_from_vector(vector_t* vector, void* data) {

	if (!vector || !vector->data)
		return -1;

	int id = vector->size;
	while (id --) {
		if (vector->data[id].data == data) {
			if (vector->destroy_data_f)
				vector->destroy_data_f(vector->data[id].data);
			vector->data[id].used = VECTOR_ENTRY_FREE;
			vector->used --;
			return 0;
		}
	}

	return -1;
}

int delete_node_from_vector(vector_t* vector, vector_iterator_t* it) {

	if (!vector || !it)
		return -1;

	vector_entry_t* entry = &vector->data[it->id - 1];
	if (vector->destroy_data_f)
		vector->destroy_data_f(entry);
	entry->used = VECTOR_ENTRY_FREE;
	vector->used --;

	return 0;
}

int vector_used(vector_t* vector) {

	return vector->used;
}

int vector_size(vector_t* vector) {

	return vector->size;
}

int vector_resize(vector_t* vector, int size) {

	if (!vector || size < vector->used)
		return -1;

	if (size > 0) {
		vector_entry_t* data = calloc(size, sizeof(*data));
		if (!data)
			return -1;

		int new_id = 0;
		int old_id = vector->size;
		while (old_id -- ) {
			if (vector->data[old_id].used == VECTOR_ENTRY_USED) {
				data[new_id].data = vector->data[old_id].data;
				data[new_id].used = VECTOR_ENTRY_USED;
				new_id ++;
			}
		}

		if (vector->data)
			free(vector->data);

		vector->data = data;
	}

	else {
		if (vector->data)
			free(vector->data);
		vector->data = NULL;
	}

	vector->size = size;
	return 0;
}

vector_iterator_t* vector_iterator_create(vector_t* vector) {

	if (!vector)
		return NULL;

	vector_iterator_t* it = calloc(1, sizeof(*it));
	if (it)
		vector_iterator_set(it, vector);

	return it;
}

int vector_iterator_set(vector_iterator_t* it, vector_t* vector) {

	if (!it || !vector)
		return -1;

	it->vector = vector;
	it->id = 0;

	return 0;
}

void vector_iterator_destroy(void* data) {

	if (data)
		free(data);
}

void* vector_iterate(vector_iterator_t* it) {

	if (!it)
		return NULL;

	void* data = NULL;

	while (it->id < it->vector->size) {
		if (it->vector->data[it->id].used == VECTOR_ENTRY_USED) {
			data = it->vector->data[it->id].data;
			it->id ++;
			break;
		}

		it->id ++;
	}

	return data;
}
