#ifndef STACK_H
#define STACK_H

#include "ast.h"
#include "symbol_table.h"

// reserved memory addresses for virtual registers
#define VREG_SP    0  
#define VREG_BP    1    
#define VREG_R1    2    
#define VREG_R2    3    
#define VREG_R3    4    
#define VREG_PTR   10 

#define STACK_BASE 1000  
#define GLOBAL_BASE 5000 

// Check local variable
int is_local_variable(const char* varName);

// get offset of local variable
int get_stack_offset(const char* varName);

// get absolute location of variable
int get_variable_location(const char* varName);

// initialize stack manager
void init_stack_manager();

#endif
