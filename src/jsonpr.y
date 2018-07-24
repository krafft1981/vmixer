
%define api.pure full

%define parse.error verbose

%lex-param	{ yyscan_t scanner }
%parse-param	{ yyscan_t scanner }
%parse-param	{ json_node_t* *node }

%code requires {

#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "rbtree.h"
#include "vector.h"

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

#define YY_NEED_STRLEN 1

struct parser_s {

	yyscan_t scanner;
};

struct json_node_s {

	json_node_type_t type;
	union {
		int       v_int;
		double    v_double;
		char*     v_string;
		int       v_bool;
		rbtree_t* v_object;
		vector_t* v_array;
	};
};

typedef struct json_value_s json_value_t;

struct json_value_s {

	char* name;
	json_node_t* node;
	json_value_t* next;
};

int yylex();
extern void yyerror(yyscan_t scanner, json_node_t* *node, char const* msg);
static json_value_t* json_value_create(json_node_t* node);
static void json_value_destroy(json_value_t* value, int mode);
}

%union {
	json_node_t* node;
	char* s;
	double d;
	int i;
	int b;
	json_value_t* v;
}

%token		TOKEN_NULL
%token	<b>	TOKEN_BOOL
%token	<i>	TOKEN_INTEGER
%token	<d>	TOKEN_DOUBLE
%token	<s>	TOKEN_STRING

%type	<node>	START
%type	<node>	OBJECT
%type	<node>	ARRAY
%type	<v>	VALUE
%type	<v>	MEMBER
%type	<v>	ELEMENT

%destructor { json_value_destroy($$, 1); } <v>
%destructor { json_node_destroy ($$); } <node>

%start START

%%

START	:	VALUE					{ if (node)
								*node = $1->node;
							else	json_node_destroy($1->node);
							json_value_destroy($1, 0);
							$$ = NULL;
						}
;

OBJECT	:	'{' '}'					{ $$ = json_node_object(NULL); }
	|	'{' MEMBER '}'				{ rbtree_t* rbtree = rbtree_create(free, json_node_destroy);
								$$ = json_node_object(rbtree);
								json_value_t* value = $2;

								while (value) {
									set_to_rbtree(rbtree, value->name, value->node);
									value = value->next;
								}

								json_value_destroy($2, 0);
						}
;

ARRAY	:	'[' ']'					{ $$ = json_node_array(NULL); }
	|	'[' ELEMENT ']'				{ vector_t* vector = vector_create(0, json_node_destroy);
								$$ = json_node_array(vector);
								json_value_t* value = $2;

								while (value) {
									set_to_vector(vector, value->node);
									value = value->next;
								}

								json_value_destroy($2, 0);
						}
;

MEMBER	:	TOKEN_STRING ':' VALUE			{ $$ = $3;                $3->name = strdup($1); }
	|	MEMBER ',' TOKEN_STRING ':' VALUE	{ $$ = $5; $5->next = $1; $5->name = strdup($3); }
;

ELEMENT	:	VALUE					{ $$ = $1;                }
	|	ELEMENT ',' VALUE			{ $$ = $3; $3->next = $1; }
;

VALUE	:	OBJECT					{ $$ = json_value_create($1); }
	|	ARRAY					{ $$ = json_value_create($1); }
	|	TOKEN_STRING				{ $$ = json_value_create(json_node_string($1)); }
	|	TOKEN_BOOL				{ $$ = json_value_create(json_node_bool  ($1)); }
	|	TOKEN_INTEGER				{ $$ = json_value_create(json_node_int   ($1)); }
	|	TOKEN_DOUBLE				{ $$ = json_value_create(json_node_double($1)); }
	|	TOKEN_NULL				{ $$ = json_value_create(json_node_null  (  )); }
;

%%

static json_value_t* json_value_create(json_node_t* node) {

	json_value_t* value;
	if (value = calloc(1, sizeof(json_value_t)))
		value->node = node;

	return value;
}

static void json_value_destroy(json_value_t* value, int mode) {

	if (!value)
		return;

	if (mode) { // destroy data or not
		if (value->name)
			free(value->name);

		if (value->node)
			json_node_destroy(value->node);
	}

	if (value->next)
		json_value_destroy(value->next, mode);

	free(value);
}
