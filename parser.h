// parser.h
#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbols.h"
#include "ast.h"


extern int yylex();
extern char* yytext;
extern int lineno;

// Global variables for parser
extern FILE* log_file;
extern int lookahead_token;
extern int derivation_step;

// Main parser function
ASTNode* parse_program();
void init_parser();

// Grammar rule functions
// forward declarations: all functions now return ASTNode*
ASTNode* prog();
ASTNode* classOrImplOrFuncList();
ASTNode* classOrImplOrFunc();
ASTNode* classDecl();
ASTNode* isaIdOpt();
ASTNode* idTail();
ASTNode* visibilitymemberDeclList();
ASTNode* implDef();
ASTNode* funcDefList();
ASTNode* funcDef();
ASTNode* visibility();
ASTNode* memberDecl();
ASTNode* funcHead();
ASTNode* funcBody();
ASTNode* varDeclOrStmtList();
ASTNode* varDeclOrStmt();
ASTNode* attributeDecl();
ASTNode* localVarDecl();
ASTNode* varDecl();
ASTNode* arraySizeList();
ASTNode* statement();
ASTNode* assignStat();
ASTNode* assignOp();
ASTNode* statBlock();
ASTNode* statmentList();
ASTNode* expr();
ASTNode* relExpr();
ASTNode* relOp();
ASTNode* arithExpr();
ASTNode* arithExprTail();
ASTNode* addOp();
ASTNode* term();
ASTNode* termTail();
ASTNode* multOp();
ASTNode* factor();
ASTNode* sign();
ASTNode* variable();
ASTNode* idnestList();
ASTNode* indiceList();
ASTNode* functionCall();
ASTNode* idnest();
ASTNode* idnestTail();
ASTNode* idOrSelf();
ASTNode* indice();
ASTNode* arraySize();
ASTNode* type();
ASTNode* returnType();
ASTNode* fParams();
ASTNode* fParamsTailList();
ASTNode* aParams();
ASTNode* aParamsTailList();
ASTNode* fParamsTail();
ASTNode* aParamsTail();


// Utility functions
void match(int expected_token);
void error(char* message);
void write_derivation(char* rule);

#endif