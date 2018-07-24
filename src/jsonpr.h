/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_JSONPR_H_INCLUDED
# define YY_YY_JSONPR_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 10 "jsonpr.y" /* yacc.c:1909  */


#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "rbtree.h"
#include "vector.h"

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
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

#line 97 "jsonpr.h" /* yacc.c:1909  */

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TOKEN_NULL = 258,
    TOKEN_BOOL = 259,
    TOKEN_INTEGER = 260,
    TOKEN_DOUBLE = 261,
    TOKEN_STRING = 262
  };
#endif
/* Tokens.  */
#define TOKEN_NULL 258
#define TOKEN_BOOL 259
#define TOKEN_INTEGER 260
#define TOKEN_DOUBLE 261
#define TOKEN_STRING 262

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 63 "jsonpr.y" /* yacc.c:1909  */

	json_node_t* node;
	char* s;
	double d;
	int i;
	int b;
	json_value_t* v;

#line 132 "jsonpr.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int yyparse (yyscan_t scanner, json_node_t* *node);

#endif /* !YY_YY_JSONPR_H_INCLUDED  */
