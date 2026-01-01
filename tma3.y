%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "symbols.h"
    #include "parser.h"

    extern int yylex();
    extern int lineno;
    extern int yywrap();
    extern struct Token t;
    extern int yylineno;

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
%token <integer_values> INTEGER_LITERAL
%token INTEGER
%token <float_values> FRACTION FLOAT_LITERAL
%token FLOAT
%token <integer_values> NONZERO
%token <character_values> LETTER DIGIT

%token PRINT_SYMBOLS
%token EXIT

%%

program:
    PRINT_SYMBOLS {
        printf("Yacc:: PRINT_SYMBOLS found.\n");
        print_symbols();
    }
    | EXIT {
        printf("Yacc:: EXIT found.\n");
        print_symbols();
        exit(0);
    }
    /* instead of using yacc grammar recursive descent parsing is used*/
    |{
        printf("Yacc:: Switching to recursive descent parser...\n");
        ASTNode* root = parse_program(); // Calling to recursive descent parser
        (void)root;
        YYACCEPT; // stops the parsing process of yyparse()
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

    ASTNode* root = parse_program();
    (void)root;
    //print root
    printf("root: %d", root);

    printf("Program analysis completed.\n");
    print_symbols();
    return 0;
}

void yyerror(const char* s) {
    printf("ERROR OCCURRED:: %s at line %d\n", s, lineno);
}