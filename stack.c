/*
 * stack.c - virtual stack mplementation for accumulator based ISA architecture as it has only accumulator
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "stack.h"
#include "symbol_table.h"

// global variable tracking
typedef struct VarMap {
    char* name;
    int address;
    struct VarMap* next;
} VarMap;

static VarMap* global_vars = NULL;
static int next_global_addr = GLOBAL_BASE;

void init_stack_manager() {
    while (global_vars) {
        VarMap* tmp = global_vars;
        global_vars = global_vars->next;
        free(tmp->name);
        free(tmp);
    }
    global_vars = NULL;
    next_global_addr = GLOBAL_BASE;
}

// find local variables in symbol table
int is_local_variable(const char* varName) {
    if (!varName) return 0;
    
    // check for virtual registers
    if (strcmp(varName, "SP") == 0) return 0;
    if (strcmp(varName, "BP") == 0) return 0;
    if (strcmp(varName, "PTR") == 0) return 0;
    
    if (isdigit(varName[0]) || (varName[0] == '-' && isdigit(varName[1]))) {
        return 0;
    }
    
    if (varName[0] == 't' && isdigit(varName[1])) {
        return 0; // temporary variable
    }
    
    // look up in symbol table (use global search during code generation)
    SymbolEntry* sym = st_lookup_global(varName);
    if (sym) {
        if (sym->scopeLevel > 0 && sym->kind != SYM_CLASS) {
            return 1;
        }
    } else {
        printf(" (Symbol not found!)\n");
    }
    
    return 0;
}

// stack offset for local variable from symbol table
int get_stack_offset(const char* varName) {
    if (!varName) return 0;
    
    SymbolEntry* sym = st_lookup_global(varName);
    if (sym && sym->scopeLevel > 0 && sym->kind != SYM_CLASS) {
        return sym->offset;
    }
    
    return 0;
}

// get variable location
int get_variable_location(const char* varName) {
    if (!varName) return 0;
    
    //registers have fixed addresses
    if (strcmp(varName, "SP") == 0) return VREG_SP;
    if (strcmp(varName, "BP") == 0) return VREG_BP;
    if (strcmp(varName, "PTR") == 0) return VREG_PTR;
    
    if (isdigit(varName[0]) || (varName[0] == '-' && isdigit(varName[1]))) {
        return -1;
    }
    
    if (is_local_variable(varName)) {
        return get_stack_offset(varName);
    }
    
    // global variables static addresses
    for (VarMap* v = global_vars; v; v = v->next) {
        if (strcmp(v->name, varName) == 0) {
            return v->address;
        }
    }
    
    // allocate new global address
    VarMap* newVar = malloc(sizeof(VarMap));
    newVar->name = strdup(varName);
    newVar->address = next_global_addr++;
    newVar->next = global_vars;
    global_vars = newVar;
    
    return newVar->address;
}
