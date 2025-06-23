
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
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


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     IF = 258,
     ELSE = 259,
     THEN = 260,
     WHILE = 261,
     RETURN = 262,
     READ = 263,
     WRITE = 264,
     FUNC = 265,
     CLASS = 266,
     CONSTRUCT = 267,
     ATTRIBUTE = 268,
     IMPLEMENT = 269,
     ISA = 270,
     SELF = 271,
     PUBLIC = 272,
     PRIVATE = 273,
     LOCAL = 274,
     VOID = 275,
     SEMICOLON = 276,
     COMMA = 277,
     DOT = 278,
     COLON = 279,
     LEFTPAREN = 280,
     RIGHTPAREN = 281,
     LEFTBRACE = 282,
     RIGHTBRACE = 283,
     LEFTBRACKET = 284,
     RIGHTBRACKET = 285,
     PLUS = 286,
     MINUS = 287,
     MULTIPLY = 288,
     DIVIDE = 289,
     LESS = 290,
     GREATER = 291,
     ASSIGN = 292,
     GOEQ = 293,
     LOEQ = 294,
     NEQ = 295,
     ARROW = 296,
     AND = 297,
     OR = 298,
     NOT = 299,
     ID = 300,
     ALPHANUM = 301,
     INTEGER = 302,
     FLOAT = 303,
     FRACTION = 304,
     NONZERO = 305,
     LETTER = 306,
     DIGIT = 307,
     PRINT_SYMBOL_TABLE = 308,
     EXIT = 309
   };
#endif
/* Tokens.  */
#define IF 258
#define ELSE 259
#define THEN 260
#define WHILE 261
#define RETURN 262
#define READ 263
#define WRITE 264
#define FUNC 265
#define CLASS 266
#define CONSTRUCT 267
#define ATTRIBUTE 268
#define IMPLEMENT 269
#define ISA 270
#define SELF 271
#define PUBLIC 272
#define PRIVATE 273
#define LOCAL 274
#define VOID 275
#define SEMICOLON 276
#define COMMA 277
#define DOT 278
#define COLON 279
#define LEFTPAREN 280
#define RIGHTPAREN 281
#define LEFTBRACE 282
#define RIGHTBRACE 283
#define LEFTBRACKET 284
#define RIGHTBRACKET 285
#define PLUS 286
#define MINUS 287
#define MULTIPLY 288
#define DIVIDE 289
#define LESS 290
#define GREATER 291
#define ASSIGN 292
#define GOEQ 293
#define LOEQ 294
#define NEQ 295
#define ARROW 296
#define AND 297
#define OR 298
#define NOT 299
#define ID 300
#define ALPHANUM 301
#define INTEGER 302
#define FLOAT 303
#define FRACTION 304
#define NONZERO 305
#define LETTER 306
#define DIGIT 307
#define PRINT_SYMBOL_TABLE 308
#define EXIT 309




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 33 ".\\tma_1_yc.y"

    int integer_values;
    char* character_values;
    float float_values;



/* Line 1676 of yacc.c  */
#line 168 "y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


