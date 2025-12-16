#include "parser.h"
#include "y.tab.h"
#include "semantic.h"

#if defined(_WIN32) || defined(_WIN64) 
#include <direct.h>
#define MKDIR(dir) _mkdir(dir)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR(dir) mkdir(dir, 0755)
#endif

FILE* log_file;
int lookahead_token;
int derivation_step = 1;
extern int lineno;

// ====== Initialize parser =====
void init_parser() {
    // ensure output directory exists
    MKDIR("files");
    log_file = fopen("files/derivation.txt", "w");
    if (!log_file) {
        printf("Error:: Cannot create derivation file\n");
        exit(1);
    }
    fprintf(log_file,"======================================\n");
    fprintf(log_file,"EEX6363 - Compiler Construction.\n");
    fprintf(log_file,"Name: W.M.A.T.Wanninayake.\n");
    fprintf(log_file,"Reg: 321428456 | S.No: S92068456\n");
    fprintf(log_file,"======================================\n\n");
    fprintf(log_file, "Parsing steps of Program:\n");
    fprintf(log_file, "=====================================\n\n");

    lookahead_token = yylex();
}

// ====== Write the rules to the file =====
void write_derivation(char* rule) {
    fprintf(log_file, "Step %d: %s\n", derivation_step++, rule);
    printf("Derivation: %s\n", rule);
}

// ====== write the rules to the file ==============================
void write_syntax_tree(ASTNode* root) {
    FILE* output_file = fopen("files/syntax_tree.txt", "w");
    if (!output_file) {
        printf("Error:: Cannot create syntax tree file\n");
        exit(1);
    }
    fprintf(log_file,"======================================\n");
    printDetailedAST(root, output_file);
    fclose(output_file);
    printf("Syntax tree written to syntax_tree.txt\n");
}

// ====== Match the expected and current token and notify issues =====
void match(int required_token) {
    if (lookahead_token == EXIT) {
        printf("Parser:: EXIT.\n");
        print_symbols();
        exit(0);
    }
    if (lookahead_token == PRINT_SYMBOLS) {
        printf("Parser:: PRINT_SYMBOLS.\n");
        print_symbols();
        lookahead_token = yylex();
    }   
    if (lookahead_token == required_token) {
        printf("Matched token: %d\n", required_token);
        lookahead_token = yylex();
        printf("Next token: %d\n", lookahead_token);
    } else {
        char error_msg[256];
        sprintf(error_msg, "Expected token %d, but found %d", 
                required_token, lookahead_token);
        error(error_msg);

    }
}

// ====== Handle the errors =====
void error(char* message) {
    printf("Parser Error Occurred:: %s\n", message);
    fprintf(log_file, "ERROR: %s\n", message);
    fclose(log_file);
    exit(1);
}


// ====AST Node creation for the starting point of the parsing process====

ASTNode* prog() {
    write_derivation("prog -> classOrImplOrFuncList");
    ASTNode* node = createNode("prog", "");
    addChild(node, classOrImplOrFuncList());
    return node;
}
ASTNode* classOrImplOrFuncList() {
    ASTNode* node = createNode("classOrImplOrFuncList", "");
    if (lookahead_token == CLASS || lookahead_token == IMPLEMENT || 
        lookahead_token == FUNC || lookahead_token == CONSTRUCT) {
        write_derivation("classOrImplOrFuncList -> classOrImplOrFunc classOrImplOrFuncList");
        addChild(node, classOrImplOrFunc());
        addChild(node, classOrImplOrFuncList());
        return node;
    } else {
        write_derivation("classOrImplOrFuncList -> ε");
        return node; // returning node even for ε to maintain tree structure
    }
}   
ASTNode* classOrImplOrFunc() {
    ASTNode* node = createNode("classOrImplOrFunc", "");
    if (lookahead_token == CLASS) {
        write_derivation("classOrImplOrFunc -> classDecl");
        ASTNode* child = classDecl();
        addChild(node, child);
        return node;
    } else if (lookahead_token == IMPLEMENT) {
        write_derivation("classOrImplOrFunc -> implDef");
        ASTNode* child = implDef();
        addChild(node, child);
        return node;
    } else if (lookahead_token == FUNC || lookahead_token == CONSTRUCT) {
        write_derivation("classOrImplOrFunc -> funcDef");
        ASTNode* child = funcDef();
        addChild(node, child);
        return node;
    } else {
        printf("Expected 'class', 'implement', 'func', or 'construct'");
        return NULL;
    }
}

ASTNode* classDecl() {
    write_derivation("classDecl -> class id isaIdOpt { visibilitymemberDeclList } ;");
    ASTNode* node = createNode("classDecl", "");
    if (lookahead_token == CLASS) {
        match(CLASS);
        if (lookahead_token == ID) {
            ASTNode* idnode = createNode("ID", yytext);
            idnode->line = lineno;
            match(ID);
            addChild(node, idnode);
        }
        // optional isaIdOpt
        addChild(node, isaIdOpt());
        match(LEFTBRACE);
        addChild(node, visibilitymemberDeclList());
        match(RIGHTBRACE);
        match(SEMICOLON);
    }
    return node;
}

ASTNode* implDef() {
    write_derivation("implDef -> implement id { funcDefList }");
    ASTNode* node = createNode("implDef", "");
    if (lookahead_token == IMPLEMENT) {
        match(IMPLEMENT);
        if (lookahead_token == ID) {
            ASTNode* idnode = createNode("ID", yytext);
            idnode->line = lineno;
            match(ID);
            addChild(node, idnode);
        }
        match(LEFTBRACE);
        addChild(node, funcDefList());
        match(RIGHTBRACE);
    }
    return node;
}

ASTNode* funcDef() {
    write_derivation("funcDef -> funcHead funcBody");
    ASTNode* node = createNode("funcDef", "");
    // funcHead creates its own node and children
    addChild(node, funcHead());
    addChild(node, funcBody());
    return node;
}

ASTNode* funcDefList() {
    ASTNode* node = createNode("funcDefList", "");
    if (lookahead_token == FUNC || lookahead_token == CONSTRUCT) {
        addChild(node, funcDef());
        addChild(node, funcDefList());
        return node;
    } else {
        // epsilon
        return node;
    }
}

ASTNode* visibilitymemberDeclList() {
    ASTNode* node = createNode("visibilitymemberDeclList", "");
    if (lookahead_token == PUBLIC || lookahead_token == PRIVATE) {
        addChild(node, visibility());
        addChild(node, memberDecl());
        addChild(node, visibilitymemberDeclList());
    }
    return node;
}

ASTNode* visibility() {
    ASTNode* node = createNode("visibility", "");
    if (lookahead_token == PUBLIC) {
        write_derivation("visibility -> public");
        match(PUBLIC);
        addChild(node, createNode("PUBLIC", ""));
    } else if (lookahead_token == PRIVATE) {
        write_derivation("visibility -> private");
        match(PRIVATE);
        addChild(node, createNode("PRIVATE", ""));
    }
    return node;
}

// ========= RESOLVED AMBIGUITY IN memberDecl = funcDecl | funcDef | attributeDecl ========
ASTNode* memberDecl() {
    ASTNode* node = createNode("memberDecl", "");
    if (lookahead_token == FUNC || lookahead_token == CONSTRUCT) {
        ASTNode* head = funcHead();
        if (lookahead_token == SEMICOLON) {
            write_derivation("memberDecl -> funcDecl");
            addChild(node, head);
            match(SEMICOLON);
        } else if (lookahead_token == LEFTBRACE) {
            write_derivation("memberDecl -> funcDef");
            addChild(node, head);
            addChild(node, funcBody());
            if (lookahead_token == SEMICOLON) match(SEMICOLON);
        }
    } else if (lookahead_token == ATTRIBUTE) {
        write_derivation("memberDecl -> attributeDecl");
        addChild(node, attributeDecl());
    }
    else {
        printf("Expected 'func', 'construct', or 'attribute' in memberDecl\n");
    }
    return node;
}

ASTNode* funcHead() {
    ASTNode* node = createNode("funcHead", "");
    if (lookahead_token == FUNC) {
        write_derivation("funcHead -> func id (fParams) => returnType");
        match(FUNC);
        if (lookahead_token == ID) {
            {
                ASTNode* idnode = createNode("ID", yytext);
                idnode->line = lineno;
                addChild(node, idnode);
            }
            match(ID);
        }
        match(LEFTPAREN);
        addChild(node, fParams());
        match(RIGHTPAREN);
        match(ARROW);
        addChild(node, returnType());
    } else if (lookahead_token == CONSTRUCT) {
        write_derivation("funcHead -> constructor (fParams)");
        match(CONSTRUCT);
        match(LEFTPAREN);
        addChild(node, fParams());
        match(RIGHTPAREN);
    }
    else {
        printf("Expected 'func' or 'construct' in funcHead\n");
    }
    return node;
}

ASTNode* funcBody() {
    ASTNode* node = createNode("funcBody", "");
    if (lookahead_token == LEFTBRACE) {
        write_derivation("funcBody -> { varDeclOrStmtList }");
        match(LEFTBRACE);
        addChild(node, varDeclOrStmtList());
        match(RIGHTBRACE);
    }
    return node;
}

ASTNode* funcDecl() {
    ASTNode* node = createNode("funcDecl", "");
    if (lookahead_token == FUNC || lookahead_token == CONSTRUCT) {
        write_derivation("funcDecl -> funcHead ;");
        addChild(node, funcHead());
        match(SEMICOLON);
    }
    return node;
}

ASTNode* varDeclOrStmtList() {
    ASTNode* node = createNode("varDeclOrStmtList", "");
    if (lookahead_token == LOCAL || lookahead_token == ID || 
        lookahead_token == IF || lookahead_token == WHILE ||
        lookahead_token == READ || lookahead_token == WRITE ||
        lookahead_token == RETURN || lookahead_token == SELF) {
        addChild(node, varDeclOrStmt());
        addChild(node, varDeclOrStmtList());
    }
    return node;
}

ASTNode* varDeclOrStmt() {
    ASTNode* node = createNode("varDeclOrStmt", "");
    if (lookahead_token == LOCAL) {
        write_derivation("varDeclOrStmt -> localVarDecl");
        addChild(node, localVarDecl());
    } else {
        write_derivation("varDeclOrStmt -> statement");
        addChild(node, statement());
    }
    return node;
}

ASTNode* localVarDecl() {
    ASTNode* node = createNode("localVarDecl", "");
    if (lookahead_token == LOCAL) {
        write_derivation("localVarDecl -> local varDecl");
        match(LOCAL);
        addChild(node, varDecl());
    }
    return node;
}

ASTNode* varDecl() {
    ASTNode* node = createNode("varDecl", "");
    if (lookahead_token == ID) {
        {
            ASTNode* idnode = createNode("ID", yytext);
            idnode->line = lineno;
            addChild(node, idnode);
        }
        match(ID);
        match(COLON);
        addChild(node, type());
        addChild(node, arraySizeList());
        match(SEMICOLON);
    }
    return node;
}

ASTNode* attributeDecl() {
    ASTNode* node = createNode("attributeDecl", "");
    if (lookahead_token == ATTRIBUTE) {
        write_derivation("attributeDecl -> attribute varDecl");
        match(ATTRIBUTE);
        addChild(node, varDecl());
    }
    return node;
}

ASTNode* statement() {
    ASTNode* node = createNode("statement", "");
    if (lookahead_token == ID || lookahead_token == SELF) {
        // assignment or method call starting with id/self
        write_derivation("statement -> assignStat ;");
        addChild(node, assignStat());
        match(SEMICOLON);
        // addChild(node, idnestList());
        // if (lookahead_token == ID) {
        //     // peek ahead: if next token after ID is ASSIGN -> assignStat
        //     match(ID);
        //     if ( lookahead_token = LEFTPAREN ) {
        //         // function call
        //         write_derivation("statement -> functionCall ;");
        //         match(LEFTPAREN);
        //         addChild(node, aParams());
        //         match(RIGHTPAREN);
        //         match(SEMICOLON);
        //         return node;
        //     } else if (lookahead_token == ASSIGN) {
        //         addChild(node, indiceList());
        //         if (lookahead_token == ASSIGN) {
        //             write_derivation("statement -> assignStat ;");
        //             match(ASSIGN);
        //             addChild(node, expr());
        //             match(SEMICOLON);
        //             return node;
        //         }
        //     }
        // }
        return node;
    } else if (lookahead_token == IF) {
        write_derivation("statement -> if ( relExpr ) then statBlock else statBlock ;");
        addChild(node, createNode("if", ""));
        match(IF);
        match(LEFTPAREN);
        addChild(node, relExpr());
        match(RIGHTPAREN);
        match(THEN);
        addChild(node, statBlock());
        match(ELSE);
        addChild(node, statBlock());
        match(SEMICOLON);
    } else if (lookahead_token == WHILE) {
        write_derivation("statement -> while ( relExpr ) statBlock ;");
        addChild(node, createNode("while", ""));
        match(WHILE);
        match(LEFTPAREN);
        addChild(node, relExpr());
        match(RIGHTPAREN);
        addChild(node, statBlock());
        match(SEMICOLON);
    } else if (lookahead_token == READ) {
        write_derivation("statement -> read ( variable ) ;");
        addChild(node, createNode("read", ""));
        match(READ);
        match(LEFTPAREN);
        addChild(node, variable());
        match(RIGHTPAREN);
        match(SEMICOLON);
    } else if (lookahead_token == WRITE) {
        write_derivation("statement -> write ( expr ) ;");
        addChild(node, createNode("write", ""));
        match(WRITE);
        match(LEFTPAREN);
        addChild(node, expr());
        match(RIGHTPAREN);
        match(SEMICOLON);
    } else if (lookahead_token == RETURN) {
        write_derivation("statement -> return ( expr ) ;");
        addChild(node, createNode("return", ""));
        match(RETURN);
        match(LEFTPAREN);
        addChild(node, expr());
        match(RIGHTPAREN);
        match(SEMICOLON);
    }
    return node;
}

ASTNode* statBlock() {
    ASTNode* node = createNode("statBlock", "");
    if (lookahead_token == LEFTBRACE) {
        write_derivation("statBlock -> { statmentList }");
        match(LEFTBRACE);
        addChild(node, statmentList());
        match(RIGHTBRACE);
    } else {
        addChild(node, statement());
    }
    return node;
}

ASTNode* statmentList() {
    ASTNode* node = createNode("statmentList", "");
    if (lookahead_token == ID || lookahead_token == IF || 
        lookahead_token == WHILE || lookahead_token == READ ||
        lookahead_token == WRITE || lookahead_token == RETURN ||
        lookahead_token == SELF) {
        addChild(node, statement());
        addChild(node, statmentList());
    }
    return node;
}

ASTNode* assignStat() {
    ASTNode* node = createNode("assignStat", "");
    if (lookahead_token == ID || lookahead_token == SELF) {
        write_derivation("assignStat -> variable assignOp expr");
        addChild(node, variable());
        addChild(node, assignOp());
        addChild(node, expr());
    }
    return node;
}

ASTNode* assignOp() {
    ASTNode* node = createNode("assignOp", "");
    if (lookahead_token == ASSIGN) {
        write_derivation("assignOp -> :=");
        addChild(node, createNode(":=", ""));
        match(ASSIGN);
    }
    return node;
}

ASTNode* relExpr() {
    write_derivation("relExpr -> arithExpr relOp arithExpr");
    ASTNode* node = createNode("relExpr", "");
    addChild(node, arithExpr());
    addChild(node, relOp());
    addChild(node, arithExpr());
    return node;
}

ASTNode* relOp() {
    ASTNode* node = createNode("relOp", "");
    if (lookahead_token == LESS) {
        addChild(node, createNode("<", ""));
        match(LESS);
    } else if (lookahead_token == GREATER) {
        addChild(node, createNode(">", ""));
        match(GREATER);
    } else if (lookahead_token == LOEQ) {
        addChild(node, createNode("<=", ""));
        match(LOEQ);
    } else if (lookahead_token == GOEQ) {
        addChild(node, createNode(">=", ""));
        match(GOEQ);
    } else if (lookahead_token == NEQ) {
        addChild(node, createNode("<>", ""));
        match(NEQ);
    }
    return node;
}

ASTNode* expr() {
    // expr -> arithExpr [relOp arithExpr]
    ASTNode* node = createNode("expr", "");
    addChild(node, arithExpr());
    if (lookahead_token == LESS || lookahead_token == GREATER ||
        lookahead_token == LOEQ || lookahead_token == GOEQ ||
        lookahead_token == NEQ) {
        addChild(node, relOp());
        addChild(node, arithExpr());
    }
    return node;
}

ASTNode* arithExpr() {
    ASTNode* node = createNode("arithExpr", "");
    addChild(node, term());
    addChild(node, arithExprTail());
    return node;
}

ASTNode* term() {
    ASTNode* node = createNode("term", "");
    addChild(node, factor());
    addChild(node, termTail());
    return node;
}

ASTNode* idOrSelf() {
    ASTNode* node = createNode("idOrSelf", "");
    if (lookahead_token == ID) {
        {
            ASTNode* idnode = createNode("ID", yytext);
            idnode->line = lineno;
            addChild(node, idnode);
        }
        match(ID);
    } else if (lookahead_token == SELF) {
        addChild(node, createNode("SELF", yytext));
        match(SELF);
    }
    return node;
}

ASTNode* idnestTail() {
    ASTNode* node = createNode("idnestTail", "");
    if (lookahead_token == LEFTBRACKET) {
        addChild(node, indiceList());
        if (lookahead_token == DOT) match(DOT);
    } else if (lookahead_token == LEFTPAREN) {
        match(LEFTPAREN);
        addChild(node, aParams());
        match(RIGHTPAREN);
        if (lookahead_token == DOT) match(DOT);
    }
    return node;
}

ASTNode* idnest() {
    ASTNode* node = createNode("idnest", "");
    addChild(node, idOrSelf());
    addChild(node, idnestTail());
    return node;
}

ASTNode* idnestList() {
    ASTNode* node = createNode("idnestList", "");
    // idnestList -> idnest idnestList | epsilon
    if (lookahead_token == ID || lookahead_token == SELF) {
        addChild(node, idnest());
        addChild(node, idnestList());
    }
    return node;
}

ASTNode* indiceList() {
    ASTNode* node = createNode("indiceList", "");
    if (lookahead_token == LEFTBRACKET) {
        addChild(node, indice());
        addChild(node, indiceList());
    }
    return node;
}

ASTNode* indice() {
    write_derivation("indice -> [ arithExpr ]");
    ASTNode* node = createNode("indice", "");
    match(LEFTBRACKET);
    addChild(node, arithExpr());
    match(RIGHTBRACKET);
    return node;
}

ASTNode* variable() {
    write_derivation("variable -> idnestList id indiceList");
    ASTNode* node = createNode("variable", "");
    addChild(node, idnestList());
    if (lookahead_token == ID) {
        ASTNode* idnode = createNode("ID", yytext);
        idnode->line = lineno;
        addChild(node, idnode);
        match(ID);
    }
    addChild(node, indiceList());
    return node;
}

ASTNode* functionCall() {
    write_derivation("functionCall -> idnestList id ( aParams )");
    ASTNode* node = createNode("functionCall", "");
    addChild(node, idnestList());
    if (lookahead_token == ID) {
        ASTNode* idnode = createNode("ID", yytext);
        idnode->line = lineno;
        addChild(node, idnode);
        match(ID);
    }
    match(LEFTPAREN);
    addChild(node, aParams());
    match(RIGHTPAREN);
    return node;
}

ASTNode* aParams() {
    ASTNode* node = createNode("aParams", "");
    if (lookahead_token == ID || lookahead_token == INTEGER_LITERAL || 
        lookahead_token == FLOAT_LITERAL || lookahead_token == LEFTPAREN ||
        lookahead_token == NOT || lookahead_token == PLUS ||
        lookahead_token == MINUS || lookahead_token == SELF) {
        addChild(node, expr());
        addChild(node, aParamsTailList());
    }
    return node;
}

ASTNode* aParamsTailList() {
    ASTNode* node = createNode("aParamsTailList", "");
    if (lookahead_token == COMMA) {
        addChild(node, aParamsTail());
        addChild(node, aParamsTailList());
    }
    return node;
}

ASTNode* aParamsTail() {
    ASTNode* node = createNode("aParamsTail", "");
    match(COMMA);
    addChild(node, expr());
    return node;
}

ASTNode* arithExprTail() {
    ASTNode* node = createNode("arithExprTail", "");
    if (lookahead_token == PLUS || lookahead_token == MINUS || lookahead_token == OR) {
        addChild(node, addOp());
        addChild(node, term());
        addChild(node, arithExprTail());
    }
    return node;
}

ASTNode* addOp(){
    ASTNode* node = createNode("addOp", "");
    if(lookahead_token == PLUS){
        addChild(node, createNode("+", ""));
        match(PLUS);
    }
    else if(lookahead_token == MINUS){
        addChild(node, createNode("-", ""));
        match(MINUS);
    }
    else if(lookahead_token == OR){
        addChild(node, createNode("or", ""));
        match(OR);
    }
    return node;
}

ASTNode* termTail() {
    ASTNode* node = createNode("termTail", "");
    if (lookahead_token == MULTIPLY || lookahead_token == DIVIDE || 
        lookahead_token == AND) {
        addChild(node, multOp());
        addChild(node, factor());
        addChild(node, termTail());
    }
    return node;
}

ASTNode* multOp(){
    ASTNode* node = createNode("multOp", "");
    if(lookahead_token == MULTIPLY){
        addChild(node, createNode("*", ""));
        match(MULTIPLY);
    }
    else if(lookahead_token == DIVIDE){
        addChild(node, createNode("/", ""));
        match(DIVIDE);
    }
    else if(lookahead_token == AND){
        addChild(node, createNode("and", ""));
        match(AND);
    }
    return node;
}

ASTNode* sign() {
    ASTNode* node = createNode("sign", "");
    if (lookahead_token == PLUS) {
        addChild(node, createNode("+", ""));
        match(PLUS);
    } else if (lookahead_token == MINUS) {
        addChild(node, createNode("-", ""));
        match(MINUS);
    }
    return node;
}

ASTNode* factor() {
    ASTNode* node = createNode("factor", "");
    if (lookahead_token == ID || lookahead_token == SELF) {
        addChild(node, variable());
    } else if (lookahead_token == INTEGER_LITERAL) {
        addChild(node, createNode("intLit", yytext));
        match(INTEGER_LITERAL);
    } else if (lookahead_token == FLOAT_LITERAL) {
        addChild(node, createNode("floatLit", yytext));
        match(FLOAT_LITERAL);
    } else if (lookahead_token == LEFTPAREN) {
        match(LEFTPAREN);
        addChild(node, arithExpr());
        match(RIGHTPAREN);
    } else if (lookahead_token == NOT) {
        addChild(node, createNode("not", ""));
        match(NOT);
        addChild(node, factor());
    } else if (lookahead_token == PLUS || lookahead_token == MINUS) {
        addChild(node, sign());
        addChild(node, factor());
    }
    return node;
}

ASTNode* arraySizeList() {
    ASTNode* node = createNode("arraySizeList", "");
    if (lookahead_token == LEFTBRACKET) {
        write_derivation("arraySizeList -> arraySize arraySizeList");
        addChild(node, arraySize());
        addChild(node, arraySizeList());
    } else {
        write_derivation("arraySizeList -> ε");
    }
    return node;
}

ASTNode* arraySize() {
    ASTNode* node = createNode("arraySize", "");
    if (lookahead_token == LEFTBRACKET) {
        write_derivation("arraySize -> [ intLit ] | [ ]");
        match(LEFTBRACKET);
        if (lookahead_token == INTEGER_LITERAL) {
            addChild(node, createNode("intLit", yytext));
            match(INTEGER_LITERAL);
        }
        match(RIGHTBRACKET);
    }
    return node;
}


ASTNode* type() {
    ASTNode* node = createNode("type", "");
    if (lookahead_token == INTEGER) {
        addChild(node, createNode("integer", ""));
        match(INTEGER);
    } else if (lookahead_token == FLOAT) {
        addChild(node, createNode("float", ""));
        match(FLOAT);
    } else if (lookahead_token == ID) {
        {
            ASTNode* idnode = createNode("ID", yytext);
            idnode->line = lineno;
            addChild(node, idnode);
        }
        match(ID);
    }
    return node;
}

ASTNode* returnType() {
    ASTNode* node = createNode("returnType", "");
    if (lookahead_token == VOID) {
        addChild(node, createNode("void", ""));
        match(VOID);
    } else {
        addChild(node, type());
    }
    return node;
}

ASTNode* fParams() {
    ASTNode* node = createNode("fParams", "");
    if (lookahead_token == ID) {
        {
            ASTNode* idnode = createNode("ID", yytext);
            idnode->line = lineno;
            addChild(node, idnode);
        }
        match(ID);
        match(COLON);
        addChild(node, type());
        addChild(node, arraySizeList());
        addChild(node, fParamsTailList());
    }
    return node;
}

ASTNode* fParamsTailList() {
    ASTNode* node = createNode("fParamsTailList", "");
    if (lookahead_token == COMMA) {
        write_derivation("fParamsTailList -> fParamsTail fParamsTailList");
        addChild(node, fParamsTail());
        addChild(node, fParamsTailList());
    } else {
        write_derivation("fParamsTailList -> ε");
    }
    return node;
}

ASTNode* fParamsTail() {
    ASTNode* node = createNode("fParamsTail", "");
    if (lookahead_token == COMMA) {
        write_derivation("fParamsTail -> , id : type arraySizeList");
        match(COMMA);
        if (lookahead_token == ID) {
            ASTNode* idnode = createNode("ID", yytext);
            idnode->line = lineno;
            addChild(node, idnode);
            match(ID);
        }
        match(COLON);
        addChild(node, type());
        addChild(node, arraySizeList());
    } else {
        write_derivation("fParamsTail -> ε");
    }
    return node;
}

ASTNode* idTail() {
    ASTNode* node = createNode("idTail", "");
    if (lookahead_token == COMMA) {
        write_derivation("idTail -> , id idTail");
        match(COMMA);
        if (lookahead_token == ID) {
            ASTNode* idnode = createNode("ID", yytext);
            idnode->line = lineno;
            addChild(node, idnode);
            match(ID);
        }
        addChild(node, idTail());
    } else {
        write_derivation("idTail -> ε");
    }
    return node;
}

ASTNode* isaIdOpt() {
    ASTNode* node = createNode("isaIdOpt", "");
    if (lookahead_token == ISA) {
        write_derivation("isaIdOpt -> isa id idTail");
        match(ISA);
        if (lookahead_token == ID) {
            ASTNode* idnode = createNode("ID", yytext);
            idnode->line = lineno;
            addChild(node, idnode);
            match(ID);
        }
        addChild(node, idTail());
    } else {
        write_derivation("isaIdOpt -> ε");
    }
    return node;
}


// ====== Driver function to run recursive descent parser =====
// called by yacc
ASTNode* parse_program() {
    init_parser();
    printf("Starting parsing using recursive descent parser...\n");

    ASTNode* root = prog();
    printf("Final lookahead token (numeric): %d\n", lookahead_token);

    // safety limit prevents infinite loops.
    int safety = 0;
    while (lookahead_token != 0 && lookahead_token != EXIT && lookahead_token != PRINT_SYMBOLS && safety < 1000) {
        lookahead_token = yylex();
        safety++;
    }

    if (lookahead_token == 0) {
        printf("Parsing completed successfully!\n");
        write_derivation("Parsing completed!");
        write_syntax_tree(root);
        
        // start semantic analysis
        run_semantic(root);
    } else if (lookahead_token == EXIT) {
        printf("Parser:: EXIT.\n");
        write_derivation("Parsing completed (exit token)");
        write_syntax_tree(root);
    } else if (lookahead_token == PRINT_SYMBOLS) {
        printf("Parser:: PRINT_SYMBOLS.\n");
        write_derivation("Parsing completed (print_symbols token)");
        write_syntax_tree(root);
    } else {
        printf("Unexpected token at end of file\n");
    }

    fclose(log_file);
    printf("Derivation written to derivation.txt\n");
    return root;
}