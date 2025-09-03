#include "parser.h"
#include "y.tab.h"

FILE* log_file;
int lookahead_token; //to keep track of the lexemes (look the next lexmes coming from LA)
int derivation_step = 1;

// ====== Function to initialize the parser and get the lookahead token =====
void init_parser() {
    log_file = fopen("derivation.txt", "w");
    if (!log_file) {
        printf("Error:: Cannot create derivation file\n");
        exit(1);
    }
    fprintf(log_file,"======================================\n");
    fprintf(log_file,"EEX6363 - Compiler Construction.\n");
    fprintf(log_file,"Name: W.M.A.T.Wanninayake.\n");
    fprintf(log_file,"Reg: 321428456 | S.No: S92068456\n");
    fprintf(log_file,"======================================\n\n");
    fprintf(log_file, "Parsing Steps of Program:\n");
    fprintf(log_file, "================\n\n");

    lookahead_token = yylex();
}

// ====== Function to write the rules to the file =====
void write_derivation(char* rule) {
    fprintf(log_file, "Step %d: %s\n", derivation_step++, rule);
    printf("Derivation: %s\n", rule);
}

// ====== Function to match the expected and current token and notify issues =====
void match(int required_token) {
    if (lookahead_token == EXIT) {
        printf("Parser:: EXIT.\n");
        print_symbol_table();
        exit(0);
    }
    if (lookahead_token == PRINT_SYMBOL_TABLE) {
        printf("Parser:: PRINT_SYMBOL_TABLE.\n");
        print_symbol_table();
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

// ====== Function to handle the errors =====
void error(char* message) {
    printf("Parser Error Occured:: %s\n", message);
    fprintf(log_file, "ERROR: %s\n", message);
    fclose(log_file);
    exit(1);
}

// ====== Creating recursive functions for all the non-terminals defined in the BNF converted production rules =====
void prog() {
    write_derivation("prog -> classOrImplOrFuncList");
    classOrImplOrFuncList();
}

void classOrImplOrFuncList() {
    if (lookahead_token == CLASS || lookahead_token == IMPLEMENT || 
        lookahead_token == FUNC || lookahead_token == CONSTRUCT) {
        write_derivation("classOrImplOrFuncList -> classOrImplOrFunc classOrImplOrFuncList");
        classOrImplOrFunc();
        classOrImplOrFuncList();
    } else {
        write_derivation("classOrImplOrFuncList -> ε");
        return;
    }
}

void classOrImplOrFunc() {
    if (lookahead_token == CLASS) {
        write_derivation("classOrImplOrFunc -> classDecl");
        classDecl();
    } else if (lookahead_token == IMPLEMENT) {
        write_derivation("classOrImplOrFunc -> implDef");
        implDef();
    } else if (lookahead_token == FUNC || lookahead_token == CONSTRUCT) {
        write_derivation("classOrImplOrFunc -> funcDef");
        funcDef();
    } else {
        printf("Expected 'class', 'implement', 'func', or 'construct'");
        return;
    }
}

void classDecl() {
    if (lookahead_token == CLASS) {
        write_derivation("classDecl -> class id isaIdOpt { visibilitymemberDeclList } ;");
        match(CLASS);
        match(ID);
        isaIdOpt();
        match(LEFTBRACE);
        visibilitymemberDeclList();
        match(RIGHTBRACE);
        match(SEMICOLON);
    }
    else{
        return;
    }
}

void isaIdOpt() {
    if (lookahead_token == ISA) {
        write_derivation("isaIdOpt -> isa id idTail");
        match(ISA);
        match(ID);
        idTail();
    } else {
        write_derivation("isaIdOpt -> ε");
        return;
    }
}

void idTail() {
    if (lookahead_token == COMMA) {
        write_derivation("idTail -> , id idTail");
        match(COMMA);
        match(ID);
        idTail();
    } else {
        write_derivation("idTail -> ε");
        return;
    }
}

void visibilitymemberDeclList() {
    if (lookahead_token == PUBLIC || lookahead_token == PRIVATE) {
        write_derivation("visibilitymemberDeclList -> visibility memberDecl visibilitymemberDeclList");
        visibility();
        memberDecl();
        visibilitymemberDeclList();
    } else {
        write_derivation("visibilitymemberDeclList -> ε");
        return;
    }
}

void implDef() {
    if (lookahead_token == IMPLEMENT) {
        write_derivation("implDef -> implement id { funcDefList }");
        match(IMPLEMENT);
        match(ID);
        match(LEFTBRACE);
        funcDefList();
        match(RIGHTBRACE);
    } else {
        return;
    }
}

void funcDefList() {
    if (lookahead_token == FUNC || lookahead_token == CONSTRUCT) {
        write_derivation("funcDefList -> funcDef funcDefList");
        funcDef();
        funcDefList();
    } else {
        write_derivation("funcDefList -> ε");
        return;
    }
}

void funcDef() {
    if (lookahead_token == FUNC | lookahead_token == CONSTRUCT) {
        write_derivation("funcDef -> funcHead funcBody");
        funcHead();
        funcBody();
    } else {
        printf("Expected 'func' or 'constructor'");
        return;
    }
}

void visibility() {
    if (lookahead_token == PUBLIC) {
        write_derivation("visibility -> public");
        match(PUBLIC);
    } else if (lookahead_token == PRIVATE) {
        write_derivation("visibility -> private");
        match(PRIVATE);
    } else {
        printf("Did not find expected 'public' or 'private'");
    }
}


/*
[Ambiguity]
Both start with the same sequence (funcHead).
The only difference is next token ; (declaration) or a { }.
-Made parser to look ahead one token after parsing the function header to decide which rule to apply.
*/

//just calling the functions in memberDecl → funcDecl | attributeDecl cause this ambiguity
// void memberDecl() {
//     if (lookahead_token == FUNC || lookahead_token == CONSTRUCT) {
//         write_derivation("memberDecl -> funcDecl");
//         funcDecl();
//     } else if (lookahead_token == ATTRIBUTE) {
//         write_derivation("memberDecl -> attributeDecl");
//         attributeDecl();
//     } else {
//         printf("Expected function declaration or attribute declaration");
//     }
// }

void memberDecl() {
    if (lookahead_token == FUNC || lookahead_token == CONSTRUCT) {
        funcHead();
        if (lookahead_token == SEMICOLON) {
            write_derivation("memberDecl -> funcDecl");
            match(SEMICOLON);
        } else if (lookahead_token == LEFTBRACE) {
            write_derivation("memberDecl -> funcDef");
            funcBody();
            if (lookahead_token == SEMICOLON) {
                match(SEMICOLON);
            } else {
                printf("Expected ';' after function body");
            }
        } else {
            printf("Expected ';' or '{' after function header");
        }
    } else if (lookahead_token == ATTRIBUTE) {
        write_derivation("memberDecl -> attributeDecl");
        attributeDecl();
    } else {
        printf("Expected function declaration or attribute declaration");
        return;
    }
}

void funcHead() {
    if (lookahead_token == FUNC) {
        write_derivation("funcHead -> func id (fParams) => returnType");
        match(FUNC);
        match(ID);
        match(LEFTPAREN);
        fParams();
        match(RIGHTPAREN);
        match(ARROW);
        returnType();
    } else if (lookahead_token == CONSTRUCT) {
        write_derivation("funcHead -> constructor (fParams)");
        match(CONSTRUCT);
        match(LEFTPAREN);
        fParams();
        match(RIGHTPAREN);
    } else {
        printf("Did not found expected 'func' or 'constructor'");
        //return;
    }
}

void funcDecl() {
    if (lookahead_token == FUNC || lookahead_token == CONSTRUCT) {
        write_derivation("funcDecl -> funcHead ;");
        funcHead();
        match(SEMICOLON);
    } else {
        printf("Did not find expected 'func' or 'constructor'");
        //return;
    }
}

void funcBody() {
    if (lookahead_token == LEFTBRACE) {
        write_derivation("funcBody -> { varDeclOrStmtList }");
        match(LEFTBRACE);
        varDeclOrStmtList();
        match(RIGHTBRACE);
    } else {
        printf("Did not found expected '{' for funcBody");
        return;
    } 
}

void varDeclOrStmtList() {
    if (lookahead_token == LOCAL || lookahead_token == ID || 
        lookahead_token == IF || lookahead_token == WHILE ||
        lookahead_token == READ || lookahead_token == WRITE ||
        lookahead_token == RETURN || lookahead_token == SELF) {
        write_derivation("varDeclOrStmtList -> varDeclOrStmt varDeclOrStmtList");
        varDeclOrStmt();
        varDeclOrStmtList();
    } else {
        write_derivation("varDeclOrStmtList -> ε");
        return;
    }
}

void varDeclOrStmt() {
    if (lookahead_token == LOCAL) {
        write_derivation("varDeclOrStmt -> localVarDecl");
        localVarDecl();
    } else {
        write_derivation("varDeclOrStmt -> statement");
        statement();
    }
}

void attributeDecl() {
    if (lookahead_token == ATTRIBUTE) {
        write_derivation("attributeDecl -> attribute varDecl");
        match(ATTRIBUTE);
        varDecl();
    } else {
        printf("Did not found expected 'attribute'");
        //return;
    }
}

void localVarDecl() {
    if(lookahead_token == LOCAL) {
        write_derivation("localVarDecl -> local varDecl");
        match(LOCAL);
        varDecl();
    } else {
        printf("Did not found expected 'local'");
        //return;
    }
}

void varDecl() {
    if(lookahead_token == ID){
        write_derivation("varDecl -> id : type arraySizeList ;");
        match(ID);
        match(COLON);
        type();
        arraySizeList();
        match(SEMICOLON);
    }
    else{
        printf("Did not found expected 'id'");
        //return;
    }
}

void arraySizeList() {
    if (lookahead_token == LEFTBRACKET) {
        write_derivation("arraySizeList -> arraySize arraySizeList");
        arraySize();
        arraySizeList();
    } else {
        write_derivation("arraySizeList -> ε");
        return;
    }
}

/*
[Ambiguity]
assignStat → variable assignOp expr
functionCall → idnestList id ( aParams )
variable → idnestList id indiceList
*/
void statement() {
    if (lookahead_token == ID || lookahead_token == SELF) {
        idnestList();
        if (lookahead_token == ID) {
            match(ID);
            if (lookahead_token == LEFTPAREN) {
                // functionCall ;
                write_derivation("statement -> functionCall ;");
                match(LEFTPAREN);
                aParams();
                match(RIGHTPAREN);
                match(SEMICOLON);
                return;
            } else {
                // assignStat ;
                indiceList();
                if (lookahead_token == ASSIGN) {
                    write_derivation("statement -> assignStat ;");
                    match(ASSIGN);
                    expr();
                    match(SEMICOLON);
                    return;
                } else {
                    printf("Expected := for assignment or ( for function call");
                    return;
                }
            }
        } else {
            printf("Expected identifier after idnestList in statement");
            return;
        }
    } else if (lookahead_token == IF) {
        write_derivation("statement -> if ( relExpr ) then statBlock else statBlock ;");
        match(IF);
        match(LEFTPAREN);
        relExpr();
        match(RIGHTPAREN);
        match(THEN);
        statBlock();
        match(ELSE);
        statBlock();
        match(SEMICOLON);
    } else if (lookahead_token == WHILE) {
        write_derivation("statement -> while ( relExpr ) statBlock ;");
        match(WHILE);
        match(LEFTPAREN);
        relExpr();
        match(RIGHTPAREN);
        statBlock();
        match(SEMICOLON);
    } else if (lookahead_token == READ) {
        write_derivation("statement -> read ( variable ) ;");
        match(READ);
        match(LEFTPAREN);
        variable();
        match(RIGHTPAREN);
        match(SEMICOLON);
    } else if (lookahead_token == WRITE) {
        write_derivation("statement -> write ( expr ) ;");
        match(WRITE);
        match(LEFTPAREN);
        expr();
        match(RIGHTPAREN);
        match(SEMICOLON);
    } else if (lookahead_token == RETURN) {
        write_derivation("statement -> return ( expr ) ;");
        match(RETURN);
        match(LEFTPAREN);
        expr();
        match(RIGHTPAREN);
        match(SEMICOLON);
    } else {
        printf("Did not found expected 'statement'");
        return;
    }
}

void assignStat() {
    if (lookahead_token == ID || lookahead_token == SELF) {
        write_derivation("assignStat -> variable assignOp expr");
        variable();
        assignOp();
        expr();
    }
    else{
        printf("Did not found expected 'id' or 'self'");
        return;
    }
}

void assignOp() {
    if (lookahead_token == ASSIGN) {
        write_derivation("assignOp -> :=");
        match(ASSIGN);
    } else {
        printf("Did not found expected ':='");
        return;
    }
}

void statBlock() {
    if (lookahead_token == LEFTBRACE) {
        write_derivation("statBlock -> { statmentList }");
        match(LEFTBRACE);
        statmentList();
        match(RIGHTBRACE);
    } else if (
        lookahead_token == ID || lookahead_token == IF || 
        lookahead_token == WHILE || lookahead_token == READ ||
        lookahead_token == WRITE || lookahead_token == RETURN ||
        lookahead_token == SELF) {
        write_derivation("statBlock -> statement");
        statement();
    } else {
        write_derivation("statBlock -> ε");
        return;
    }
}

void statmentList() {
    if (lookahead_token == ID || lookahead_token == IF || 
        lookahead_token == WHILE || lookahead_token == READ ||
        lookahead_token == WRITE || lookahead_token == RETURN ||
        lookahead_token == SELF) {
        write_derivation("statmentList -> statement statmentList");
        statement();
        statmentList();
    } else {
        write_derivation("statmentList -> ε");
        return;
    }
}
// [Ambiguity]
// relExpr → arithExpr relOp arithExpr
// arithExpr → term arithExperTail
void expr() {
    if (lookahead_token == ID || lookahead_token == INTEGER_LITERAL || 
        lookahead_token == FLOAT_LITERAL || lookahead_token == LEFTPAREN ||
        lookahead_token == NOT || lookahead_token == PLUS ||
        lookahead_token == MINUS || lookahead_token == SELF) {
        // 
        arithExpr();
        if (lookahead_token == LESS || lookahead_token == GREATER ||
            lookahead_token == LOEQ || lookahead_token == GOEQ ||
            lookahead_token == NEQ) {
            relOp();
            arithExpr();
            write_derivation("expr -> relExpr");
            write_derivation("relExpr -> arithExpr relOp arithExpr"); 
        } else if (lookahead_token == RIGHTPAREN || lookahead_token == COMMA || lookahead_token == SEMICOLON) {
            write_derivation("expr -> arithExpr");
            write_derivation("arithExpr -> term arithExprTail");
        } else {
            printf("Expected relational operator in expression");
            return;
        }
    } else {
        printf("Did not found expected start of expression");
        return; 
    }
}

void relExpr() {
    write_derivation("relExpr -> arithExpr relOp arithExpr");
    arithExpr();
    if (lookahead_token == LESS || lookahead_token == GREATER ||
        lookahead_token == LOEQ || lookahead_token == GOEQ ||
        lookahead_token == NEQ) {
        relOp();
        arithExpr();
    } else {
        printf("Did not found expected relOp in relExpr");
        return;
    }
}

void relOp() {
    if (lookahead_token == LESS) {
        write_derivation("relOp -> <");
        match(LESS);
    } else if (lookahead_token == GREATER) {
        write_derivation("relOp -> >");
        match(GREATER);
    } else if (lookahead_token == LOEQ) {
        write_derivation("relOp -> <=");
        match(LOEQ);
    } else if (lookahead_token == GOEQ) {
        write_derivation("relOp -> >=");
        match(GOEQ);
    } else if (lookahead_token == NEQ) {
        write_derivation("relOp -> <>");
        match(NEQ);
    } else {
        printf("Did not found expected relOp");
        return;
    }
}

void arithExpr() {
    if (lookahead_token == ID || lookahead_token == INTEGER_LITERAL || 
        lookahead_token == FLOAT_LITERAL || lookahead_token == LEFTPAREN ||
        lookahead_token == NOT || lookahead_token == PLUS ||
        lookahead_token == MINUS || lookahead_token == SELF) {
        write_derivation("arithExpr -> term arithExprTail");
        term();
        arithExprTail();
        }
    else{
        printf("Did not found expected start of arithExpr");
        return; 
    }
}

void arithExprTail() {
    if (lookahead_token == PLUS || lookahead_token == MINUS || 
        lookahead_token == OR) {
        write_derivation("arithExprTail -> addOp term arithExprTail");
        addOp();
        term();
        arithExprTail();
    } else {
        write_derivation("arithExprTail -> ε");
        return;
    }
}

void addOp(){
    if(lookahead_token == PLUS){
        write_derivation("addOp -> +");
        match(PLUS);
    }
    else if(lookahead_token == MINUS){
        write_derivation("addOp -> -");
        match(MINUS);
    }
    else if(lookahead_token == OR){
        write_derivation("addOp -> or");
        match(OR);
    }
    else{
        printf("Did not found expected addOp");
        return;
    }
}

void term() {
    write_derivation("term -> factor termTail");
    factor();
    termTail();
}

void termTail() {
    if (lookahead_token == MULTIPLY || lookahead_token == DIVIDE || 
        lookahead_token == AND) {
        write_derivation("termTail -> multOp factor termTail");
        multOp();
        factor();
        termTail();
    } else {
        write_derivation("termTail -> ε");
        return;
    }
}

void multOp(){
    if(lookahead_token == MULTIPLY){
        write_derivation("multOp -> *");
        match(MULTIPLY);
    }
    else if(lookahead_token == DIVIDE){
        write_derivation("multOp -> /");
        match(DIVIDE);
    }
    else if(lookahead_token == AND){
        write_derivation("multOp -> and");
        match(AND);
    }
    else{
        printf("Did not found expected multOp");
        return;
    }
}

void factor() {
    if (lookahead_token == ID || lookahead_token == SELF) {
        idnestList();
        if (lookahead_token == ID) {
            match(ID);
            if (lookahead_token == LEFTPAREN) {
                write_derivation("factor -> functionCall");
                match(LEFTPAREN);
                aParams();
                match(RIGHTPAREN);
            } else {
                write_derivation("factor -> variable");
                indiceList();
            }
        } else {
            printf("Expected identifier after idnestList in factor");
            return;
        }
    } else if (lookahead_token == INTEGER_LITERAL) {
        write_derivation("factor -> intLit");
        match(INTEGER_LITERAL);
    } else if (lookahead_token == FLOAT_LITERAL) {
        write_derivation("factor -> floatLit");
        match(FLOAT_LITERAL);
    } else if (lookahead_token == LEFTPAREN) {
        write_derivation("factor -> ( arithExpr )");
        match(LEFTPAREN);
        arithExpr();
        match(RIGHTPAREN);
    } else if (lookahead_token == NOT) {
        write_derivation("factor -> not factor");
        match(NOT);
        factor();
    } else if (lookahead_token == PLUS || lookahead_token == MINUS) {
        write_derivation("factor -> sign factor");
        sign();
        factor();
    } else {
        printf("Invalid factor");
        return;
    }
}

void sign() {
    if (lookahead_token == PLUS) {
        write_derivation("sign -> +");
        match(PLUS);
    } else if (lookahead_token == MINUS) {
        write_derivation("sign -> -");
        match(MINUS);
    } else {
        printf("Did not found expected sign");
        return;
    }
}

void variable() {
    write_derivation("variable -> idnestList id indiceList");
    idnestList();
    match(ID);
    indiceList();
}

void idnestList() {
    write_derivation("idnestList -> idnest idnestList |  ε ");
    return;
}

void indiceList() {
    if (lookahead_token == LEFTBRACKET) {
        write_derivation("indiceList -> indice indiceList");
        indice();
        indiceList();
    } else {
        write_derivation("indiceList -> ε");
        return;
    }
}

void functionCall() {
    write_derivation("functionCall -> idnestList id ( aParams )");
    idnestList();
    match(ID);
    match(LEFTPAREN);
    aParams();
    match(RIGHTPAREN);
}

void idnest() {
    write_derivation("idnest -> idOrSelf idnestTail");
    idOrSelf();
    idnestTail();
}

void idnestTail() {
    if (lookahead_token == LEFTBRACKET) {
        write_derivation("idnestTail -> indiceList .");
        indiceList();
        match(DOT);
    } else if (lookahead_token == LEFTPAREN) {
        write_derivation("idnestTail -> (aParams) .");
        match(LEFTPAREN);
        aParams();
        match(RIGHTPAREN);
        match(DOT);
    } else {
        return;
    }
}

void idOrSelf() {
    if (lookahead_token == ID) {
        write_derivation("idOrSelf -> id");
        match(ID);
    } else if (lookahead_token == SELF) {
        write_derivation("idOrSelf -> self");
        match(SELF);
    } else {
        printf("Expected identifier or 'self'");
        return;
    }
}

void indice() {
    write_derivation("indice -> [ arithExpr ]");
    match(LEFTBRACKET);
    arithExpr();
    match(RIGHTBRACKET);
}

void arraySize() {
    if (lookahead_token == LEFTBRACKET) {
        write_derivation("arraySize -> [ intLit ] | [ ]");
        match(LEFTBRACKET);
        if (lookahead_token == INTEGER_LITERAL) {
            match(INTEGER_LITERAL);
        }
        match(RIGHTBRACKET);
    } else {
        printf("Did not found expected '[' for arraySize");
        //return;
    }
}

void type() {
    if (lookahead_token == INTEGER) {
        write_derivation("type -> integer");
        match(INTEGER);
    } else if (lookahead_token == FLOAT) {
        write_derivation("type -> float");
        match(FLOAT);
    } else if (lookahead_token == ID) {
        write_derivation("type -> id");
        match(ID);
    } else {
        printf("Did not found expected identifier");
        return;
    }
}

void returnType() {
    if (lookahead_token == VOID) {
        write_derivation("returnType -> void");
        match(VOID);
    } else {
        write_derivation("returnType -> type");
        type();
    }
}

void fParams() {
    if (lookahead_token == ID) {
        write_derivation("fParams -> id : type arraySizeList fParamsTailList");
        match(ID);
        match(COLON);
        type();
        arraySizeList();
        fParamsTailList();
    } else {
        write_derivation("fParams -> ε");
        return;
    }
}

void fParamsTailList() {
    if (lookahead_token == COMMA) {
        write_derivation("fParamsTailList -> fParamsTail fParamsTailList");
        fParamsTail();
        fParamsTailList();
    } else {
        write_derivation("fParamsTailList -> ε");
        return;
    }
}

void aParams() {
    if (lookahead_token == ID || lookahead_token == INTEGER_LITERAL || 
        lookahead_token == FLOAT_LITERAL || lookahead_token == LEFTPAREN ||
        lookahead_token == NOT || lookahead_token == PLUS ||
        lookahead_token == MINUS || lookahead_token == SELF) {
        write_derivation("aParams -> expr aParamsTailList");
        expr();
        aParamsTailList();
    } else {
        write_derivation("aParams -> ε");
        return;
    }
}

void aParamsTailList() {
    if (lookahead_token == COMMA) {
        write_derivation("aParamsTailList -> aParamsTail aParamsTailList");
        aParamsTail();
        aParamsTailList();
    } else {
        write_derivation("aParamsTailList -> ε");
        return;
    }
}

void fParamsTail() {
    if (lookahead_token == COMMA) {
        write_derivation("fParamsTail -> , id : type arraySizeList");
        match(COMMA);
        match(ID);
        match(COLON);
        type();
        arraySizeList();
    } else {
        write_derivation("fParamsTail -> ε");
        return;
    }
}

void aParamsTail() {
    write_derivation("aParamsTail -> , expr");
    match(COMMA);
    expr();
}

// ====== Driver function to run recursive descent parser =====
// called by yacc
void parse_program() {
    init_parser();
    printf("Starting parsing using recursive descent parser...\n");
    
    prog();
    
    if (lookahead_token == 0) {
        printf("Parsing completed successfully!\n");
        write_derivation("Parsing completed!");
    } else {
        printf("Unexpected token at end of file");
        return;
    }
    
    fclose(log_file);
    printf("Derivation written to derivation.txt\n");
}