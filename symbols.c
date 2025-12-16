#include "symbols.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SYMBOLS 1000

static struct Symbol symbols[MAX_SYMBOLS];
static int symbol_count = 0;

void init_symbols() {
    symbol_count = 0;
}

int add_symbol(const char* lexeme, const char* type, const char* token_type) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbols[i].lexeme, lexeme) == 0) {
            return i;
        }
    }

    symbols[symbol_count].lexeme = strdup(lexeme);
    symbols[symbol_count].type = strdup(type);
    symbols[symbol_count].token_type = strdup(token_type);

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

void print_symbols() {
    printf("\n---------------- SYMBOLS ----------------\n");
    printf("Lexeme\t\tType\t\tToken Type\n");
    for (int i = 0; i < symbol_count; i++) {
        printf("%-15s %-15s %-15s\n",
               symbols[i].lexeme,
               symbols[i].type,
               symbols[i].token_type);
    }
    printf("----------------------------------------------\n");
}
