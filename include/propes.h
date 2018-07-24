
#ifndef PROPES_H
#define PROPES_H

/** this structure are protected */
typedef struct rbtree_s propes_t;

/** this structure are protected */
typedef struct property_s property_t;

struct property_s {

	char* name;
	char* defval;
	char* description;
	const char* (*check)(property_t* property, const char* value);
};

/** create propes_t struct */
propes_t* propes_create(property_t* list);

/** destroy parray_t struct */
void propes_destroy(void* data);

/** get property value*/
const char* get_from_propes(propes_t* propes, const char* name);

/**set property value*/
int set_to_propes(propes_t* propes, const char* name, const char* value);

/** property type validator (none) */
const char* check_type_str(property_t* property, const char* value);

/** property type validator (int) */
const char* check_type_int(property_t* property, const char* value);

/** property type validator (ip) */
const char* check_type_ip(property_t* property, const char* value);

/** property type validator (node) */
const char* check_type_node(property_t* property, const char* value);

/** property type validator (variant) */
const char* check_type_variant(property_t* property, const char* value);

#endif // PROPES_H
