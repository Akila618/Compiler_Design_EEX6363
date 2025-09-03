#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SYMBOLS 1000

static struct Symbol symbols[MAX_SYMBOLS];
static int symbol_count = 0;

void init_symbol_table() {
    symbol_count = 0;
}

int add_symbol(const char* lexeme, const char* type, const char* token_type,int size, const char* scope, int line,const char* address, const char* other) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbols[i].lexeme, lexeme) == 0) {
            return i;
        }
    }

    symbols[symbol_count].lexeme = strdup(lexeme);
    symbols[symbol_count].type = strdup(type);
    symbols[symbol_count].token_type = strdup(token_type);
    symbols[symbol_count].size = size;
    symbols[symbol_count].scope = strdup(scope);
    symbols[symbol_count].line = line;
    symbols[symbol_count].address = strdup(address);
    symbols[symbol_count].other = strdup(other);

    symbol_count++;
    return symbol_count - 1;
}

struct Symbol* get_symbol(const char* lexeme) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbols[i].lexeme, lexeme) == 0) {
            return &symbols[i];
        }
    }
    return NULL; // Not found
}

void print_symbol_table() {
    printf("\n---------------- SYMBOL TABLE ----------------\n");
    printf("Lexeme\t\tType\t\tToken Type\tSize\tScope\tLine\tAddress\tOther\n");
    for (int i = 0; i < symbol_count; i++) {
        printf("%-15s %-10s %-12s %-5d %-10s %-5d %-15s %-10s\n",
               symbols[i].lexeme,
               symbols[i].type,
               symbols[i].token_type,
               symbols[i].size,
               symbols[i].scope,
               symbols[i].line,
               symbols[i].address,
               symbols[i].other);
    }
    printf("----------------------------------------------\n");
}
