/*
structure of a lex program

%{
[DECLARATION]    
%}

%%
[TRANSITION RULES]
%%

[AUXILIARY FUNCTIONS]
*/

%{
    #include <stdlib.h> // to invoke atoi(yytext)
    #include <stdio.h>  
    #include "y.tab.h" // to include the token definitions
    #include <string.h> // to use strdup
    #include "symbols.h"

    struct Token {
        char* token_type;
        char* lexeme;
        int location;
    };

    int lineno = 1;

    struct Token t;

    void result_type(char* type);
    struct Token create_token(char* token, char* lexeme);

%}

letter [a-zA-Z]
digit [0-9]
alphanum [a-zA-Z0-9_]
id {letter}{alphanum}*

nonzero [1-9]
integer {nonzero}{digit}*|0

fraction {nonzero}*|0\.{digit}*{nonzero}|\.0
float {integer}{fraction}([e][+-]?{integer})?

whitespace [ \t]+

punctuation [;(){}[],.:]
operator [-+*/=<>]

single_line_comment \/\/(.|\n)**
multi_line_comment "/*"(.|\n)*"*/"

%%

"EXIT" { return EXIT; }
"PRINT_SYMBOLS" { return PRINT_SYMBOLS; }

{whitespace} { /* skip */ }
\n { lineno++; }
{single_line_comment}   { /* skip */ }
{multi_line_comment}    { /* skip */ }

";"     { result_type("SEMICOLON"); t = create_token("SEMICOLON", yytext); return SEMICOLON; }
","     { result_type("COMMA"); t = create_token("COMMA", yytext); return COMMA; }
"("     { result_type("LEFTPAREN"); t = create_token("LEFTPAREN", yytext); return LEFTPAREN; }
")"     { result_type("RIGHTPAREN"); t = create_token("RIGHTPAREN", yytext); return RIGHTPAREN; }
"{"     { result_type("LEFTBRACE"); t = create_token("LEFTBRACE", yytext); return LEFTBRACE; }
"}"     { result_type("RIGHTBRACE"); t = create_token("RIGHTBRACE", yytext); return RIGHTBRACE; }
"["     { result_type("LEFTBRACKET"); t = create_token("LEFTBRACKET", yytext); return LEFTBRACKET; }
"]"     { result_type("RIGHTBRACKET"); t = create_token("RIGHTBRACKET", yytext); return RIGHTBRACKET; }
"."     { result_type("DOT"); t = create_token("DOT", yytext); return DOT; }
":"     { result_type("COLON"); t = create_token("COLON", yytext); return COLON; }


"+"     {result_type("PLUS"); t = create_token("PLUS", yytext); return PLUS; }
"-"     {result_type("MINUS"); t = create_token("MINUS", yytext); return MINUS; }
"*"     {result_type("MULTIPLY"); t = create_token("MULTIPLY", yytext); return MULTIPLY; }
"/"     {result_type("DIVIDE"); t = create_token("DIVIDE", yytext); return DIVIDE; }
"<"     {result_type("LESS"); t = create_token("LESS", yytext); return LESS; }
">"     {result_type("GREATER"); t = create_token("GREATER", yytext); return ASSIGN; }
":="     {result_type("ASSIGN"); t = create_token("ASSIGN", yytext); return ASSIGN; }

">="    {result_type("GREATEROREQUAL"); t = create_token("GREATEROREQUAL", yytext); return GOEQ; }
"<="    {result_type("LESSOREQUAL"); t = create_token("LESSOREQUAL", yytext); return LOEQ; }
"<>"    {result_type("NOTEQUAL"); t = create_token("NOTEQUAL", yytext); return NEQ; }
"=>"    {result_type("ARROW"); t = create_token("ARROW", yytext); return ARROW; }

"or"    {result_type("OR"); t = create_token("OR", yytext); return OR; }
"and"   {result_type("AND"); t = create_token("AND", yytext); return AND; }
"not"   {result_type("NOT"); t = create_token("NOT", yytext); return NOT; }

"else"  {result_type("else"); t = create_token("ELSE", yytext); return ELSE; }
"float" {result_type("float"); t = create_token("FLOAT", yytext); return FLOAT; }
"func"  {result_type("func"); t = create_token("FUNC", yytext); return FUNC; }
"if"    {result_type("if"); t = create_token("IF", yytext); return IF; }
"implement" {result_type("implement"); t = create_token("IMPLEMENT", yytext); return IMPLEMENT; }
"class"     {result_type("class"); t = create_token("CLASS", yytext); return CLASS; }
"attribute" {result_type("attribute"); t = create_token("ATTRIBURE", yytext); return ATTRIBUTE; }
"integer"   {result_type("integer"); t = create_token("INTEGER", yytext); return INTEGER; }
"isa"       {result_type("isa"); t = create_token("ISA", yytext); return ISA; }
"private"   {result_type("private"); t = create_token("PRIVATE", yytext); return PRIVATE; }
"public"    {result_type("public"); t = create_token("PUBLIC", yytext); return PUBLIC; }
"read"      {result_type("read"); t = create_token("READ", yytext); return READ; }
"return"    {result_type("return"); t = create_token("RETURN", yytext); return RETURN; }
"self"      {result_type("self"); t = create_token("SELF", yytext); return SELF; }
"construct" {result_type("construct"); t = create_token("CONSTRUCT", yytext); return CONSTRUCT; }
"then"      {result_type("then"); t = create_token("THEN", yytext); return THEN; }
"local"     {result_type("local"); t = create_token("LOCAL", yytext); return LOCAL; }
"void"      {result_type("void"); t = create_token("VOID", yytext); return VOID; }
"while"     {result_type("while"); t = create_token("WHILE", yytext); return WHILE; }
"write"     {result_type("write"); t = create_token("WRITE", yytext); return WRITE; }

{integer}   {result_type("integer_literal"); t = create_token("INTEGER_LITERAL", yytext); 
    yylval.integer_values = atoi(strdup(yytext)); 
    return INTEGER_LITERAL; 
}
{id}        {result_type("id"); t = create_token("ID", yytext); 
    add_symbol(yytext, "id", "ID");
    yylval.character_values = strdup(strdup(yytext)); 
    return ID;
}


{digit}     {result_type("digit"); t = create_token("DIGIT", yytext); 
    yylval.integer_values = atoi(strdup(yytext)); 
    return DIGIT; 
}
{alphanum}  {result_type("alphanum"); t = create_token("ALPHANUM", yytext);
    add_symbol(yytext, "alphanum", "ALPHANUM");
    yylval.character_values = strdup(yytext);
    return ALPHANUM;
}

{float}     {result_type("float_literal"); t = create_token("FLOAT_LITERAL", yytext); 
    yylval.float_values = atof(strdup(yytext)); 
    return FLOAT_LITERAL; 
}


. { printf("Your input does not match for any lexemes defined in this language!\nUnknown character: %s (ASCII: %d)\n", yytext, yytext[0]); }

%%

//int main(){
//    printf("Enter a character, number, word, operator etc.: \n");
//    yylex();
//}

void result_type(char* type){
    printf("Found token type: %s\n", type);
}

struct Token create_token(char* token, char* lexeme){
    struct Token token_to_send;
    token_to_send.token_type = strdup(token);
    token_to_send.lexeme = strdup(lexeme);
    token_to_send.location = lineno;

    printf(
        "\nToken type: %s\nLexeme: %s\nLocation(Line): %d\n\n",token_to_send.token_type, token_to_send.lexeme, token_to_send.location);

    return token_to_send; 
}

int yywrap(){
    return 1;
}