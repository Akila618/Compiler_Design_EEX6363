%{
    
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    extern int yylex();
    extern int lineno;
    extern int yywrap();
    extern struct Token t;

    struct Symbol_table {
        char* lexeme;
        char* type;
        char* token_type;
        int size;
        char* scope;
        int line;
        char* address;
        char* other;
    };


    struct Symbol_table st[1000];
    void add_symbol(char* lexeme, char* type, char* token_type, int size, char* scope, int line, char* address, char* other);
    void print_symbol_table();
    int no_of_symbols = 0;
    
    void yyerror(const char* s);
    int yyparse();
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
%token <integer_values> INTEGER
%token <float_values> FLOAT FRACTION
%token <integer_values>NONZERO
%token <character_values> LETTER DIGIT

%token PRINT_SYMBOL_TABLE
%token EXIT

%left ':' '.' ';' ',' '(' ')' '{' '}' '[' ']'
%left '+' '-' '<' '>'
%left '*' '/' '%'

%%

program:
    statements
;

statements:
    | statements statement
;

statement:
    lexical_element
    | punctuation
    | operator
    | reserved_word
    | exit
    | print_symbol_table
;

print_symbol_table:
    PRINT_SYMBOL_TABLE{
        printf("Yacc:: PRINT_TABLE found.\n");
        print_symbol_table();
    }   
;

lexical_element:
    LETTER {
        add_symbol($1, "lexical_element", "ID", 0, "global", lineno, "-", "-");
    }
    |ALPHANUM {
        add_symbol($1, "lexical_element", "ALPHANUM", 0, "global", lineno, "-", "-");
    }
    |ID {
        add_symbol($1, "lexical_element", "ID", 0, "global", lineno, "-", "-");
    }
    |DIGIT{
        printf("Yacc:: DIGIT found.\n");
    }
    |INTEGER{
        printf("Yacc:: INTEGER found.\n");
    }
    |FLOAT{
        printf("Yacc:: FLOAT found.\n");
    }
    |NONZERO{
        printf("Yacc:: NONZERO found.\n");
    }
    |FRACTION{
        printf("Yacc:: FRACTION found.\n");
    }
;

punctuation:
    SEMICOLON {
        printf("Yacc:: Semicolon found.\n");
    }
    |COMMA {
        printf("Yacc:: Comma found.\n");
    }
    |DOT {
        printf("Yacc:: Dot found.\n");
    }
    |COLON {
        printf("Yacc:: Colon found.\n");
    }
    |LEFTPAREN {
        printf("Yacc:: Left Parenthesis found.\n");
    }
    |RIGHTPAREN {
        printf("Yacc:: Right Parenthesis found.\n");
    }
    |LEFTBRACE {
        printf("Yacc:: Left Brace found.\n");
    }
    |RIGHTBRACE {
        printf("Yacc:: Right Brace found.\n");
    }
    |LEFTBRACKET {
        printf("Yacc:: Left Bracket found.\n");
    }
    |RIGHTBRACKET {
        printf("Yacc:: Right Bracket found.\n");
    }
;

operator:
    PLUS {
        printf("Yacc:: Plus operator found.\n");
    }
    |MINUS {
        printf("Yacc:: Minus operator found.\n");
    }
    |MULTIPLY {
        printf("Yacc:: Multiply operator found.\n");
    }
    |DIVIDE {
        printf("Yacc:: Divide operator found.\n");
    }
    |LESS {
        printf("Yacc:: Less than operator found.\n");
    }
    |GREATER {
        printf("Yacc:: Greater than operator found.\n");
    }
    |ASSIGN {
        printf("Yacc:: Assignment operator found.\n");
    }
    |GOEQ {
        printf("Yacc:: Greater than or equal to operator found.\n");
    }
    |LOEQ {
        printf("Yacc:: Less than or equal to operator found.\n");
    }
    |NEQ {
        printf("Yacc:: Not equal to operator found.\n");
    }
    |ARROW {
        printf("Yacc:: Arrow operator found.\n");
    }
    |AND {
        printf("Yacc:: AND operator found.\n");
    }
    |OR {
        printf("Yacc:: OR operator found.\n");
    }
    |NOT {
        printf("Yacc:: NOT operator found.\n");
    }
;

reserved_word:
    ELSE {
        printf("Yacc:: ELSE reserved word found.\n");
    }
    |IF {
        printf("Yacc:: IF reserved word found.\n");
    }
    |FUNC {
        printf("Yacc:: FUNC reserved word found.\n");
    }
    |IMPLEMENT {
        printf("Yacc:: IMPLEMENT reserved word found.\n");
    }
    |CLASS {
        printf("Yacc:: CLASS reserved word found.\n");
    }
    |ATTRIBUTE {
        printf("Yacc:: ATTRIBUTE reserved word found.\n");
    }
    |ISA {
        printf("Yacc:: ISA reserved word found.\n");
    }
    |PRIVATE {
        printf("Yacc:: PRIVATE reserved word found.\n");
    }
    |PUBLIC {
        printf("Yacc:: PUBLIC reserved word found.\n");
    }
    |READ {
        printf("Yacc:: READ reserved word found.\n");
    }
    |RETURN {
        printf("Yacc:: RETURN reserved word found.\n");
    }
    |SELF {
        printf("Yacc:: SELF reserved word found.\n");
    }
    |THEN {
        printf("Yacc:: THEN reserved word found.\n");
    }
    |LOCAL {
        printf("Yacc:: LOCAL reserved word found.\n");
    }
    |VOID {
        printf("Yacc:: VOID reserved word found.\n");
    }
    |WHILE {
        printf("Yacc:: WHILE reserved word found.\n");
    }
;

exit: 
    EXIT {
        printf("Yacc:: EXIT reserved word found.\n");
        print_symbol_table();
        exit(0);        
    }
;


%%


int main() {
    printf("======================================");
    printf("EEX6363 - Compiler Construction.\n");
    printf("Name: W.M.A.T.Wanninayake.\n");
    printf("Reg: 321428456 | S.No: S92068456 \n");
    printf("======================================\n\n");

    printf("Running Lex and Yacc:\n");

    yyparse();

    printf("Parsing completed.\n");

    return 0;
}

void add_symbol(char* lexeme, char* type, char* token_type, int size, char* scope, int line, char* address, char* other) {
    st[no_of_symbols].lexeme = strdup(lexeme);
    st[no_of_symbols].type = strdup(type);
    st[no_of_symbols].token_type = strdup(token_type);
    st[no_of_symbols].line = line;

    st[no_of_symbols].size = 0;
    st[no_of_symbols].scope = "global";
    st[no_of_symbols].address = "-";
    st[no_of_symbols].other = "-";

    no_of_symbols++;

    printf("Symbol added: %s, Type: %s, Token Type: %s, Size: %d, Scope: %s, Line: %d, Address: %s, Other: %s\n",
            lexeme, type, token_type, size, scope, line, address, other);
    printf("Total symbols: %d\n", no_of_symbols);
}

void print_symbol_table() {
    printf("\n------------------------------- SYMBOL TABLE ---------------------------------------\n");
    printf("Lexeme\tType\tToken Type\tSize\tScope\tLine\tAddress\tOther\n");

    for (int i = 0; i < no_of_symbols; i++) {
        printf("%s\t%s\t%s\t%d\t%s\t%d\t%s\t%s\n",
            st[i].lexeme,
            st[i].type,
            st[i].token_type,
            st[i].size,
            st[i].scope,
            st[i].line,
            st[i].address,
            st[i].other
        );
    }
}


void yyerror(const char* s) {
    printf("ERROR OCCURED:: %s\n", s);
}
