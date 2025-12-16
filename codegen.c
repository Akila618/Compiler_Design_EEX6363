#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "codegen.h"
#include "symbol_table.h"
#include "stack.h"
#include "isa2.h"

static int temp_counter = 0;
static int label_counter = 0;
static char* new_temp() { char b[32]; snprintf(b,32,"t%d",++temp_counter); return strdup(b); }
static char* new_label() { char b[32]; snprintf(b,32,"L%d",++label_counter); return strdup(b); }

static char* emit_expr(ASTNode* n);
static char* emit_term(ASTNode* n);
static char* emit_arithExpr(ASTNode* n);
static void emit_statements(ASTNode* n);

// ================================== quadruple structures =============================
static Quadruple* quad_list = NULL;
static int quad_count = 0;
static int quad_cap = 0;

// stack-based variable address
int get_var_address(const char* name) {
    return get_variable_location(name);
}

Quadruple* get_quad_list() { return quad_list; }
int get_quad_count() { return quad_count; }

static void emit_quad(const char* op, const char* a1, const char* a2, const char* res) {
    if (quad_count >= quad_cap) { quad_cap = (quad_cap==0)?64:quad_cap*2; quad_list = realloc(quad_list, sizeof(Quadruple)*quad_cap); }
    quad_list[quad_count].op = op?strdup(op):NULL;
    quad_list[quad_count].arg1 = a1?strdup(a1):NULL;
    quad_list[quad_count].arg2 = a2?strdup(a2):NULL;
    quad_list[quad_count].res = res?strdup(res):NULL;
    quad_count++;
}


static ASTNode* find_child(ASTNode* n, const char* name) {
    if (!n) return NULL;
    for (ASTNode* c=n->child; c; c=c->sibling) if(strcmp(c->name, name)==0) return c;
    return NULL;
}

static char* find_first_lexeme(ASTNode* root) {
    if (!root) return NULL;
    
    if (root->lexeme && strlen(root->lexeme) > 0) {
        char c = root->lexeme[0];
        if (isalpha(c) || c == '_') { 
             if (strcmp(root->lexeme, "if")!=0 && strcmp(root->lexeme, "while")!=0 && 
                 strcmp(root->lexeme, "return")!=0 && strcmp(root->lexeme, "integer")!=0 &&
                 strcmp(root->lexeme, "float")!=0 && strcmp(root->lexeme, "void")!=0 &&
                 strcmp(root->lexeme, "func")!=0 && strcmp(root->lexeme, "class")!=0 &&
                 strcmp(root->lexeme, "implement")!=0 && strcmp(root->lexeme, "then")!=0 &&
                 strcmp(root->lexeme, "else")!=0) {
                 return strdup(root->lexeme);
             }
        }
    }
    
    for (ASTNode* c = root->child; c; c = c->sibling) {
        char* res = find_first_lexeme(c);
        if (res) return res;
    }
    return NULL;
}

static char* find_literal(ASTNode* root) {
    if (!root) return NULL;
    if (root->lexeme && (isdigit(root->lexeme[0]) || root->lexeme[0] == '-')) return strdup(root->lexeme);
    for (ASTNode* c = root->child; c; c = c->sibling) {
        char* res = find_literal(c);
        if (res) return res;
    }
    return NULL;
}

static char* get_operator(ASTNode* opNode) {
    if (!opNode) return strdup("+");
    
    if (opNode->lexeme && strlen(opNode->lexeme) > 0 && strchr("+-*/<>=", opNode->lexeme[0])) {
        return strdup(opNode->lexeme);
    }
    
    for (ASTNode* c = opNode->child; c; c = c->sibling) {
        printf("[DEBUG]   Checking child: name='%s', lexeme='%s'\n", c->name, c->lexeme ? c->lexeme : "(null)");
        if (c->lexeme && strlen(c->lexeme) > 0 && strchr("+-*/<>=", c->lexeme[0])) {
            printf("[DEBUG]     Found in child lexeme: '%s'\n", c->lexeme);
            return strdup(c->lexeme);
        }

        if (strcmp(c->name, "+") == 0) { printf("[DEBUG]     Found + by name\n"); return strdup("+"); }
        if (strcmp(c->name, "-") == 0) { printf("[DEBUG]     Found - by name\n"); return strdup("-"); }
        if (strcmp(c->name, "*") == 0) { printf("[DEBUG]     Found * by name\n"); return strdup("*"); }
        if (strcmp(c->name, "/") == 0) { printf("[DEBUG]     Found / by name\n"); return strdup("/"); }
        if (strcmp(c->name, "<") == 0) { printf("[DEBUG]     Found < by name\n"); return strdup("<"); }
        if (strcmp(c->name, ">") == 0) { printf("[DEBUG]     Found > by name\n"); return strdup(">"); }
        if (strcmp(c->name, "==") == 0) { printf("[DEBUG]     Found == by name\n"); return strdup("=="); }
        if (strcmp(c->name, "<=") == 0) { printf("[DEBUG]     Found <= by name\n"); return strdup("<="); }
        if (strcmp(c->name, ">=") == 0) { printf("[DEBUG]     Found >= by name\n"); return strdup(">="); }
        
        if (strcmp(c->name, "multOp") == 0 || strcmp(c->name, "addOp") == 0) {
            printf("[DEBUG]     Recursing into %s\n", c->name);
            char* op_result = get_operator(c);
            if (op_result && strlen(op_result) > 0) return op_result;
        }
    }
    
    if (strstr(opNode->name, "mult")) { printf("[DEBUG]   Fallback mult -> *\n"); return strdup("*"); }
    if (strstr(opNode->name, "div")) { printf("[DEBUG]   Fallback div -> /\n"); return strdup("/"); }
    if (strstr(opNode->name, "add")) { printf("[DEBUG]   Fallback add -> +\n"); return strdup("+"); }
    if (strstr(opNode->name, "sub")) { printf("[DEBUG]   Fallback sub -> -\n"); return strdup("-"); }
    
    printf("[DEBUG]   Final fallback -> +\n");
    return strdup("+");
}


// ===================================== stack operations =====================================
static char* emit_variable(ASTNode* n, int want_rvalue) {
    char* name = find_first_lexeme(n);
    if (!name) name = strdup("unknown_var");
    
    printf("[DEBUG] emit_variable: name='%s', is_local=%d\n", name, is_local_variable(name));
    
    // local variable 
    if (is_local_variable(name)) {
        int offset = get_stack_offset(name);
        printf("[DEBUG]   Local variable '%s' at offset %d\n", name, offset);
        
        // address calculation: addr_temp = BP + offset
        char offsetStr[16];
        snprintf(offsetStr, 16, "%d", offset);
        
        char* addrTemp = new_temp();
        emit_quad("frameAddr", "BP", offsetStr, addrTemp); 
        
        if (want_rvalue) {
            char* val = new_temp();
            emit_quad("loadStack", addrTemp, "0", val);
            free(name);
            return val;
        } else {
            char* marker = malloc(64);
            snprintf(marker, 64, "STACK[%s]", addrTemp);
            free(name);
            return marker;
        }
    }
    
    ASTNode* idxNode = NULL;
    if (n) {
        ASTNode* stack[50]; int top=0; stack[top++]=n;
        while(top>0) {
            ASTNode* cur = stack[--top];
            if (strstr(cur->name, "indice")) { idxNode = cur; break; }
            for(ASTNode* k=cur->child; k; k=k->sibling) stack[top++]=k;
        }
    }

    if (idxNode) {
        ASTNode* arith = NULL;
        for(ASTNode* k=idxNode->child; k; k=k->sibling) {
             if(strstr(k->name, "arithExpr")) { arith=k; break; }
        }
        if (!arith && idxNode->child) arith = idxNode->child;

        if (arith) {
            char* idx = emit_arithExpr(arith);
            char* width = strdup("4");
            char* off = new_temp();
            emit_quad("*", idx, width, off);
            free(idx); free(width);
            if (want_rvalue) {
                char* val = new_temp();
                emit_quad("load", name, off, val);
                free(name); free(off);
                return val;
            } else {
                char b[64]; snprintf(b, 64, "%s@%s", name, off);
                free(name); free(off);
                return strdup(b);
            }
        }
    }
    return name;
}

static char* emit_factor(ASTNode* n) {
    if (!n) return strdup("0");
    
    if (strcmp(n->name, "functionCall") == 0) {
        char* lbl = find_first_lexeme(n);
        if (!lbl) lbl = strdup("func");
        char* res = new_temp();
        char fLabel[64]; snprintf(fLabel, 64, "F_%s", lbl);
        emit_quad("call", fLabel, "0", res);
        free(lbl);
        return res;
    }

    ASTNode* child = n->child;
    if (child && strstr(child->name, "arithExpr")) return emit_arithExpr(child);
    if (child && strstr(child->name, "expr")) return emit_expr(child);

    if (strstr(n->name, "arithExpr")) return emit_arithExpr(n);
    if (strstr(n->name, "expr")) return emit_expr(n);

    char* lit = find_literal(n);
    if (lit) return lit;

    char* var = find_first_lexeme(n);
    if (var) return emit_variable(n, 1);

    if (n->child) return emit_factor(n->child);
    return strdup("0");
}

static char* emit_term(ASTNode* n) {
    if (!n) return strdup("0");
    char* left = emit_factor(find_child(n, "factor"));
    
    ASTNode* tail = find_child(n, "termTail");
    while (tail) {
        ASTNode* nextFactor = NULL;
        ASTNode* opNode = NULL;
        int foundData = 0;

        for (ASTNode* c = tail->child; c; c = c->sibling) {
            if (strcmp(c->name, "factor") == 0) {
                nextFactor = c;
                foundData = 1;
            } else if (!opNode && (strstr(c->name, "multOp") || strchr("*/", c->lexeme ? c->lexeme[0] : ' '))) {
                opNode = c;
            }
        }

        if (!foundData) break;
        
        char* op = get_operator(opNode ? opNode : tail);
        char* right = emit_factor(nextFactor);
        char* t = new_temp();
        
        emit_quad(op, left, right, t);
        free(left); free(right); free(op);
        left = t;
        
        tail = find_child(tail, "termTail");
    }
    return left;
}

static char* emit_arithExpr(ASTNode* n) {
    if (!n) return strdup("0");
    char* left = emit_term(find_child(n, "term"));
    
    ASTNode* tail = find_child(n, "arithExprTail");
    while (tail) {
        ASTNode* nextTerm = NULL;
        ASTNode* opNode = NULL;
        int foundData = 0;

        for (ASTNode* c = tail->child; c; c = c->sibling) {
            if (strcmp(c->name, "term") == 0) {
                nextTerm = c;
                foundData = 1;
            } else if (!opNode && (strstr(c->name, "addOp") || strchr("+-", c->lexeme ? c->lexeme[0] : ' '))) {
                opNode = c;
            }
        }

        if (!foundData) break;
        
        char* op = get_operator(opNode ? opNode : tail);
        char* right = emit_term(nextTerm);
        char* t = new_temp();
        
        emit_quad(op, left, right, t);
        free(left); free(right); free(op);
        left = t;
        
        tail = find_child(tail, "arithExprTail");
    }
    return left;
}

static char* emit_expr(ASTNode* n) {
    if (find_child(n, "relExpr")) {
        ASTNode* rel = find_child(n, "relExpr");
        return emit_arithExpr(find_child(rel, "arithExpr"));
    }
    return emit_arithExpr(find_child(n, "arithExpr"));
}

static void emit_statement(ASTNode* n) {
    if (!n) return;
    ASTNode* ch = n->child;
    if (!ch) return;

    if (strcmp(ch->name, "assignStat")==0) {
        ASTNode* v = find_child(ch, "variable");
        ASTNode* e = find_child(ch, "expr");
        if (v && e) {
            char* val = emit_expr(e);
            char* tgt = emit_variable(v, 0);
            
            // check if is stack-based variable
            if (strncmp(tgt, "STACK[", 6) == 0) {
                char* addrTemp = strdup(tgt + 6);
                char* bracket = strchr(addrTemp, ']');
                if (bracket) *bracket = '\0';
                emit_quad("storeStack", val, "0", addrTemp);
                free(addrTemp);
            } else {
                // global
                char* at = strchr(tgt, '@');
                if (at) {
                    *at = '\0';
                    emit_quad("store", val, at+1, tgt);
                } else {
                    emit_quad("assign", val, NULL, tgt);
                }
            }
            free(val); free(tgt);
        }
        return;
    }

    if (find_child(n, "while") || (ch && strcmp(ch->name, "while")==0)) {
        ASTNode* rel = find_child(n, "relExpr");
        
        ASTNode* blk = NULL;
        for(ASTNode* c=n->child; c; c=c->sibling) {
            if(strstr(c->name, "Block") || strstr(c->name, "List")) { blk=c; break; }
        }
        
        char* lStart = new_label();
        char* lEnd = new_label();
        emit_quad("label", NULL, NULL, lStart);
        
        if (rel) {
            ASTNode* lNode = find_child(rel, "arithExpr");
            ASTNode* rNode = lNode ? lNode->sibling->sibling : NULL; 
            if (!rNode) rNode = lNode;
            ASTNode* opNode = lNode ? lNode->sibling : NULL;
            
            char* l = emit_arithExpr(lNode);
            char* r = emit_arithExpr(rNode);
            char* op = get_operator(opNode);
            
            char* inv = "==";
            if (strcmp(op, "<")==0) inv = ">=";
            else if (strcmp(op, ">")==0) inv = "<=";
            else if (strcmp(op, "<=")==0) inv = ">";
            else if (strcmp(op, ">=")==0) inv = "<";
            else if (strcmp(op, "==")==0) inv = "<>";
            
            emit_quad(inv, l, r, lEnd);
            free(l); free(r); free(op);
        }
        if (blk) emit_statements(blk);
        emit_quad("goto", NULL, NULL, lStart);
        emit_quad("label", NULL, NULL, lEnd);
        return;
    }

    if (find_child(n, "if") || (find_child(n, "relExpr") && !find_child(n, "while"))) {
        ASTNode* rel = find_child(n, "relExpr");
        char* lElse = new_label();
        char* lEnd = new_label();
        
        if (rel) {
            ASTNode* lNode = find_child(rel, "arithExpr");
            ASTNode* rNode = lNode ? lNode->sibling->sibling : NULL;
            ASTNode* opNode = lNode ? lNode->sibling : NULL;
            char* l = emit_arithExpr(lNode);
            char* r = emit_arithExpr(rNode);
            char* op = get_operator(opNode);
            
            char* inv = "==";
            if (strcmp(op, "<")==0) inv = ">=";
            else if (strcmp(op, ">")==0) inv = "<=";
            else if (strcmp(op, "<=")==0) inv = ">";
            else if (strcmp(op, ">=")==0) inv = "<";
            else if (strcmp(op, "==")==0) inv = "<>";
            
            emit_quad(inv, l, r, lElse);
            free(l); free(r); free(op);
        }
        
        ASTNode* b1 = NULL; ASTNode* b2 = NULL;
        int foundFirst = 0;
        
        for(ASTNode* c=n->child; c; c=c->sibling) {
            if (strcmp(c->name, "if")==0 || strcmp(c->name, "relExpr")==0 || 
                strcmp(c->name, "then")==0 || strcmp(c->name, "else")==0) continue;
            
            if (!foundFirst) { b1 = c; foundFirst = 1; }
            else { b2 = c; }
        }
        
        if (b1) emit_statements(b1);
        emit_quad("goto", NULL, NULL, lEnd);
        emit_quad("label", NULL, NULL, lElse);
        if (b2) emit_statements(b2);
        emit_quad("label", NULL, NULL, lEnd);
        return;
    }

    if (find_child(n, "return") || (ch && strstr(ch->name, "return"))) {
        ASTNode* ex = find_child(n, "expr");
        char* v = ex ? emit_expr(ex) : NULL;
        emit_quad("return", v, NULL, NULL);
        if(v) free(v);
        return;
    }
    
    if (find_child(n, "write")) {
        ASTNode* ex = find_child(n, "expr");
        char* v = emit_expr(ex);
        emit_quad("write", v, NULL, NULL);
        free(v);
        return;
    }

    for (ASTNode* c=n->child; c; c=c->sibling) emit_statements(c);
}

static void emit_statements(ASTNode* n) {
    if (!n) return;
    if (strcmp(n->name, "statement")==0) { emit_statement(n); return; }
    for (ASTNode* c=n->child; c; c=c->sibling) emit_statements(c);
}

void traverse_func(ASTNode* n) {
    char* name = find_first_lexeme(find_child(n, "funcHead"));
    if (name) {
        char l[64]; snprintf(l, 64, "F_%s", name);
        emit_quad("label", NULL, NULL, l);
        
        // save base pointer
        emit_quad("pushBP", "BP", "SP", NULL);
        
        // update base pointer: BP := SP
        emit_quad("setBP", "SP", NULL, "BP");
        
        // allocate space for local variables
        emit_quad("allocFrame", "SP", "32", "SP"); // Allocate 32 bytes
        emit_statements(find_child(n, "funcBody"));
        emit_quad("restoreSP", "BP", NULL, "SP");
        emit_quad("popBP", "SP", NULL, "BP");
        
        emit_quad("return", NULL, NULL, NULL);
        free(name);
    }
}

void traverse_all(ASTNode* n) {
    if (!n) return;
    
    if (strcmp(n->name, "funcDef")==0) {
        traverse_func(n);
    }
    if (strcmp(n->name, "memberDecl")==0) {
        ASTNode* funcHead = find_child(n, "funcHead");
        ASTNode* funcBody = find_child(n, "funcBody");
        
        if (funcHead && funcBody) {
            char* name = find_first_lexeme(funcHead);
            if (name) {
                char l[64]; snprintf(l, 64, "F_%s", name);
                emit_quad("label", NULL, NULL, l);
                emit_statements(funcBody);
                emit_quad("return", NULL, NULL, NULL);
                free(name);
            }
        }
    }
    
    if (n->child) traverse_all(n->child);
    if (n->sibling) traverse_all(n->sibling);
}

// ====================================== write 3AC generated from the AST to file ===============================================
static void dump_3ac(FILE* f) {
    fprintf(f, "==========================================\n");
    fprintf(f, "           Three-Address Code\n");
    fprintf(f, "==========================================\n");
    for(int i=0; i<quad_count; i++) {
        Quadruple* q = &quad_list[i];
        
        // Label
        if (strcmp(q->op, "label") == 0) {
            fprintf(f, "%-6s:\n", q->res);
        }
        // stack frame operations
        else if (strcmp(q->op, "pushBP") == 0) {
        }
        else if (strcmp(q->op, "setBP") == 0) {
        }
        else if (strcmp(q->op, "allocFrame") == 0) {
        }
        else if (strcmp(q->op, "restoreSP") == 0) {  
        }
        else if (strcmp(q->op, "popBP") == 0) {  
        }
        // Stack Variable Access
        else if (strcmp(q->op, "frameAddr") == 0) {
            fprintf(f, "        %s := BP + (%s)\n", q->res, q->arg2);
        }
        else if (strcmp(q->op, "loadStack") == 0) {
            fprintf(f, "        %s := *%s\n", q->res, q->arg1);
        }
        else if (strcmp(q->op, "storeStack") == 0) {
            fprintf(f, "        *%s := %s\n", q->res, q->arg1);
        }
        // Control Flow
        else if (strcmp(q->op, "goto") == 0) {
            fprintf(f, "        goto %s\n", q->res);
        }
        else if (strchr("=<>", q->op[0]) || strcmp(q->op, "==")==0 || strcmp(q->op, "if")==0) {
             fprintf(f, "        if %s %s %s goto %s\n", q->arg1, q->op, q->arg2, q->res);
        }
        // Assignment & Memory
        else if (strcmp(q->op, "assign") == 0) {
            fprintf(f, "        %s := %s\n", q->res, q->arg1);
        }
        else if (strcmp(q->op, "store") == 0) {
            fprintf(f, "       *%s := %s  (offset %s)\n", q->res, q->arg1, q->arg2);
        }
        else if (strcmp(q->op, "load") == 0) {
            fprintf(f, "        %s := *%s (offset %s)\n", q->res, q->arg1, q->arg2);
        }
        // Function Calls
        else if (strcmp(q->op, "call") == 0) {
            fprintf(f, "        call %s, %s -> %s\n", q->arg1, q->arg2, q->res);
        }
        else if (strcmp(q->op, "return") == 0) {
            fprintf(f, "        return %s\n", q->arg1 ? q->arg1 : "");
        }
        // I/O
        else if (strcmp(q->op, "write") == 0) {
            fprintf(f, "        write %s\n", q->arg1);
        }
        // Arithmetic/Logical
        else {
            fprintf(f, "        %s := %s %s %s\n", q->res, q->arg1, q->op, q->arg2);
        }
    }
}

static const char* get_quad_name(const char* op) {
    if (strcmp(op, "+") == 0) return "ADD";
    if (strcmp(op, "-") == 0) return "SUB";
    if (strcmp(op, "*") == 0) return "MULT";
    if (strcmp(op, "/") == 0) return "DIV";
    if (strcmp(op, "assign") == 0) return "MOV";
    if (strcmp(op, "goto") == 0) return "JUMP";
    if (strcmp(op, "label") == 0) return "LABEL";
    if (strcmp(op, "call") == 0) return "CALL";
    if (strcmp(op, "return") == 0) return "RET";
    if (strcmp(op, "write") == 0) return "WRITE";
    if (strcmp(op, "load") == 0) return "LOAD";
    if (strcmp(op, "store") == 0) return "STORE";
    if (strcmp(op, "==") == 0) return "EQ";
    if (strcmp(op, "<") == 0)  return "LT";
    if (strcmp(op, ">") == 0)  return "GT";
    if (strcmp(op, "<=") == 0) return "LE";
    if (strcmp(op, ">=") == 0) return "GE";
    if (strcmp(op, "<>") == 0) return "NE";
    return op;
}

// ===================================== write quadruples to file ============================================
static void dump_quads(FILE* f) {
    fprintf(f, "==========================================\n");
    fprintf(f, "               Quadruples\n");
    fprintf(f, "==========================================\n");
    for(int i=0; i<quad_count; i++) {
        Quadruple* q = &quad_list[i];
        const char* opName = get_quad_name(q->op ? q->op : "nop");
        char* r = q->res ? q->res : "_";
        char* a1 = q->arg1 ? q->arg1 : "_";
        char* a2 = q->arg2 ? q->arg2 : "_";
        
        if (strcmp(opName, "LABEL") == 0) fprintf(f, "%3d: %-6s %s\n", i, opName, r);
        else if (strcmp(opName, "JUMP") == 0) fprintf(f, "%3d: %-6s %s\n", i, opName, r);
        else if (strcmp(opName, "WRITE") == 0) fprintf(f, "%3d: %-6s %s\n", i, opName, a1);
        else if (strcmp(opName, "MOV") == 0) fprintf(f, "%3d: %-6s %s, %s\n", i, opName, r, a1);
        else fprintf(f, "%3d: %-6s %s, %s, %s\n", i, opName, r, a1, a2);
    }
}

// ===================================== driver function =================================
ASTNode* generate_ir(ASTNode* root) {
    if (!root) return NULL;
    
    // Initialize stack management
    init_stack_manager();
    
    printf("[CODEGEN]: Traversing AST for 3AC...\n");
    traverse_all(root);
    
    #if defined(_WIN32) || defined(_WIN64)
    _mkdir("files");
    #else
    mkdir("files", 0755);
    #endif
    
    FILE* f = fopen("files/address_code.txt", "w");
    if (f) { dump_3ac(f); fclose(f); }

    FILE* fq = fopen("files/quads.txt", "w");
    if (fq) { dump_quads(fq); fclose(fq); }
    
    generate_isa2_target();
    return root;
}