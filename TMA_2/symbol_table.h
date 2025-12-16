#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

struct Symbol {
    char* lexeme;
    char* type;
    char* token_type;
    int size;
    char* scope;
    int line;
    char* address;
    char* other;
};

void init_symbol_table(); 

int add_symbol(const char* lexeme, const char* type, const char* token_type, int size, const char* scope, int line, const char* address, const char* other);

struct Symbol* get_symbol(const char* lexeme);

void print_symbol_table();

#endif 

