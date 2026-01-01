#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

struct Symbol {
    char* lexeme;
    char* type;
    char* token_type;
};

void init_symbols(); 

int add_symbol(const char* lexeme, const char* type, const char* token_type);
struct Symbol* get_symbol(const char* lexeme);

void print_symbols();

#endif 

