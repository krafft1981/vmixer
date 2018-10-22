
#ifndef PARSER_H
#define PARSER_H

#include <rbtree.h>
#include <vector.h>

/** protected structure */
typedef struct parser_s parser_t;
/** protected structure */
typedef struct json_node_s json_node_t;

typedef enum json_node_type_e json_node_type_t;
typedef enum json_style_e json_style_t;

enum json_node_type_e {

	JSON_NODE_TYPE_NULL    = 0,
	JSON_NODE_TYPE_BOOL    = 1,
	JSON_NODE_TYPE_INTEGER = 2,
	JSON_NODE_TYPE_DOUBLE  = 3,
	JSON_NODE_TYPE_STRING  = 4,
	JSON_NODE_TYPE_OBJECT  = 5,
	JSON_NODE_TYPE_ARRAY   = 6,
	JSON_NODE_TYPE_ANY     = 7,
};

enum json_style_e {

	JSON_STYLE_MINIMAL = 0,
	JSON_STYLE_HUMAN   = 1
};

/** create parser */
parser_t* parser_create();

/** destroy parser */
void parser_destroy(void* data);

/** parse file to json_node_t */
json_node_t* parser_parse_file(parser_t* parser, const char* file);

/** parse buffer to json_node_t */
json_node_t* parser_parse_buffer(parser_t* parser, const char* buffer, int len);

/** parse string to json_node_t */
json_node_t* parser_parse_string(parser_t* parser, const char* str);

/** destroy json_node_t */
void json_node_destroy(void* data);

/** print json node to char */
int json_node_print(json_node_t* node, json_style_t style, int* len, char* str);

/** Create json_node_t* type JSON_NODE_TYPE_OBJECT */
json_node_t* json_node_object(rbtree_t* tree);

/** get node with name:type from object*/
json_node_t* json_node_object_node(json_node_t* node, const char* name, json_node_type_t type);

/** Create json_node_t* type JSON_NODE_TYPE_ARRAY */
json_node_t* json_node_array(vector_t* vector);

/** get node value from array by index*/
json_node_t* json_node_array_node(json_node_t* node, int id);

/** Create json_node_t* type JSON_NODE_TYPE_STRING */
json_node_t* json_node_string(const char* value);

/** get char value from JSON_NODE_TYPE_STRING */
const char* json_node_string_value(json_node_t* node);

/** Create json_node_t* type JSON_NODE_TYPE_INTEGER */
json_node_t* json_node_int(int value);

/** get int value from JSON_NODE_TYPE_INTEGER */
int json_node_int_value(json_node_t* node);

/** Create json_node_t* type JSON_NODE_TYPE_DOUBLE */
json_node_t* json_node_double(double value);

/** get double value from JSON_NODE_TYPE_DOUBLE */
double json_node_double_value(json_node_t* node);

/** Create json_node_t* type JSON_NODE_TYPE_BOOL */
json_node_t* json_node_bool(int value);

/** get bool value from JSON_NODE_TYPE_BOOL */
int json_node_bool_value(json_node_t* node);

/** Create json_node_t* type JSON_NODE_TYPE_NULL */
json_node_t* json_node_null();

/** get json node type */
json_node_type_t json_node_type(json_node_t* node);

/** get json node type str */
const char* json_node_type_str(json_node_t* node);

/** get json array element count */
int json_node_array_count(json_node_t* node);

/** get json object element count */
int json_node_object_count(json_node_t* node);

/** append child to array node */
int json_node_array_add(json_node_t* node, json_node_t* child);

/** delete child from array node */
int json_node_array_del(json_node_t* node, json_node_t* child);

/** append child to object node */
int json_node_object_add(json_node_t* node, const char* name, json_node_t* child);

/** delete child from object node */
int json_node_object_del(json_node_t* node, const char* name);

#endif // PARSER_H
