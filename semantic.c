#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "symbol_table.h"
#include "ast.h"
#include "codegen.h"

static ASTNode* find_rightmost_id(ASTNode* n);
static ASTNode* find_child(ASTNode* node, const char* name);
static Type get_type_from_typeNode(ASTNode* typeNode); 
static ASTNode* g_root = NULL;
static FILE* errf = NULL;

//type kind names of the TypeKind enum
const char* type_kind_names[] = {
    "int",
    "float",
    "void",
    "class",
    "array",
    "unknown"
};

static int count_array_dims(ASTNode* node) {
    if (!node) return 0;
    int cnt = 0;
    if (strstr(node->name ? node->name : "", "array") || strstr(node->name ? node->name : "", "indice")) {
            for (ASTNode* c = node->child; c; c = c->sibling) {
                cnt++;
            }
        return cnt;
    }
    ASTNode* asl = NULL;
    for (ASTNode* c = node->child; c; c = c->sibling) {
        if (strstr(c->name ? c->name : "", "arraySize") != NULL) {
            asl = c;
            break;
        }
    }
    if (!asl) return 0;
    for (ASTNode* c = asl->child; c; c = c->sibling) {
        cnt++;
    }
    return cnt;
}

static Type make_array_type(Type base, int dims, ASTNode* declNode) {
    Type at = make_basic_type(TYPE_ARRAY);
    Type* elem = (Type*)malloc(sizeof(Type));
    *elem = base;
    at.elementType = elem;
    at.dimensions = dims;
    at.dimSizes = NULL;

    if (!declNode) return at;

    // find arraySizeList or arraySize node under declNode
    ASTNode* asl = NULL;
    typedef struct StackItem { ASTNode* node; struct StackItem* next; } StackItem;
    StackItem* stack = NULL;
    StackItem* s0 = (StackItem*)malloc(sizeof(StackItem)); s0->node = declNode; s0->next = stack; stack = s0;
    while (stack) {
        ASTNode* cur = stack->node;
        StackItem* next = stack->next; free(stack); stack = next;
        if (!cur) continue;
        if (cur->name && (strcmp(cur->name, "arraySizeList") == 0 || strcmp(cur->name, "arraySize") == 0)) { asl = cur; break; }
        if (cur->child) { StackItem* it = (StackItem*)malloc(sizeof(StackItem)); it->node = cur->child; it->next = stack; stack = it; }
        for (ASTNode* sib = cur->sibling; sib; sib = sib->sibling) { StackItem* it = (StackItem*)malloc(sizeof(StackItem)); it->node = sib; it->next = stack; stack = it; }
    }
    if (!asl) return at;

    // collect integer extents into a dynamic array
    size_t cap = 4; size_t cnt = 0; size_t* sizes = (size_t*)malloc(sizeof(size_t) * cap);
    int ok = 1;
    if (strcmp(asl->name, "arraySizeList") == 0) {
        for (ASTNode* a = asl->child; a; a = a->sibling) {
            if (!a) continue;
            if (strcmp(a->name, "arraySize") != 0) continue;
            if (a->child && a->child->name && strcmp(a->child->name, "intLit") == 0 && a->child->lexeme) {
                long v = atol(a->child->lexeme);
                if (v <= 0) { ok = 0; break; }
                if (cnt >= cap) { cap *= 2; sizes = (size_t*)realloc(sizes, sizeof(size_t) * cap); }
                sizes[cnt++] = (size_t)v;
            } else { ok = 0; break; }
        }
    } else if (strcmp(asl->name, "arraySize") == 0) {
        ASTNode* a = asl;
        if (a->child && a->child->name && strcmp(a->child->name, "intLit") == 0 && a->child->lexeme) {
            long v = atol(a->child->lexeme);
            if (v <= 0) ok = 0; else { sizes[cnt++] = (size_t)v; }
        } else ok = 0;
    }

    if (!ok || cnt == 0) {
        free(sizes);
        at.dimSizes = NULL;
        at.dimensions = 0;
    } else {
        // shrink to exact size
        sizes = (size_t*)realloc(sizes, sizeof(size_t) * cnt);
        at.dimSizes = sizes;
        at.dimensions = (int)cnt;
    }
    return at;
}

// scans the classdeclNode for attributeDecl or funcDef with the given name.
static Type resolve_member_type(Type classType, const char* memberName) {
    Type unknown = make_basic_type(TYPE_UNKNOWN);
    if (!memberName) return unknown;
    if (classType.kind != TYPE_CLASS) return unknown;
    SymbolEntry* classSym = st_lookup(classType.name);
    if (!classSym || !classSym->declNode) return unknown;
    ASTNode* classDecl = classSym->declNode;
    for (ASTNode* c = classDecl->child; c; c = c->sibling) {
        if (!c) continue;
        if (strcmp(c->name, "visibilitymemberDeclList") == 0) {
            for (ASTNode* m = c->child; m; m = m->sibling) {
                if (!m) continue;
                // attributeDecl -> varDecl
                if (strcmp(m->name, "attributeDecl") == 0) {
                    ASTNode* vd = m->child;
                    ASTNode* id = NULL;
                    ASTNode* typeNode = NULL;
                    if (vd) { id = find_child(vd, "ID"); typeNode = find_child(vd, "type"); }
                    if (id && id->lexeme && strcmp(id->lexeme, memberName) == 0) {
                        return get_type_from_typeNode(typeNode);
                    }
                }
                // function declarations inside class
                if (strcmp(m->name, "funcDef") == 0 || strcmp(m->name, "funcDecl") == 0) {
                    ASTNode* hd = (strcmp(m->name, "funcDef") == 0) ? m->child : find_child(m, "funcHead");
                    if (hd) {
                        ASTNode* id = find_child(hd, "ID");
                        ASTNode* rt = find_child(hd, "returnType");
                        if (id && id->lexeme && strcmp(id->lexeme, memberName) == 0) {
                            return rt ? get_type_from_typeNode(rt) : make_basic_type(TYPE_VOID);
                        }
                    }
                }
            }
        }
    }
    return unknown;
}

// lookup member type following parent chain if necessary
static Type resolve_member_with_inheritance(Type classType, const char* memberName) {
    Type cur = classType;
    while (cur.kind == TYPE_CLASS) {
        Type found = resolve_member_type(cur, memberName);
        if (found.kind != TYPE_UNKNOWN) return found;
        if (cur.parent_name && cur.parent_name[0]) {
            SymbolEntry* ps = st_lookup(cur.parent_name);
            if (!ps) break;
            cur = ps->type;
        } else break;
    }
    return make_basic_type(TYPE_UNKNOWN);
}

// forward declare helper used before its definition
static void add_symbol_if_missing(const char* lexeme, SymbolKind kind, Type t, ASTNode* declNode, int line, const char* dupMsg);

static ASTNode* find_child(ASTNode* node, const char* name) {
    if (!node) return NULL;
    for (ASTNode* c = node->child; c; c = c->sibling) {
        if (strcmp(c->name, name) == 0) return c;
    }
    return NULL;
}

// insert any ID nodes found as parameters
static void insert_params_recursive(ASTNode* n, SymbolEntry* current_class, SymbolEntry* current_function) {
    if (!n) return;
    if (strcmp(n->name, "ID") == 0 && n->lexeme) {
        ASTNode* sib = n->sibling;
        ASTNode* typeNode = NULL;
        for (int steps = 0; sib && steps < 6; sib = sib->sibling, ++steps) {
            if (strcmp(sib->name, "type") == 0) { typeNode = sib; break; }
        }
        Type ptype = get_type_from_typeNode(typeNode);
        // array size information from nearby subtree
        ASTNode* decl_for_sizes = typeNode ? typeNode : n;
        ASTNode* asl = NULL;
        typedef struct StackItem { ASTNode* node; struct StackItem* next; } StackItem;
        StackItem* stack = NULL;
        StackItem* s0 = (StackItem*)malloc(sizeof(StackItem)); s0->node = decl_for_sizes; s0->next = stack; stack = s0;
        while (stack) {
            ASTNode* cur = stack->node; StackItem* next = stack->next; free(stack); stack = next;
            if (!cur) continue;
            if (cur->name && (strcmp(cur->name, "arraySizeList") == 0 || strcmp(cur->name, "arraySize") == 0)) { asl = cur; break; }
            if (cur->child) { StackItem* it = (StackItem*)malloc(sizeof(StackItem)); it->node = cur->child; it->next = stack; stack = it; }
            for (ASTNode* sib = cur->sibling; sib; sib = sib->sibling) { StackItem* it = (StackItem*)malloc(sizeof(StackItem)); it->node = sib; it->next = stack; stack = it; }
        }
        int dims = 0;
        if (asl) {
            if (strcmp(asl->name, "arraySizeList") == 0) {
                for (ASTNode* a = asl->child; a; a = a->sibling) {
                    if (!a) continue;
                    if (strcmp(a->name, "arraySize") == 0) dims++;
                }
            } else if (strcmp(asl->name, "arraySize") == 0) dims = 1;
        }
        if (dims > 0) ptype = make_array_type(ptype, dims, decl_for_sizes);
    // Pass the node that actually contains the type/arraySize information
    ASTNode* decl_for_symbol = decl_for_sizes ? decl_for_sizes : n;
    add_symbol_if_missing(n->lexeme, SYM_PARAM, ptype, decl_for_symbol, n->line, "Duplicate parameter '%s'");
    }
    for (ASTNode* c = n->child; c; c = c->sibling) insert_params_recursive(c, current_class, current_function);
}

// add symbol in current scope
static void add_symbol_if_missing(const char* lexeme, SymbolKind kind, Type t, ASTNode* declNode, int line, const char* dupMsg) {
    if (!lexeme) return;
    SymbolEntry* existing = st_lookup_local(lexeme);
    if (existing) {
        if (existing->declNode != declNode) semantic_error_rule(line, "Duplicate declaration in same scope", dupMsg, lexeme);
    } else {
        SymbolEntry* e = st_add_symbol(lexeme, kind, t, declNode, line);
        if (!e) semantic_error_rule(line, "Duplicate declaration in same scope", dupMsg, lexeme);
    }
}

// create Type from type AST node
static Type get_type_from_typeNode(ASTNode* typeNode) {
    if (!typeNode) return make_basic_type(TYPE_UNKNOWN);
    
    // Direct type match
    if (strcmp(typeNode->name, "integer") == 0) return make_basic_type(TYPE_INT);
    if (strcmp(typeNode->name, "float") == 0) return make_basic_type(TYPE_FLOAT);
    if (strcmp(typeNode->name, "void") == 0) return make_basic_type(TYPE_VOID);
    
    // Check child
    if (typeNode->child) {
        ASTNode* ch = typeNode->child;
        
        if (strcmp(ch->name, "type") == 0 && ch->child) {
            ch = ch->child;
        }
        
        if (strcmp(ch->name, "integer") == 0) return make_basic_type(TYPE_INT);
        if (strcmp(ch->name, "float") == 0) return make_basic_type(TYPE_FLOAT);
        if (strcmp(ch->name, "void") == 0) return make_basic_type(TYPE_VOID);
        if (strcmp(ch->name, "ID") == 0 && ch->lexeme) return make_class_type(ch->lexeme);
    }
    
    // Check if typeNode itself is an ID (class type)
    if (strcmp(typeNode->name, "ID") == 0 && typeNode->lexeme) return make_class_type(typeNode->lexeme);
    
    return make_basic_type(TYPE_UNKNOWN);
}

static void declaration_pass_rec(ASTNode* node, SymbolEntry* current_class, SymbolEntry* current_function) {
    if (!node) return;
    if (strcmp(node->name, "classDecl") == 0) {
        ASTNode* id = find_child(node, "ID");
        if (id) {
            Type t = make_class_type(id->lexeme);
            ASTNode* isa = find_child(node, "isaIdOpt");
            if (isa && isa->child) {
                for (ASTNode* p = isa->child; p; p = p->sibling) {
                    if (strcmp(p->name, "ID") == 0 && p->lexeme) {
                        strncpy(t.parent_name, p->lexeme, sizeof(t.parent_name)-1);
                        t.parent_name[sizeof(t.parent_name)-1] = '\0';
                        break;
                    }
                }
            }
            add_symbol_if_missing(id->lexeme, SYM_CLASS, t, node, id->line, "Class declared more than once '%s'");
            isa = find_child(node, "isaIdOpt");
            if (isa && isa->child) {
                for (ASTNode* p = isa->child; p; p = p->sibling) {
                    if (strcmp(p->name, "ID") == 0) {
                        SymbolEntry* ps = st_lookup(p->lexeme);
                        if (!ps) semantic_error_rule(p->line, "Parent class must be a previously defined", "Undefined parent class '%s'", p->lexeme);
                        else if (ps->kind != SYM_CLASS) semantic_error_rule(p->line, "Parent must be a class", "'%s' is not a class", p->lexeme);
                    }
                }
            }
            st_enter_scope(id->lexeme);
            SymbolEntry* classSym = st_lookup(id->lexeme);
            ASTNode* members = find_child(node, "visibilitymemberDeclList");
            if (members) declaration_pass_rec(members, classSym, current_function);
            st_exit_scope();
        }
        declaration_pass_rec(node->sibling, current_class, current_function);
        return;
    }

    if (strcmp(node->name, "funcDef") == 0) {
        ASTNode* head = node->child; // funcHead
        if (head) {
            ASTNode* id = find_child(head, "ID");
            ASTNode* rt = find_child(head, "returnType");
            Type fnType = rt ? get_type_from_typeNode(rt) : make_basic_type(TYPE_VOID);
            if (id) {
                add_symbol_if_missing(id->lexeme, SYM_FUNCTION, fnType, node, id->line, "Duplicate function '%s'");
                st_enter_scope(id->lexeme);
                SymbolEntry* funcSym = st_lookup(id->lexeme);
                for (ASTNode* ch = head->child; ch; ch = ch->sibling) declaration_pass_rec(ch, current_class, funcSym);
                ASTNode* body = head->sibling;
                if (body) {
                    declaration_pass_rec(body, current_class, funcSym);
                }
                st_exit_scope();
            }
        }
        declaration_pass_rec(node->sibling, current_class, current_function);
        return;
    }

    if (strcmp(node->name, "implDef") == 0 || strcmp(node->name, "implement") == 0) {
        // implDef -> implement ID { funcDefList }
        ASTNode* id = find_child(node, "ID");
        if (id) {
            SymbolEntry* cs = st_lookup(id->lexeme);
            if (!cs) semantic_error_rule(id->line, "Implement rule: target must be a previously declared class", "Undefined class '%s' in implement", id->lexeme);
            else if (cs->kind != SYM_CLASS) semantic_error_rule(id->line, "Implement rule: target must be a class", "'%s' is not a class", id->lexeme);
            else {
                st_enter_scope(id->lexeme);
                SymbolEntry* targetClass = st_lookup(id->lexeme);
                for (ASTNode* c = node->child; c; c = c->sibling) {
                    declaration_pass_rec(c, targetClass, current_function);
                }
                st_exit_scope();
            }
        }
        declaration_pass_rec(node->sibling, current_class, current_function);
        return;
    }

    if (strcmp(node->name, "funcHead") == 0) {
        ASTNode* id = find_child(node, "ID");
        ASTNode* rt = find_child(node, "returnType");
        Type fnType = rt ? get_type_from_typeNode(rt) : make_basic_type(TYPE_VOID);
        if (id) {
            add_symbol_if_missing(id->lexeme, SYM_FUNCTION, fnType, node, id->line, "Duplicate function '%s'");
            ASTNode* params = find_child(node, "fParams");
            if (params) declaration_pass_rec(params, current_class, current_function);
        }
        declaration_pass_rec(node->sibling, current_class, current_function);
        return;
    }

    // handle function parameter list
    if (strcmp(node->name, "fParams") == 0) {
        // Use recursive insertion to catch parameters nested inside fParamsTailList
        insert_params_recursive(node, current_class, current_function);
        declaration_pass_rec(node->sibling, current_class, current_function);
        return;
    }

    if (strcmp(node->name, "funcDecl") == 0) {
        declaration_pass_rec(node->child, current_class, current_function);
        declaration_pass_rec(node->sibling, current_class, current_function);
        return;
    }
    if (strcmp(node->name, "attributeDecl") == 0) {
        // attributeDecl -> varDecl
        ASTNode* vd = node->child;
        ASTNode* id = find_child(vd, "ID");
        ASTNode* typeNode = find_child(vd, "type");
    Type base = get_type_from_typeNode(typeNode);
    int dims = count_array_dims(vd);
    Type t = dims > 0 ? make_array_type(base, dims, vd) : base;
        if (id) add_symbol_if_missing(id->lexeme, SYM_ATTRIBUTE, t, node, id->line, "Duplicate attribute '%s'");
        declaration_pass_rec(node->sibling, current_class, current_function);
        return;
    }

    

    if (strcmp(node->name, "varDecl") == 0) {
        ASTNode* id = find_child(node, "ID");
        ASTNode* typeNode = find_child(node, "type");
    Type t = get_type_from_typeNode(typeNode);
    int dims = count_array_dims(node);
    if (dims > 0) t = make_array_type(t, dims, node);
    if (id) add_symbol_if_missing(id->lexeme, SYM_VARIABLE, t, node, id->line, "Duplicate variable '%s'");
        declaration_pass_rec(node->sibling, current_class, current_function);
        return;
    }

    declaration_pass_rec(node->child, current_class, current_function);
    declaration_pass_rec(node->sibling, current_class, current_function);
    return;
}

void declaration_pass(ASTNode* root) {
    declaration_pass_rec(root, NULL, NULL);
}

// type checking 
static void resolution_pass_rec(ASTNode* node, SymbolEntry* current_class, SymbolEntry* current_function);
static Type resolve_id_chain(ASTNode* idnest, SymbolEntry* current_class, SymbolEntry* current_function, int reportErrors);
static Type type_check_pass_rec(ASTNode* node, SymbolEntry* current_class, SymbolEntry* current_func);

static int check_assignStat(ASTNode* assignNode, SymbolEntry* current_class, SymbolEntry* current_func);
static int check_varDecl(ASTNode* varDeclNode, SymbolEntry* current_class, SymbolEntry* current_func);
static int check_if_condition(ASTNode* ifNode, SymbolEntry* current_class, SymbolEntry* current_func);
static int check_while_condition(ASTNode* whileNode, SymbolEntry* current_class, SymbolEntry* current_func);
static int check_return_vs_function(ASTNode* returnNode, SymbolEntry* current_class, SymbolEntry* current_func);
static int check_functionCall_args_bool(ASTNode* callNode, SymbolEntry* current_class, SymbolEntry* current_func);

// write type checking results to semantic_errors.txt
void type_check_pass(ASTNode* root) {
    if (errf) fclose(errf);
    errf = fopen("files/semantic_errors.txt", "a");
    if (!errf) return;
    type_check_pass_rec(root, NULL, NULL);
    fclose(errf); errf = NULL;
}

// Helper to count indices in a variable node
static int count_variable_indices(ASTNode* varNode) {
    if (!varNode) return 0;
    int count = 0;
    typedef struct StackItem { ASTNode* node; struct StackItem* next; } StackItem;
    StackItem* stack = NULL;
    StackItem* s0 = (StackItem*)malloc(sizeof(StackItem)); 
    s0->node = varNode; s0->next = stack; stack = s0;
    while (stack) {
        ASTNode* cur = stack->node;
        StackItem* nx = stack->next; free(stack); stack = nx;
        if (!cur) continue;
        // Check if this is an indiceList with actual indice children
        if (cur->name && strcmp(cur->name, "indiceList") == 0) {
            for (ASTNode* ch = cur->child; ch; ch = ch->sibling) {
                if (ch->name && strcmp(ch->name, "indice") == 0) {
                    count++;
                }
            }
        }
        if (cur->child) {
            StackItem* it = (StackItem*)malloc(sizeof(StackItem)); 
            it->node = cur->child; it->next = stack; stack = it;
        }
        for (ASTNode* sib = cur->sibling; sib; sib = sib->sibling) {
            StackItem* it = (StackItem*)malloc(sizeof(StackItem)); 
            it->node = sib; it->next = stack; stack = it;
        }
    }
    return count;
}

static Type get_variable_type(ASTNode* idnode) {
    if (!idnode || strcmp(idnode->name, "ID") != 0) return make_basic_type(TYPE_UNKNOWN);
    SymbolEntry* s = st_lookup(idnode->lexeme);
    if (s) return s->type;
    return make_basic_type(TYPE_UNKNOWN);
}

static Type promote_arith_type(Type a, Type b) {
    if (a.kind == TYPE_FLOAT || b.kind == TYPE_FLOAT) return make_basic_type(TYPE_FLOAT);
    if (a.kind == TYPE_INT && b.kind == TYPE_INT) return make_basic_type(TYPE_INT);
    return make_basic_type(TYPE_UNKNOWN);
}

static int get_node_line(ASTNode* n) {
    if (!n) return 0;
    if (n->line) return n->line;
    for (ASTNode* c = n->child; c; c = c->sibling) {
        int l = get_node_line(c);
        if (l) return l;
    }
    return 0;
}

// check whether an AST node represents a return statement
static int is_return_node(ASTNode* n) {
    if (!n) return 0;
    if (strcmp(n->name, "returnStat") == 0 || strcmp(n->name, "returnStatement") == 0) return 1;
    if (strcmp(n->name, "statement") == 0 && n->child) {
        if ((n->child->name && strcmp(n->child->name, "RETURN") == 0) ||
            (n->child->lexeme && strcmp(n->child->lexeme, "return") == 0) ||
            (strstr(n->child->name ? n->child->name : "", "return") != NULL && strcmp(n->child->name, "returnType") != 0)) {
            return 1;
        }
    }
    return 0;
}

// recursively search subtree
static ASTNode* find_expr_in_subtree(ASTNode* n) {
    if (!n) return NULL;
    if (n->name && (strcmp(n->name, "expr") == 0 || strcmp(n->name, "arithExpr") == 0 || strcmp(n->name, "variable") == 0)) return n;
    for (ASTNode* c = n->child; c; c = c->sibling) {
        ASTNode* r = find_expr_in_subtree(c);
        if (r) return r;
    }
    return NULL;
}

// function declaration/definition AST node
static Type* get_param_types_from_decl(ASTNode* declNode, int* out_count) {
    *out_count = 0;
    if (!declNode) return NULL;
    ASTNode* fparams = find_child(declNode, "fParams");
    if (!fparams) {
        if (strcmp(declNode->name, "funcHead") == 0) fparams = find_child(declNode, "fParams");
    }
    if (!fparams) return NULL;
    int cnt = 0;
    for (ASTNode* ch = fparams->child; ch; ch = ch->sibling) {
        if (strcmp(ch->name, "ID") == 0) cnt++;
    }
    if (cnt == 0) {
        *out_count = 0;
        return NULL;
    }

    Type* types = (Type*)malloc(sizeof(Type) * cnt);
    int idx = 0;
    for (ASTNode* ch = fparams->child; ch; ch = ch->sibling) {
        if (strcmp(ch->name, "ID") == 0) {
            ASTNode* sib = ch->sibling;
            ASTNode* typeNode = NULL;
            for (int steps = 0; sib && steps < 6; sib = sib->sibling, ++steps) {
                if (strcmp(sib->name, "type") == 0) { typeNode = sib; break; }
            }
            types[idx++] = get_type_from_typeNode(typeNode);
        }
    }
    *out_count = idx;
    return types;
}

// check a function argument match declared parameter list
static void check_function_call_args(ASTNode* callNode, SymbolEntry* funcSym, SymbolEntry* current_class, SymbolEntry* current_func) {
    if (!callNode || !funcSym) return;
    ASTNode* aparams = find_child(callNode, "aParams");
    int argCount = 0;
    ASTNode* arg;
    if (aparams) {
        for (ASTNode* ch = aparams->child; ch; ch = ch->sibling) {
            if (strcmp(ch->name, "expr") == 0 || strcmp(ch->name, "arithExpr") == 0 || strcmp(ch->name,"variable")==0) argCount++;
        }
    }

    int paramCount = 0;
    Type* paramTypes = get_param_types_from_decl(funcSym->declNode, &paramCount);
    if (paramTypes || argCount > 0) {
        if (paramCount != argCount) {
            int line = get_node_line(callNode);
            semantic_error(line ? line : callNode->line, "Argument count mismatch in call to '%s' (expected %d, got %d)", funcSym->name, paramCount, argCount);
        }
    }
    if (paramTypes && aparams) {
        int pidx = 0;
        for (ASTNode* ch = aparams->child; ch; ch = ch->sibling) {
            if (!(strcmp(ch->name, "expr") == 0 || strcmp(ch->name, "arithExpr") == 0 || strcmp(ch->name,"variable")==0)) continue;
            Type at = type_check_pass_rec(ch, current_class, current_func);
            if (pidx < paramCount) {
                if (!type_equal(&at, &paramTypes[pidx])) {
                    int line = get_node_line(ch);
                    semantic_error(line ? line : ch->line, "Argument type mismatch in call to '%s' for parameter %d (expected %d, got %d)", funcSym->name, pidx+1, paramTypes[pidx].kind, at.kind);
                }
            }
            pidx++;
        }
    }

    if (paramTypes) free(paramTypes);
}

// main recursive type checking function
static Type type_check_pass_rec(ASTNode* node, SymbolEntry* current_class, SymbolEntry* current_func) {
    if (!node) return make_basic_type(TYPE_UNKNOWN);

    if (strcmp(node->name, "funcDef") == 0) {
        ASTNode* head = node->child; // funcHead
        SymbolEntry* funcSym = NULL;
        if (head) {
            ASTNode* id = find_child(head, "ID");
            if (id) {
                /* enter the function scope to lookup type checking find parameters and local variables */
                st_enter_scope(id->lexeme);
                funcSym = st_lookup(id->lexeme);
            }
            for (ASTNode* ch = head->child; ch; ch = ch->sibling) type_check_pass_rec(ch, current_class, funcSym);
            ASTNode* body = head->sibling;
            if (body) type_check_pass_rec(body, current_class, funcSym);
            if (id) st_exit_scope();
        }
        return type_check_pass_rec(node->sibling, current_class, current_func);
    }

    /* statBlock -> { statementList } */
    if (strcmp(node->name, "statBlock") == 0 || strcmp(node->name, "block") == 0) {
        st_enter_scope("<block>");
        type_check_pass_rec(node->child, current_class, current_func);
        st_exit_scope();
        return type_check_pass_rec(node->sibling, current_class, current_func);
    }

    if (strcmp(node->name, "classDecl") == 0) {
        for (ASTNode* ch = node->child; ch; ch = ch->sibling) {
            if (strcmp(ch->name, "ID") == 0) {
                st_enter_scope(ch->lexeme);
                SymbolEntry* classSym = st_lookup(ch->lexeme);
                for (ASTNode* m = node->child; m; m = m->sibling) {
                    if (strcmp(m->name, "visibilitymemberDeclList") == 0) {
                        type_check_pass_rec(m, classSym, current_func);
                        break;
                    }
                }
                st_exit_scope();
                break;
            }
        }
        return type_check_pass_rec(node->sibling, current_class, current_func);
    }

    if (strcmp(node->name, "implDef") == 0 || strcmp(node->name, "implement") == 0) {
        ASTNode* id = find_child(node, "ID");
        if (id) {
            st_enter_scope(id->lexeme);
            SymbolEntry* targetClass = st_lookup(id->lexeme);
            for (ASTNode* c = node->child; c; c = c->sibling) {
                type_check_pass_rec(c, targetClass, current_func);
            }
            st_exit_scope();
        }
        return type_check_pass_rec(node->sibling, current_class, current_func);
    }
    if (is_return_node(node)) {
        check_return_vs_function(node, current_class, current_func);
        ASTNode* expr = find_expr_in_subtree(node);
        Type exprType = make_basic_type(TYPE_UNKNOWN);
        if (expr) exprType = type_check_pass_rec(expr, current_class, current_func);
        return exprType;
    }
    // functionCall -> idnestList ( aParamsOpt )
    if (strcmp(node->name, "functionCall") == 0) {
        // find rightmost ID for target
        ASTNode* idnode = find_rightmost_id(node->child);
        if (idnode) {
            SymbolEntry* s = st_lookup(idnode->lexeme);
            if (s) {
                check_function_call_args(node, s, current_class, current_func);
                return s->type;
            } else {
                semantic_error(idnode->line, "Undeclared function '%s'", idnode->lexeme);
            }
        }
    for (ASTNode* c = node->child; c; c = c->sibling) {
        type_check_pass_rec(c, current_class, current_func);
    }
        return make_basic_type(TYPE_UNKNOWN);
    }

    // relExpr -> arithExpr relOp arithExpr
    if (strcmp(node->name, "relExpr") == 0) {
        ASTNode* left = node->child;
        ASTNode* op = left ? left->sibling : NULL;
        ASTNode* right = op ? op->sibling : NULL;
    Type lt = type_check_pass_rec(left, current_class, current_func);
    Type rt = type_check_pass_rec(right, current_class, current_func);
        if (!((lt.kind == TYPE_INT || lt.kind == TYPE_FLOAT) && (rt.kind == TYPE_INT || rt.kind == TYPE_FLOAT))) {
            if (!(lt.kind == TYPE_UNKNOWN || rt.kind == TYPE_UNKNOWN || (lt.kind == rt.kind && lt.kind != TYPE_UNKNOWN))) {
                int line = get_node_line(node);
                semantic_error(line ? line : node->line, "Type error: incompatible types for relational operator (%d vs %d)", lt.kind, rt.kind);
            }
        }
        return make_basic_type(TYPE_INT);
    }

    //assignStat -> variable assignOp expr
    if (strcmp(node->name, "assignStat") == 0) {
        ASTNode* varNode = node->child;
        ASTNode* assignOpNode = varNode ? varNode->sibling : NULL;
        ASTNode* exprNode = assignOpNode ? assignOpNode->sibling : NULL;
    Type varType = type_check_pass_rec(varNode, current_class, current_func);
    Type exprType = type_check_pass_rec(exprNode, current_class, current_func);
        if (varType.kind == TYPE_ARRAY) {
            int idxCount = 0;
            typedef struct StackItem3 { ASTNode* node; struct StackItem3* next; } StackItem3;
            StackItem3* st = NULL;
            StackItem3* s0 = (StackItem3*)malloc(sizeof(StackItem3)); s0->node = varNode; s0->next = st; st = s0;
            while (st) {
                ASTNode* cur = st->node; StackItem3* nx = st->next; free(st); st = nx;
                if (!cur) continue;
                if (cur->name && strcmp(cur->name, "indice") == 0) idxCount++;
                if (cur->child) { StackItem3* it = (StackItem3*)malloc(sizeof(StackItem3)); it->node = cur->child; it->next = st; st = it; }
                for (ASTNode* sib = cur->sibling; sib; sib = sib->sibling) { StackItem3* it = (StackItem3*)malloc(sizeof(StackItem3)); it->node = sib; it->next = st; st = it; }
            }
            if (idxCount > 0) {
                if (varType.dimensions <= idxCount) {
                    if (varType.elementType) varType = *(varType.elementType);
                    else varType = make_basic_type(TYPE_UNKNOWN);
                } else {
                    Type t = make_basic_type(TYPE_ARRAY);
                    t.elementType = (Type*)malloc(sizeof(Type));
                    *(t.elementType) = *(varType.elementType);
                    t.dimensions = varType.dimensions - idxCount;
                    t.dimSizes = NULL;
                    if (varType.dimSizes) {
                        t.dimSizes = (size_t*)malloc(sizeof(size_t) * t.dimensions);
                        for (int i = 0; i < t.dimensions; ++i) t.dimSizes[i] = varType.dimSizes[i + idxCount];
                    }
                    varType = t;
                }
            }
        }
        if (!type_equal(&varType, &exprType)) {
            int line = varNode && varNode->child ? varNode->child->line : node->line;
            semantic_error(line, "Type error: cannot assign expression of type '%s' to variable of type '%s'", type_kind_names[exprType.kind], type_kind_names[varType.kind]);
        }
        return varType;
    }

    // variable -> idnestList id indiceList
    if (strcmp(node->name, "variable") == 0) {
        ASTNode* idnode = find_rightmost_id(node->child);
        
        // Count how many indices are present using helper function
        int indiceCount = count_variable_indices(node);
        
        // Validate that indices are integers
        typedef struct StackItem4 { ASTNode* node; struct StackItem4* next; } StackItem4;
        StackItem4* stack = NULL;
        StackItem4* s0 = (StackItem4*)malloc(sizeof(StackItem4)); 
        s0->node = node; s0->next = stack; stack = s0;
        while (stack) {
            ASTNode* cur = stack->node;
            StackItem4* nx = stack->next; free(stack); stack = nx;
            if (!cur) continue;
            if (cur->name && strcmp(cur->name, "indice") == 0 && cur->child) {
                Type it = type_check_pass_rec(cur->child, current_class, current_func);
                if (it.kind != TYPE_INT && it.kind != TYPE_UNKNOWN) {
                    int line = get_node_line(cur);
                    semantic_error(line ? line : cur->line, "Type error: array index must be integer (got %d)", it.kind);
                }
            }
            if (cur->child) {
                StackItem4* it = (StackItem4*)malloc(sizeof(StackItem4)); 
                it->node = cur->child; it->next = stack; stack = it;
            }
            for (ASTNode* sib = cur->sibling; sib; sib = sib->sibling) {
                StackItem4* it = (StackItem4*)malloc(sizeof(StackItem4)); 
                it->node = sib; it->next = stack; stack = it;
            }
        }
        
        if (!idnode) return make_basic_type(TYPE_UNKNOWN);
        Type resolved = resolve_id_chain(node->child, current_class, current_func, 0);
        if (resolved.kind != TYPE_UNKNOWN) return resolved;
        if (node->child && strcmp(node->child->name, "ID") != 0) {
            Type childT = type_check_pass_rec(node->child, current_class, current_func);
            if (childT.kind == TYPE_CLASS) {
                Type m = resolve_member_with_inheritance(childT, idnode->lexeme);
                if (m.kind != TYPE_UNKNOWN) return m;
            }
        }
        
        Type varType = get_variable_type(idnode);
        
        // DEBUG: Print what found
        if (indiceCount > 0) {
            fprintf(stderr, "[DEBUG VAR] Variable %s has %d indices, varType.kind=%d, dimensions=%d\n", 
                    idnode ? idnode->lexeme : "NULL", indiceCount, varType.kind, varType.dimensions);
        }
        
        // If there is array indexing, reduce dimensions or return element type
        if (indiceCount > 0 && varType.kind == TYPE_ARRAY) {
            if (varType.dimensions == indiceCount) {
                if (varType.elementType) {
                    return *(varType.elementType);
                }
                return make_basic_type(TYPE_UNKNOWN);
            } else if (varType.dimensions > indiceCount) {
                Type reducedType = make_basic_type(TYPE_ARRAY);
                reducedType.elementType = varType.elementType;
                reducedType.dimensions = varType.dimensions - indiceCount;
                return reducedType;
            }
        }
        
        return varType;
    }

    // Integer
    if (strcmp(node->name, "intLit") == 0) {
        return make_basic_type(TYPE_INT);
    }
    // Float
    if (strcmp(node->name, "floatLit") == 0) {
        return make_basic_type(TYPE_FLOAT);
    }
    // Fraction
    if (strcmp(node->name, "fractionLit") == 0) {
        return make_basic_type(TYPE_FLOAT);
    }

    // arithExpr -> term arithExprTail
    if (strcmp(node->name, "arithExpr") == 0) {
        ASTNode* termNode = node->child;
        ASTNode* tailNode = termNode ? termNode->sibling : NULL;
    Type t1 = type_check_pass_rec(termNode, current_class, current_func);
    Type t2 = type_check_pass_rec(tailNode, current_class, current_func);
        if (!((t1.kind == TYPE_INT || t1.kind == TYPE_FLOAT || t1.kind == TYPE_UNKNOWN) &&
              (t2.kind == TYPE_INT || t2.kind == TYPE_FLOAT || t2.kind == TYPE_UNKNOWN))) {
            int line = get_node_line(node);
            semantic_error(line ? line : node->line, "Type error: arithmetic operands must be numeric (got %d and %d)", t1.kind, t2.kind);
        }
        return promote_arith_type(t1, t2);
    }
    // arithExprTail -> addOp term arithExprTail | ε
    if (strcmp(node->name, "arithExprTail") == 0) {
        if (!node->child) return make_basic_type(TYPE_INT); 
        ASTNode* addOpNode = node->child;
        ASTNode* termNode = addOpNode ? addOpNode->sibling : NULL;
        ASTNode* tailNode = termNode ? termNode->sibling : NULL;
    Type t1 = type_check_pass_rec(termNode, current_class, current_func);
    Type t2 = type_check_pass_rec(tailNode, current_class, current_func);
        if (!((t1.kind == TYPE_INT || t1.kind == TYPE_FLOAT || t1.kind == TYPE_UNKNOWN) &&
              (t2.kind == TYPE_INT || t2.kind == TYPE_FLOAT || t2.kind == TYPE_UNKNOWN))) {
            int line = get_node_line(node);
            semantic_error(line ? line : node->line, "Type error: arithmetic operands must be numeric (got %d and %d)", t1.kind, t2.kind);
        }
        return promote_arith_type(t1, t2);
    }
    // term -> factor termTail
    if (strcmp(node->name, "term") == 0) {
        ASTNode* factorNode = node->child;
        ASTNode* tailNode = factorNode ? factorNode->sibling : NULL;
    Type t1 = type_check_pass_rec(factorNode, current_class, current_func);
    Type t2 = type_check_pass_rec(tailNode, current_class, current_func);
        if (!((t1.kind == TYPE_INT || t1.kind == TYPE_FLOAT || t1.kind == TYPE_UNKNOWN) &&
              (t2.kind == TYPE_INT || t2.kind == TYPE_FLOAT || t2.kind == TYPE_UNKNOWN))) {
            int line = get_node_line(node);
            semantic_error(line ? line : node->line, "Type error: arithmetic operands must be numeric (got %d and %d)", t1.kind, t2.kind);
        }
        return promote_arith_type(t1, t2);
    }
    // termTail -> multOp factor termTail | ε
    if (strcmp(node->name, "termTail") == 0) {
        if (!node->child) return make_basic_type(TYPE_INT); // epsilon
        ASTNode* multOpNode = node->child;
        ASTNode* factorNode = multOpNode ? multOpNode->sibling : NULL;
        ASTNode* tailNode = factorNode ? factorNode->sibling : NULL;
    Type t1 = type_check_pass_rec(factorNode, current_class, current_func);
    Type t2 = type_check_pass_rec(tailNode, current_class, current_func);
        if (!((t1.kind == TYPE_INT || t1.kind == TYPE_FLOAT || t1.kind == TYPE_UNKNOWN) &&
              (t2.kind == TYPE_INT || t2.kind == TYPE_FLOAT || t2.kind == TYPE_UNKNOWN))) {
            int line = get_node_line(node);
            semantic_error(line ? line : node->line, "Type error: arithmetic operands must be numeric (got %d and %d)", t1.kind, t2.kind);
        }
        return promote_arith_type(t1, t2);
    }
    // factor -> variable | intLit | floatLit | fractionLit | (arithExpr) | not factor | sign factor
    if (strcmp(node->name, "factor") == 0) {
    if (!node->child) return make_basic_type(TYPE_UNKNOWN);
    return type_check_pass_rec(node->child, current_class, current_func);
    }

    // expr -> arithExpr | relExpr
    if (strcmp(node->name, "expr") == 0) {
    if (!node->child) return make_basic_type(TYPE_UNKNOWN);
    return type_check_pass_rec(node->child, current_class, current_func);
    }

    for (ASTNode* c = node->child; c; c = c->sibling) {
        type_check_pass_rec(c, current_class, current_func);
    }
    return make_basic_type(TYPE_UNKNOWN);
}

// find the rightmost ID node in a subtree for idnestList/idnest
static ASTNode* find_rightmost_id(ASTNode* n) {
    if (!n) return NULL;
    ASTNode* found = NULL;
    for (ASTNode* c = n; c; c = c->sibling) {
        if (strcmp(c->name, "ID") == 0) found = c;
        ASTNode* deeper = find_rightmost_id(c->child);
        if (deeper) found = deeper;
    }
    return found;
}
// collect all IDs in an idnest/idnestList
static char** collect_ids(ASTNode* n, int* out_count) {
    *out_count = 0;
    if (!n) return NULL;
    int cap = 8;
    char** arr = (char**)malloc(sizeof(char*) * cap);
    typedef struct StackItem { ASTNode* node; struct StackItem* next; } StackItem;
    StackItem* stack = NULL;
    StackItem* s0 = (StackItem*)malloc(sizeof(StackItem)); s0->node = n; s0->next = stack; stack = s0;
    while (stack) {
        ASTNode* cur = stack->node;
        StackItem* next = stack->next; free(stack); stack = next;
        if (!cur) continue;
        if (strcmp(cur->name, "ID") == 0 && cur->lexeme) {
            if (*out_count >= cap) { cap *= 2; arr = (char**)realloc(arr, sizeof(char*) * cap); }
            arr[*out_count] = strdup(cur->lexeme);
            (*out_count)++;
        }
        for (ASTNode* sib = cur->sibling; sib; sib = sib->sibling) {
            StackItem* it = (StackItem*)malloc(sizeof(StackItem)); it->node = sib; it->next = stack; stack = it;
        }
        if (cur->child) {
            StackItem* it = (StackItem*)malloc(sizeof(StackItem)); it->node = cur->child; it->next = stack; stack = it;
        }
    }
    return arr;
}

static void free_id_array(char** arr, int cnt) {
    if (!arr) return;
    for (int i = 0; i < cnt; ++i) free(arr[i]);
    free(arr);
}

static Type resolve_id_chain(ASTNode* idnest, SymbolEntry* current_class, SymbolEntry* current_function, int reportErrors) {
    Type unknown = make_basic_type(TYPE_UNKNOWN);
    if (!idnest) return unknown;
    int n = 0;
    char** ids = collect_ids(idnest, &n);
    if (n == 0) return unknown;
    Type curType = make_basic_type(TYPE_UNKNOWN);
    if (strcmp(ids[0], "self") == 0) {
        if (!current_class) {
            if (reportErrors) semantic_error_rule(idnest->line, "Self-use rule", "'self' used outside of method");
            free_id_array(ids, n);
            return unknown;
        }
        curType = current_class->type;
    } else {
        SymbolEntry* baseSym = st_lookup(ids[0]);
        if (!baseSym) {
            if (reportErrors) {
                int l = get_node_line(idnest);
                semantic_error(l ? l : idnest->line, "Undeclared identifier '%s'", ids[0]);
            }
            free_id_array(ids, n);
            return unknown;
        }
        curType = baseSym->type;
    }

    for (int i = 1; i < n; ++i) {
        const char* member = ids[i];
        if (curType.kind == TYPE_CLASS) {
            Type m = resolve_member_with_inheritance(curType, member);
            if (m.kind == TYPE_UNKNOWN) {
                if (reportErrors) semantic_error(idnest->line, "Member '%s' not found in type '%s'", member, curType.name);
                free_id_array(ids, n);
                return unknown;
            }
            curType = m;
        } else {
            if (reportErrors) semantic_error(idnest->line, "Cannot access member '%s' of non-class type", member);
            free_id_array(ids, n);
            return unknown;
        }
    }
    free_id_array(ids, n);
    return curType;
}

// scan an expression subtree
static void check_ids_in_expr(ASTNode* n) {
    if (!n) return;
    if (strcmp(n->name, "ID") == 0) {
        if (n->lexeme) {
            SymbolEntry* s = st_lookup(n->lexeme);
            if (!s) {
                int l = get_node_line(n);
                semantic_error(l ? l : n->line, "Undeclared identifier '%s'", n->lexeme);
            }
        }
        return;
    }
    for (ASTNode* c = n->child; c; c = c->sibling){
        check_ids_in_expr(c);
    } 
}

// checks semantics of the generated AST ==================================
static void resolution_pass_rec(ASTNode* node, SymbolEntry* current_class, SymbolEntry* current_function) {
    if (!node) return;

    if (strcmp(node->name, "classDecl") == 0) {
        // enter class scope
        for (ASTNode* ch = node->child; ch; ch = ch->sibling) {
            if (strcmp(ch->name, "ID") == 0) {
                st_enter_scope(ch->lexeme);
                SymbolEntry* classSym = st_lookup(ch->lexeme);
                // resolve inside visibilitymemberDeclList
                for (ASTNode* m = node->child; m; m = m->sibling) {
                    if (strcmp(m->name, "visibilitymemberDeclList") == 0) {
                        resolution_pass_rec(m, classSym, current_function);
                        break;
                    }
                }
                st_exit_scope();
                break;
            }
        }
    resolution_pass_rec(node->sibling, current_class, current_function);
        return;
    }

    if (strcmp(node->name, "implDef") == 0 || strcmp(node->name, "implement") == 0) {
        ASTNode* id = find_child(node, "ID");
        if (id) {
            st_enter_scope(id->lexeme);
            SymbolEntry* targetClass = st_lookup(id->lexeme);
            for (ASTNode* c = node->child; c; c = c->sibling) {
                resolution_pass_rec(c, targetClass, current_function);
            }
            st_exit_scope();
        }
        resolution_pass_rec(node->sibling, current_class, current_function);
        return;
    }

    if (strcmp(node->name, "funcDef") == 0) {
        // funcDef -> funcHead funcBody
        ASTNode* head = node->child;
        if (head) {
            for (ASTNode* ch = head->child; ch; ch = ch->sibling) {
                if (strcmp(ch->name, "ID") == 0) {
                    st_enter_scope(ch->lexeme);
                    // resolve params (if any) and body
                    SymbolEntry* funcSym = st_lookup(ch->lexeme);
                    for (ASTNode* hh = head->child; hh; hh = hh->sibling) {
                        resolution_pass_rec(hh, current_class, funcSym);
                    }
                    ASTNode* body = head->sibling;
                    if (body) resolution_pass_rec(body, current_class, funcSym);
                    st_exit_scope();
                    break;
                }
            }
        }
    resolution_pass_rec(node->sibling, current_class, current_function);
        return;
    }

    // statBlock -> { statementList }
    if (strcmp(node->name, "statBlock") == 0 || strcmp(node->name, "block") == 0) {
    // enter a new block scope
    st_enter_scope("<block>");
    resolution_pass_rec(node->child, current_class, current_function);
    st_exit_scope();
    resolution_pass_rec(node->sibling, current_class, current_function);
        return;
    }

    // variable -> idnestList id indiceList
    if (strcmp(node->name, "variable") == 0) {
        Type resolved = resolve_id_chain(node->child, current_class, current_function, 1);
        (void)resolved;
        resolution_pass_rec(node->child, current_class, current_function);
        resolution_pass_rec(node->sibling, current_class, current_function);
        return;
    }

    if (strcmp(node->name, "functionCall") == 0) {
        // idnestList ID (aParams)
        ASTNode* idnode = find_rightmost_id(node->child);
        if (idnode) {
            Type t = resolve_id_chain(node->child, current_class, current_function, 1);
            if (t.kind == TYPE_UNKNOWN) {
                semantic_error(idnode->line, "Undeclared function 814 '%s'", idnode->lexeme);
            }
        }
    resolution_pass_rec(node->child, current_class, current_function);
    resolution_pass_rec(node->sibling, current_class, current_function);
        return;
    }

    if (strcmp(node->name, "idOrSelf") == 0) {
       
        if (node->child && strcmp(node->child->name, "ID") == 0) {
            if (node->child->lexeme && strcmp(node->child->lexeme, "self") == 0) {
                if (!current_function || !current_class) {
                    semantic_error_rule(node->child->line, "Self-use rule: 'self' only valid inside methods", "'self' used outside of method");
                }
            } else {
                SymbolEntry* s = st_lookup(node->child->lexeme);
                if (!s) semantic_error_rule(node->child->line, "Name resolution", "Undeclared identifier '%s'", node->child->lexeme);
            }
        }
        resolution_pass_rec(node->sibling, current_class, current_function);
        return;
    }

    if (strcmp(node->name, "expr") == 0 || strcmp(node->name, "arithExpr") == 0 ||
        strcmp(node->name, "term") == 0 || strcmp(node->name, "factor") == 0 ||
        strcmp(node->name, "arithExprTail") == 0 || strcmp(node->name, "termTail") == 0 ||
        strcmp(node->name, "aParams") == 0) {
        check_ids_in_expr(node);
        
    resolution_pass_rec(node->child, current_class, current_function);
    resolution_pass_rec(node->sibling, current_class, current_function);
        return;
    }
    resolution_pass_rec(node->child, current_class, current_function);
    resolution_pass_rec(node->sibling, current_class, current_function);
}

void semantic_error_rule(int line, const char* rule, const char* fmt, ...) {
    if (!errf) errf = fopen("files/semantic_errors.txt", "a");
    if (!errf) return;
    va_list ap;
    va_start(ap, fmt);
    if (rule && rule[0]) fprintf(errf, "[%s] ", rule);
    fprintf(errf, "Line %d: ", line);
    vfprintf(errf, fmt, ap);
    fprintf(errf, "\n");
    va_end(ap);
}

// Basic error reporting
void semantic_error(int line, const char* fmt, ...) {
    if (!errf) errf = fopen("files/semantic_errors.txt", "a"); 
    if (!errf) return;
    va_list ap;
    va_start(ap, fmt);
    fprintf(errf, "Line %d: ", line);
    vfprintf(errf, fmt, ap);
    fprintf(errf, "\n");
    va_end(ap);
}

// Check assignment statement semantics
static int check_assignStat(ASTNode* assignNode, SymbolEntry* current_class, SymbolEntry* current_func) {
    if (!assignNode) return 1;
    ASTNode* varNode = assignNode->child;
    ASTNode* assignOpNode = varNode ? varNode->sibling : NULL;
    ASTNode* exprNode = assignOpNode ? assignOpNode->sibling : NULL;
    Type varType = type_check_pass_rec(varNode, current_class, current_func);
    Type exprType = type_check_pass_rec(exprNode, current_class, current_func);
    if (!type_equal(&varType, &exprType)) {
        int line = varNode && varNode->child ? varNode->child->line : get_node_line(assignNode);
        semantic_error_rule(line ? line : 0, "R10-assign", "Type error: cannot assign expression of type '%s' to variable of type '%s'", type_kind_names[exprType.kind], type_kind_names[varType.kind]);
        return 0;
    }
    return 1;
}

// Check variable declaration semantic
static int check_varDecl(ASTNode* varDeclNode, SymbolEntry* current_class, SymbolEntry* current_func) {
    if (!varDeclNode) return 1;
    ASTNode* id = find_child(varDeclNode, "ID");
    ASTNode* typeNode = find_child(varDeclNode, "type");
    Type base = get_type_from_typeNode(typeNode);
    int dims = count_array_dims(varDeclNode);
    Type t = dims > 0 ? make_array_type(base, dims, varDeclNode) : base;
    if (base.kind == TYPE_CLASS) {
        // ensure class exists
        SymbolEntry* cs = st_lookup(base.name);
        if (!cs) {
            int l = id ? id->line : get_node_line(varDeclNode);
            semantic_error_rule(l ? l : 0, "R7-decl", "Undefined class type '%s' in variable declaration", base.name);
            return 0;
        }
    }
    return 1;
}

// check if-condition is boolean 
static int check_if_condition(ASTNode* ifNode, SymbolEntry* current_class, SymbolEntry* current_func) {
    if (!ifNode) return 1;
    ASTNode* cond = NULL;
    for (ASTNode* c = ifNode->child; c; c = c->sibling) {
        if (strcmp(c->name, "relExpr") == 0 || strcmp(c->name, "expr") == 0 || strcmp(c->name, "arithExpr") == 0) { cond = c; break; }
    }
    if (!cond) return 1;
    Type ct = type_check_pass_rec(cond, current_class, current_func);
    if (ct.kind != TYPE_INT && ct.kind != TYPE_UNKNOWN) {
        int l = get_node_line(cond);
        semantic_error_rule(l ? l : 0, "R11-if", "Condition expression must be Boolean (got %d)", ct.kind);
        return 0;
    }
    return 1;
}

// check while condition is boolean
static int check_while_condition(ASTNode* whileNode, SymbolEntry* current_class, SymbolEntry* current_func) {
    if (!whileNode) return 1;
    ASTNode* cond = NULL;
    for (ASTNode* c = whileNode->child; c; c = c->sibling) {
        if (strcmp(c->name, "relExpr") == 0 || strcmp(c->name, "expr") == 0 || strcmp(c->name, "arithExpr") == 0) { cond = c; break; }
    }
    if (!cond) return 1;
    Type ct = type_check_pass_rec(cond, current_class, current_func);
    if (ct.kind != TYPE_INT && ct.kind != TYPE_UNKNOWN) {
        int l = get_node_line(cond);
        semantic_error_rule(l ? l : 0, "R12-while", "Condition expression must be Boolean (got %d)", ct.kind);
        return 0;
    }
    return 1;
}

// return expression vs enclosing function return type
static int check_return_vs_function(ASTNode* returnNode, SymbolEntry* current_class, SymbolEntry* current_func) {
    if (!returnNode) return 1;
    ASTNode* expr = find_expr_in_subtree(returnNode);
    Type exprType = make_basic_type(TYPE_UNKNOWN);
    if (expr) exprType = type_check_pass_rec(expr, current_class, current_func);
    if (!current_func) {
        int l = get_node_line(returnNode);
        semantic_error_rule(l ? l : 0, "R14-return", "Return statement not inside a function");
        return 0;
    }
    if (current_func->type.kind == TYPE_VOID) {
        if (expr) {
            int l = get_node_line(expr);
            semantic_error_rule(l ? l : 0, "R14-return", "Void function should not return a value");
            return 0;
        }
    } else {
        if (!type_equal(&current_func->type, &exprType)) {
            int l = expr ? get_node_line(expr) : get_node_line(returnNode);
            semantic_error_rule(l ? l : 0, "R14-return", "Return type mismatch (expected %d, got %d)", current_func->type.kind, exprType.kind);
            return 0;
        }
    }
    return 1;
}

// function call arguments and return boolean
static int check_functionCall_args_bool(ASTNode* callNode, SymbolEntry* current_class, SymbolEntry* current_func) {
    if (!callNode) return 1;
    ASTNode* idnode = find_rightmost_id(callNode->child);
    if (!idnode) return 1;
    SymbolEntry* s = st_lookup(idnode->lexeme);
    if (!s) {
        int l = get_node_line(idnode);
        semantic_error_rule(l ? l : idnode->line, "R21-call", "Undeclared function '%s'", idnode->lexeme);
        return 0;
    }
    // count args
    ASTNode* aparams = find_child(callNode, "aParams");
    int argCount = 0;
    if (aparams) {
        for (ASTNode* ch = aparams->child; ch; ch = ch->sibling) {
            if (strcmp(ch->name, "expr") == 0 || strcmp(ch->name, "arithExpr") == 0 || strcmp(ch->name, "variable") == 0) argCount++;
        }
    }
    int paramCount = 0;
    Type* paramTypes = get_param_types_from_decl(s->declNode, &paramCount);
    if (paramTypes || argCount > 0) {
        if (paramCount != argCount) {
            int l = get_node_line(callNode);
            semantic_error_rule(l ? l : callNode->line, "R21-call", "Argument count mismatch in call to '%s' (expected %d, got %d)", s->name, paramCount, argCount);
            if (paramTypes) free(paramTypes);
            return 0;
        }
    }
    if (paramTypes && aparams) {
        int pidx = 0;
        for (ASTNode* ch = aparams->child; ch; ch = ch->sibling) {
            if (!(strcmp(ch->name, "expr") == 0 || strcmp(ch->name, "arithExpr") == 0 || strcmp(ch->name, "variable") == 0)) continue;
            Type at = type_check_pass_rec(ch, current_class, current_func);
            if (pidx < paramCount) {
                if (!type_equal(&at, &paramTypes[pidx])) {
                    int line = get_node_line(ch);
                    semantic_error_rule(line ? line : ch->line, "R21-call", "Argument type mismatch in call to '%s' for parameter %d (expected %d, got %d)", s->name, pidx+1, paramTypes[pidx].kind, at.kind);
                    free(paramTypes);
                    return 0;
                }
            }
            pidx++;
        }
    }
    if (paramTypes) free(paramTypes);
    return 1;
}

// semantic analyzer driver function == starts semantic analysis ========================
void run_semantic(ASTNode* root) {
    g_root = root;
    st_init();

    #if defined(_WIN32) || defined(_WIN64)
    _mkdir("files");
    #else
    mkdir("files", 0755);
    #endif
    
    errf = fopen("files/semantic_errors.txt", "w");
    if (!errf) {
        fprintf(stderr, "Cannot open files/semantic_errors.txt for writing\n");
    }

    // builds symbol table *===========
    declaration_pass(root);

    resolution_pass_rec(root, NULL, NULL);

    st_compute_all_frame_layouts();

    
    if (errf) { fclose(errf); errf = NULL; }

    st_write_file("files/symbol_table.txt");
    type_check_pass(root);

    /* check whether any semantic errors were written. */
    int has_errors = 0;
    FILE* ef = fopen("files/semantic_errors.txt", "r");
    if (ef) {
        if (fseek(ef, 0, SEEK_END) == 0) {
            long sz = ftell(ef);
            if (sz > 0) has_errors = 1;
        }
        fclose(ef);
    }

    if (has_errors) {
        printf("======================================================================================================\n");
        fprintf(stderr, "[SEMANTIC: ERROR]: Semantic errors found; check files/semantic_errors.txt to resolve.\n");
        printf("======================================================================================================\n");
        return 0;  // test
    } else {
        printf("======================================================================================================\n");
        printf("[SEMANTIC: OK]: No semantic errors. Tree and symbol table ready for intermediate code generation.\n");
        printf("======================================================================================================\n");
    }
    generate_ir(root);

}
