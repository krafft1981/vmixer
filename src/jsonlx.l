
%{
#include <stdarg.h>
#include "jsonpr.h"
#include "logger.h"
%}

%option noinput
%option nounput
%option never-interactive
%option noyywrap
%option reentrant
%option bison-bridge

DIGIT    [0-9]
INTEGER  -?(0|[1-9]{DIGIT}*)
NUMBER   {INTEGER}\.{DIGIT}+
ALPHA    [a-zA-Z]

TRUE [T|t][R|r][U|u][E|e]
FALSE [F|f][A|a][L|l][S|s][E|e]
NULL [N|n][U|u][L|l][L|l]

HEX_DIGIT [0-9a-fA-F]
UNESCAPEDCHAR [ -!#-\[\]-~]
ESCAPEDCHAR \\[\bfnrt\/"]
UNICODECHAR \\u{HEX_DIGIT}{HEX_DIGIT}{HEX_DIGIT}{HEX_DIGIT}
CHAR {UNESCAPEDCHAR}|{ESCAPEDCHAR}|{UNICODECHAR}
CHARS {CHAR}+

%%

"{" { return '{'; };
"}" { return '}'; };
"[" { return '['; };
"]" { return ']'; };
"," { return ','; };
":" { return ':'; };

[ \t\n\r]+ /* ignore whitespace */;

{TRUE} {
	yylval->b = !0;
	return TOKEN_BOOL;
}

{FALSE} {
	yylval->b = 0;
	return TOKEN_BOOL;
}

{NULL} {
	return TOKEN_NULL;
}

{INTEGER} {
	yylval->i = strtod(yytext, NULL);
	return TOKEN_INTEGER;
}

{NUMBER}|({INTEGER}|{NUMBER})[eE][+-]?{INTEGER}+ {
	yylval->d = strtod(yytext, NULL);
	return TOKEN_DOUBLE;
}

\"\" |
\"{CHARS}\" {

	yylval->s = yytext + 1;
	int len = yy_flex_strlen(yytext, NULL);
	if (len)
		yytext[len - 1] = 0;
	return TOKEN_STRING;
};

. { ERROR("json syntax error"); }

%%

void yyerror(yyscan_t scanner, json_node_t* *node, char const* msg) {}

parser_t* parser_create() {

	parser_t* parser = calloc(1, sizeof(parser));
	if (parser) {
		if (yylex_init(&parser->scanner)) {
			free(parser);
			parser = NULL;
		}
	}
	return parser;
}

void parser_destroy(void* data) {

	if (data) {
		parser_t* parser = data;
		yylex_destroy(parser->scanner);
		free(parser);
	}
}

json_node_t* parser_parse_file(parser_t* parser, const char* file) {

	if (!parser || !file)
		return NULL;

	FILE* in = fopen(file, "r");
	if (!in) {
		ERROR("parse '%s': %s", file, strerror(errno));
		return NULL;
	}

	json_node_t* node = NULL;
	yyset_in(in, parser->scanner);
	if (yyparse(parser->scanner, &node)) {
		json_node_destroy(node);
		node = NULL;
	}

	fclose(in);
	return node;
}

json_node_t* parser_parse_buffer(parser_t* parser, const char* buffer, int len) {

	if (!parser || !buffer || !len)
		return NULL;

	json_node_t* node = NULL;
	YY_BUFFER_STATE b = yy_scan_bytes(buffer, len, parser->scanner);
	if (yyparse(parser->scanner, &node)) {
		json_node_destroy(node);
		node = NULL;
	}

	yy_delete_buffer(b, parser->scanner);
	return node;
}

json_node_t* parser_parse_string(parser_t* parser, const char* str) {

	if (!parser || !str)
		return NULL;

	json_node_t* node = NULL;
	YY_BUFFER_STATE buffer = yy_scan_string(str, parser->scanner);
	if (yyparse(parser->scanner, &node)) {
		json_node_destroy(node);
		node = NULL;
	}

	yy_delete_buffer(buffer, parser->scanner);
	return node;
}

json_node_t* json_node_object(rbtree_t* tree) {

	json_node_t* node = calloc(1, sizeof(*node));
	if (node) {
		node->type = JSON_NODE_TYPE_OBJECT;
		if (tree)
			node->v_object = tree;
		else	node->v_object = rbtree_create(free, json_node_destroy);
	}

	return node;
}

json_node_t* json_node_array(vector_t* vector) {

	json_node_t* node = calloc(1, sizeof(*node));
	if (node) {
		node->type = JSON_NODE_TYPE_ARRAY;
		if (vector)
			node->v_array = vector;
		else	node->v_array = vector_create(0, json_node_destroy);
	}

	return node;
}

json_node_t* json_node_string(const char* value) {

	if (!value)
		return NULL;

	json_node_t* node = calloc(1, sizeof(*node));
	if (node) {
		node->type = JSON_NODE_TYPE_STRING;
		node->v_string = strdup(value);
	}

	return node;
}

json_node_t* json_node_int(int value) {

	json_node_t* node = calloc(1, sizeof(*node));
	if (node) {
		node->type = JSON_NODE_TYPE_INTEGER;
		node->v_int = value;
	}

	return node;
}

json_node_t* json_node_null() {

	json_node_t* node = calloc(1, sizeof(*node));
	if (node)
		node->type = JSON_NODE_TYPE_NULL;

	return node;
}

json_node_t* json_node_double(double value) {

	json_node_t* node = calloc(1, sizeof(*node));
	if (node) {
		node->type = JSON_NODE_TYPE_DOUBLE;
		node->v_double = value;
	}

	return node;
}

json_node_t* json_node_bool(int value) {

	json_node_t* node = calloc(1, sizeof(*node));
	if (node) {
		node->type = JSON_NODE_TYPE_BOOL;
		node->v_bool = value;
	}

	return node;
}

json_node_type_t json_node_type(json_node_t* node) {

	if (node)
		return node->type;

	return -1;
}

const char* json_node_type_str(json_node_t* node) {

	if (!node)
		return "fault";

	switch(json_node_type(node)) {
		case JSON_NODE_TYPE_STRING:
			return "STRING";

		case JSON_NODE_TYPE_NULL:
			return "NULL";

		case JSON_NODE_TYPE_BOOL:
			return "BOOL";

		case JSON_NODE_TYPE_INTEGER:
			return "INTEGER";

		case JSON_NODE_TYPE_DOUBLE:
			return "DOUBLE";

		case JSON_NODE_TYPE_OBJECT:
			return "OBJECT";

		case JSON_NODE_TYPE_ARRAY:
			return "ARRAY";

		default:
			return "fault";
	}
}

int json_node_array_count(json_node_t* node) {

	return vector_used(node->v_array);
}

int json_node_object_count(json_node_t* node) {

	return rbtree_size(node->v_object);
}

int json_node_object_add(json_node_t* node, const char* name, json_node_t* child) {

	if (!node || !name || !child || json_node_type(node) != JSON_NODE_TYPE_OBJECT)
		return -1;
	else	return set_to_rbtree(node->v_object, strdup(name), child);
}

int json_node_object_del(json_node_t* node, const char* name) {

	if (!node || !name || json_node_type(node) != JSON_NODE_TYPE_OBJECT)
		return -1;
	else	return delete_from_rbtree(node->v_object, name);
}

int json_node_array_add(json_node_t* node, json_node_t* child) {

	if (!node || !child || json_node_type(node) != JSON_NODE_TYPE_ARRAY)
		return -1;
	else	return set_to_vector(node->v_array, node);
}

int json_node_array_del(json_node_t* node, json_node_t* child) {

	if (!node || !child || json_node_type(node) != JSON_NODE_TYPE_ARRAY)
		return -1;
	else	return delete_from_vector(node->v_array, child);
}

void json_node_destroy(void* data) {

	if (!data)
		return;

	json_node_t* node = data;
	switch (json_node_type(data)) {

		case JSON_NODE_TYPE_STRING: {
			free(node->v_string);
			break;
		}

		case JSON_NODE_TYPE_OBJECT: {
			rbtree_destroy(node->v_object);
			break;
		}

		case JSON_NODE_TYPE_ARRAY: {
			vector_destroy(node->v_array);
			break;
		}

		default:
			break;
	}

	free(data);
}

static int strlcatv(char* dst, int* size, ...) {

	if (!dst || !*size)
		return -1;

	register int res = 0;
	char* src;
	va_list va;
	va_start(va, size);
	while (src = va_arg(va, char*)) {

		register int dstlen = strlen(dst);
		register int srclen = strlen(src);

		if (*size <= 0 || dstlen + srclen >= *size) {
			res = -1;
			break;
		}

		memcpy(dst + dstlen, src, srclen);
		dst[dstlen + srclen] = '\0';
		*size-=srclen;
	}
	va_end(va);

	return res;
}

int json_node_print(json_node_t* node, json_style_t style, int* len, char* str) {

	if (!node || !str || len <= 0)
		return -1;

	int res = 0;
	switch(json_node_type(node)) {
		case JSON_NODE_TYPE_BOOL: {
			if (strlcatv(str, len, node->v_bool ? "TRUE" : "FALSE", NULL))
				res = -1;
			break;
		}

		case JSON_NODE_TYPE_INTEGER: {
			char buffer[16];
			snprintf(buffer, sizeof(buffer), "%d", node->v_int);
			if (strlcatv(str, len, buffer, NULL))
				res = -1;
			break;
		}

		case JSON_NODE_TYPE_DOUBLE: {
			char buffer[64];
			snprintf(buffer, sizeof(buffer), "%f", node->v_double);
			if (strlcatv(str, len, buffer, NULL))
				res = -1;
			break;
		}

		case JSON_NODE_TYPE_NULL: {
			if (strlcatv(str, len, "NULL", NULL))
				res = -1;
			break;
		}

		case JSON_NODE_TYPE_STRING: {
			if (strlcatv(str, len, "\"", node->v_string, "\"", NULL))
				res = -1;
			break;
		}

		case JSON_NODE_TYPE_OBJECT: {
			rbtree_iterator_t* it = rbtree_iterator_create(node->v_object);
			const char* key;
			void* data;

			res = strlcatv(str, len, "{", NULL);
			int id = rbtree_size(node->v_object);
			while ((rbtree_iterate(it, &key, &data)) && !res && id--) {
				if (res = strlcatv(str, len, "\"", key, "\":", NULL)) continue;
//				if (res = json_node_print(data, style, len, str))     continue;
//				if (res = strlcatv(str, len, ",", NULL))              continue;
			}

			if (strlcatv(str, len, "}", NULL))
				res = -1;

			rbtree_iterator_destroy(it);
			break;
		}

		case JSON_NODE_TYPE_ARRAY: {
			vector_iterator_t* it = vector_iterator_create(node->v_array);
			void* data;
			if (strlcatv(str, len, "[", NULL)) res = -1;
			int id = rbtree_size(node->v_object);
			while ((data = vector_iterate(it)) && !res) {
				if (json_node_print(data, style, len, str)) res = -1;
				if (-- id)
					if (strlcatv(str, len, ",", NULL)) res = -1;
			}

			if (strlcatv(str, len, "]", NULL)) res = -1;
				vector_iterator_destroy(it);
			break;
		}

		default:
			break;
	}

	return res;
}

json_node_t* json_node_object_node(json_node_t* node, const char* name, json_node_type_t type) {

	if (node && name && json_node_type(node) == JSON_NODE_TYPE_OBJECT) {
		json_node_t* child = get_from_rbtree(node->v_object, name);
		if (!child)
			return NULL;

		else {
			if (type == json_node_type(child) || type == JSON_NODE_TYPE_ANY)
				return child;
			else	return NULL;
		}
	}
	else
		return NULL;
}

json_node_t* json_node_array_node(json_node_t* node, int id) {

	if (!node || id > json_node_array_count(node))
		return NULL;

	ERROR("get from array not ready !!!");
	return NULL;
}

const char* json_node_string_value(json_node_t* node) {

	if (node && node->type == JSON_NODE_TYPE_STRING)
		return node->v_string;
	else	return NULL;
}

int json_node_int_value(json_node_t* node) {

	if (node && node->type == JSON_NODE_TYPE_INTEGER)
		return node->v_int;
	else	return 0;
}

double json_node_double_value(json_node_t* node) {

	if (node && node->type == JSON_NODE_TYPE_DOUBLE)
		return node->v_double;
	else	return 0;
}

int json_node_bool_value(json_node_t* node) {

	if (node && node->type == JSON_NODE_TYPE_BOOL)
		return node->v_bool;
	else	return 0;
}
