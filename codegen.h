#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"

// quadruple structure
typedef struct {
    char* op;
    char* arg1;
    char* arg2;
    char* res;
} Quadruple;

ASTNode* generate_ir(ASTNode* root);
void write_program_ir(ASTNode* root);

// expose quadruple list
Quadruple* get_quad_list();
int get_quad_count();

// variable address lookup
int get_var_address(const char* name);

#endif
