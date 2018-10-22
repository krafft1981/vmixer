
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "config.h"
#include "logger.h"
#include "rbtree.h"
#include "propes.h"

#define VALUE_MAX_LEN 2048

typedef struct property_entry_s property_entry_t;

struct property_entry_s {

	property_t* property;
	char value[VALUE_MAX_LEN];
};

propes_t* propes_create(property_t* list) {

	if (!list)
		return NULL;

	propes_t* propes = rbtree_create(NULL, free);
	if (!propes)
		return NULL;

	int prop_id = 0;
	while (list[prop_id].name) {
		property_entry_t* entry = calloc(1, sizeof(*entry));
		if (!entry) {
			rbtree_destroy(propes);
			return NULL;
		}

		entry->property = &list[prop_id];
		strncpy(entry->value, entry->property->defval, sizeof(entry->value));
		if (set_to_rbtree(propes, entry->property->name, entry)) {
			free(entry);
			rbtree_destroy(propes);
			return NULL;
		}

		prop_id ++;
	}

	return propes;
}

void propes_destroy(void* data) {

	if (data)
		rbtree_destroy(data);
}

const char* get_from_propes(propes_t* propes, const char* name) {

	property_entry_t* entry = get_from_rbtree(propes, name);
	if (!entry)
		return NULL;

	return entry->value;
}

int set_to_propes(propes_t* propes, const char* name, const char* value) {

	if (!propes || !name || !value)
		return -1;

	property_entry_t* entry = get_from_rbtree(propes, name);
	if (!entry)
		return -1;

	strncpy(entry->value, value, sizeof(entry->value));
	return 0;
}
