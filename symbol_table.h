#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdio.h>
#include "ast.h"

// types of symbols: as integer constants ============
typedef enum {
    SYM_CLASS,
    SYM_FUNCTION,
    SYM_PARAM,
    SYM_VARIABLE,
    SYM_ATTRIBUTE
} SymbolKind;

// types of types: as integer constants ============
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_VOID,
    TYPE_CLASS,
    TYPE_ARRAY,
    TYPE_UNKNOWN
} TypeKind;

// structure of a type ============
typedef struct Type {
    TypeKind kind;
    char name[64]; 
    struct Type* elementType; 
    int dimensions; 
    size_t* dimSizes; 
    char parent_name[64];
} Type;

// Symbol entry structure (singly linked list) ============
typedef struct SymbolEntry {
    char name[64];
    SymbolKind kind;
    Type type;
    int scopeLevel;
    struct ASTNode* declNode;
    int line;
    size_t width;    // size in bytes
    int offset;      
    int frameSize;   // total size of frame
    struct SymbolEntry* next;
} SymbolEntry;

// Scope structure ============
typedef struct Scope {
    int level;
    char name[64];
    SymbolEntry* symbols;
    struct Scope* parent;
    struct Scope* nextSibling;
} Scope;

// APIs for symbol table management ============
void st_init();
void st_enter_scope(const char* name);
void st_exit_scope();
SymbolEntry* st_add_symbol(const char* name, SymbolKind kind, Type type, struct ASTNode* declNode, int line);
SymbolEntry* st_lookup_local(const char* name);
SymbolEntry* st_lookup(const char* name);
SymbolEntry* st_lookup_global(const char* name);  
void st_print(FILE* out);
void st_write_file(const char* path);

// compute size for a type
size_t compute_type_size(const Type* t);

// compute frame layout (assign offsets)
int st_compute_frame_layout(const char* functionName);

// find frame layouts for all function scopes
void st_compute_all_frame_layouts();

// helpers ============
Type make_basic_type(TypeKind k);
Type make_class_type(const char* className);
int type_equal(const Type* a, const Type* b);

#endif 
