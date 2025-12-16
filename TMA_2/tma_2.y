%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "symbol_table.h"
    #include "parser.h"

    extern int yylex();
    extern int lineno;
    extern int yywrap();
    extern struct Token t;

    int yyparse();
    void yyerror(const char* s);
%}

%union {
    int integer_values;
    char* character_values;
    float float_values;
}

%token IF ELSE THEN WHILE RETURN READ WRITE
%token FUNC CLASS CONSTRUCT ATTRIBUTE IMPLEMENT
%token ISA SELF PUBLIC PRIVATE LOCAL VOID

%token SEMICOLON COMMA DOT COLON
%token LEFTPAREN RIGHTPAREN LEFTBRACE RIGHTBRACE LEFTBRACKET RIGHTBRACKET

%token PLUS MINUS MULTIPLY DIVIDE
%token LESS GREATER
%token ASSIGN GOEQ LOEQ NEQ ARROW
%token AND OR NOT

%token <character_values> ID ALPHANUM
%token <integer_values> INTEGER INTEGER_LITERAL
%token <float_values> FLOAT FRACTION FLOAT_LITERAL
%token <integer_values> NONZERO
%token <character_values> LETTER DIGIT

%token PRINT_SYMBOL_TABLE
%token EXIT

%%

program:
    PRINT_SYMBOL_TABLE {
        printf("Yacc:: PRINT_TABLE found.\n");
        print_symbol_table();
    }
    | EXIT {
        printf("Yacc:: EXIT found.\n");
        print_symbol_table();
        exit(0);
    }
    /* instead of using yacc grammar, we tell program to use recursive descent parsing */
    |{
        printf("Yacc:: Switching to recursive descent parser...\n");
        parse_program(); // Calling to recursive descent parser
        YYACCEPT; // immediately terminates the parsing process of yyparse()
    }
    
;

%%

int main() {
    printf("======================================\n");
    printf("EEX6363 - Compiler Construction.\n");
    printf("Name: W.M.A.T.Wanninayake.\n");
    printf("Reg: 321428456 | S.No: S92068456\n");
    printf("======================================\n\n");

    printf("Running Lexical Analyzer with Recursive Descent Parser:\n");
    printf("Program flow: Input Text > Lex > Tokens > Recursive Descent Parser > Derivation\n");

    parse_program();

    printf("Program analysis completed.\n");
    print_symbol_table();
    return 0;
}

void yyerror(const char* s) {
    printf("ERROR OCCURRED:: %s at line %d\n", s, lineno);
}