
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "config.h"
#include "logger.h"
#include "rbtree.h"
#include "addres.h"
#include "propes.h"

typedef struct property_entry_s property_entry_t;

struct property_entry_s {

	property_t* property;
	char value[VALUE_LEN_MAX];
};

const char* check_type_str(property_t* property, const char* value) {

	return value;
}

const char* check_type_node(property_t* property, const char* value) {

	address_t* address = address_create(value);
	if (address) {
		address_destroy(address);
		return value;
	}

	return property->defval;
}

const char* check_type_int(property_t* property, const char* value) {

	int v;
	if (sscanf(value, "%d", &v) == 1)
		return value;

	return property->defval;
}

const char* check_type_ip(property_t* property, const char* value) {

	struct in_addr addr;
	if (inet_pton(AF_INET, value, &addr))
		return value;

	return property->defval;
}

const char* check_type_variant(property_t* property, const char* value) {

	const char* saparator = "(|)\"";

	char buffer[strlen(property->description) + 1];
	strcpy(buffer, property->description);
	char* ptr = rindex(buffer, '(');
	if (!ptr)
		return property->defval;

	rbtree_t* tree = rbtree_create(NULL, NULL);
	char* token;
	while (token = strtok_r(NULL, saparator, &ptr))
		set_to_rbtree(tree, token, token);

	value = get_from_rbtree(tree, value) ? value : property->defval;
	rbtree_destroy(tree);
	return value;
}

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

	if (entry->property->check)
		value = entry->property->check(entry->property, value);

	strncpy(entry->value, value, sizeof(entry->value));
	return 0;
}
