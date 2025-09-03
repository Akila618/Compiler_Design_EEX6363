// parser.h
#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"


extern int yylex();
extern char* yytext;
extern int lineno;

// Global variables for parser
extern FILE* log_file;
extern int lookahead_token;
extern int derivation_step;

// Main parser function
void parse_program();
void init_parser();

// Grammar rule functions - these implement your BNF productions
void prog();
void classOrImplOrFuncList();
void classOrImplOrFunc();
void classDecl();
void isaIdOpt();
void idTail();
void visibilitymemberDeclList();
void implDef();
void funcDefList();
void funcDef();
void visibility();
void memberDecl();
void funcDecl();
void funcHead();
void funcBody();
void varDeclOrStmtList();
void varDeclOrStmt();
void attributeDecl();
void localVarDecl();
void varDecl();
void arraySizeList();
void statement();
void assignStat();
void statBlock();
void statmentList();
void expr();
void relExpr();
void arithExpr();
void arithExprTail();
void term();
void termTail();
void factor();
void variable();
void idnestList();
void indiceList();
void functionCall();
void idnest();
void idnestTail();
void idOrSelf();
void indice();
void arraySize();
void type();
void returnType();
void fParams();
void fParamsTailList();
void aParams();
void aParamsTailList();
void fParamsTail();
void aParamsTail();

void assignOp();
void addOp();
void relOp();
void multOp();
void sign();


// Utility functions
void match(int expected_token);
void error(char* message);
void write_derivation(char* rule);

#endif