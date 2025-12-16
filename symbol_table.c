#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
static Scope* current_scope = NULL;
static Scope* all_scopes = NULL;
static int current_level = 0;
static int compute_layout_for_scope_ptr(Scope* target);

// create and initialize a scope
static Scope* create_scope(const char* name, int level, Scope* parent) {
    Scope* s = (Scope*)malloc(sizeof(Scope));
    s->level = level;
    strncpy(s->name, name, sizeof(s->name)-1);
    s->name[sizeof(s->name)-1] = '\0';
    s->symbols = NULL;
    s->parent = parent;
    s->nextSibling = all_scopes;
    all_scopes = s;
    return s;
}

void st_init() {
    if (current_scope) return;
    current_scope = create_scope("<global>", 0, NULL);
    current_level = 0;
}

// ====================================== support functions for symbol table operations ========================================
void st_enter_scope(const char* name) {
    if (!current_scope) st_init();
    for (Scope* sc = all_scopes; sc; sc = sc->nextSibling) {
        if (strcmp(sc->name, name) == 0 && sc->parent == current_scope) {
            current_scope = sc;
            current_level++;
            return;
        }
    }
    Scope* s = create_scope(name, current_level + 1, current_scope);
    current_scope = s;
    current_level++;
}

void st_exit_scope() {
    if (!current_scope) return;
    if (current_scope->parent) current_scope = current_scope->parent;
    if (current_level > 0) current_level--;
}

SymbolEntry* st_add_symbol(const char* name, SymbolKind kind, Type type, struct ASTNode* declNode, int line) {
    if (!current_scope) st_init();
    // check duplicate in local scope
    for (SymbolEntry* s = current_scope->symbols; s; s = s->next) {
        if (strcmp(s->name, name) == 0) return NULL; // duplicate
    }
    SymbolEntry* ent = (SymbolEntry*)malloc(sizeof(SymbolEntry));
    strncpy(ent->name, name, sizeof(ent->name)-1);
    ent->name[sizeof(ent->name)-1] = '\0';
    ent->kind = kind;
    ent->type = type;
    ent->scopeLevel = current_scope->level;
    ent->declNode = declNode;
    ent->line = line;
    // compute and store width
    extern size_t compute_symbol_size(const Type* t, struct ASTNode* declNode);
    if (ent->type.kind == TYPE_ARRAY && ent->type.dimSizes) {
        ent->width = compute_type_size(&ent->type);
    } else {
        ent->width = compute_symbol_size(&ent->type, declNode);
        if (ent->width == 0) {
            ent->width = compute_type_size(&ent->type);
        }
    }
    ent->offset = INT_MIN;
    ent->next = current_scope->symbols;
    current_scope->symbols = ent;
    return ent;
}

SymbolEntry* st_lookup_local(const char* name) {
    if (!current_scope) return NULL;
    for (SymbolEntry* s = current_scope->symbols; s; s = s->next) {
        if (strcmp(s->name, name) == 0) return s;
    }
    return NULL;
}

SymbolEntry* st_lookup(const char* name) {
    Scope* s = current_scope;
    while (s) {
        for (SymbolEntry* e = s->symbols; e; e = e->next) if (strcmp(e->name, name) == 0) return e;
        s = s->parent;
    }
    return NULL;
}

// Search all scopes (for code generation phase when current_scope may not be set correctly)
SymbolEntry* st_lookup_global(const char* name) {
    Scope* s = all_scopes;
    while (s) {
        for (SymbolEntry* e = s->symbols; e; e = e->next) {
            if (strcmp(e->name, name) == 0) return e;
        }
        s = s->nextSibling;
    }
    return NULL;
}

static const char* kind_to_str(SymbolKind k) {
    switch (k) {
        case SYM_CLASS: return "class";
        case SYM_FUNCTION: return "function";
        case SYM_PARAM: return "param";
        case SYM_VARIABLE: return "var";
        case SYM_ATTRIBUTE: return "attr";
        default: return "unknown";
    }
}

static void type_to_str(const Type* t, char* out, size_t n) {
    if (!t) { snprintf(out, n, "-"); return; }
    switch (t->kind) {
        case TYPE_CLASS: snprintf(out, n, "class(%s)", t->name); break;
        case TYPE_ARRAY: snprintf(out, n, "array(dim=%d)", t->dimensions); break;
        case TYPE_INT: snprintf(out, n, "int"); break;
        case TYPE_FLOAT: snprintf(out, n, "float"); break;
        case TYPE_VOID: snprintf(out, n, "void"); break;
        case TYPE_UNKNOWN: default: snprintf(out, n, "unknown"); break;
    }
}

#ifndef ARCH_INT_SIZE
#define ARCH_INT_SIZE 4
#endif
#ifndef ARCH_FLOAT_SIZE
#define ARCH_FLOAT_SIZE 8
#endif
#ifndef ARCH_PTR_SIZE
#define ARCH_PTR_SIZE 8
#endif
#ifndef ARCH_ALIGN
#define ARCH_ALIGN 4
#endif

static size_t align_up_size(size_t v, size_t align) {
    if (align == 0) return v;
    return ((v + align - 1) / align) * align;
}

size_t compute_type_size(const Type* t) {
    if (!t) return 0;
    switch (t->kind) {
        case TYPE_INT: return ARCH_INT_SIZE;
        case TYPE_FLOAT: return ARCH_FLOAT_SIZE;
        case TYPE_VOID: return 0;
        case TYPE_ARRAY: {
            if (!t->elementType) return 0;
            size_t elem = compute_type_size(t->elementType);
            // dimension sizes are present, multiply them to get total element count
            if (t->dimSizes) {
                size_t prod = 1;
                for (int i = 0; i < t->dimensions; ++i) {
                    if (t->dimSizes[i] == 0) return 0; 
                    prod *= t->dimSizes[i];
                }
                return elem * prod;
            }
            return 0;
        }
        case TYPE_CLASS: {
            // Find a scope whose name matches the class name and sum its symbols that are attributes
            size_t total = 0;
            for (Scope* sc = all_scopes; sc; sc = sc->nextSibling) {
                if (strcmp(sc->name, t->name) == 0) {
                    for (SymbolEntry* e = sc->symbols; e; e = e->next) {
                        if (e->kind == SYM_ATTRIBUTE) {
                            size_t w = compute_type_size(&e->type);
                            total += align_up_size(w, ARCH_ALIGN);
                        }
                    }
                    break;
                }
            }
            return total;
        }
        case TYPE_UNKNOWN:
        default:
            return 0;
    }
}

// =================================== calculate symbol size =====================================================================
size_t compute_symbol_size(const Type* t, struct ASTNode* declNode) {
    if (!t) return 0;
    if (t->kind != TYPE_ARRAY) return compute_type_size(t);
    // find arraySizeList AST node under declNode
    if (!declNode) return 0;
    struct ASTNode* n = declNode;
    struct ASTNode* asl = NULL;
    typedef struct StackItem { struct ASTNode* node; struct StackItem* next; } StackItem;
    StackItem* stack = NULL;
    StackItem* s0 = (StackItem*)malloc(sizeof(StackItem)); s0->node = n; s0->next = stack; stack = s0;
    while (stack) {
        struct ASTNode* cur = stack->node;
        StackItem* next = stack->next; free(stack); stack = next;
        if (!cur) continue;
        if (cur->name && strcmp(cur->name, "arraySizeList") == 0) { asl = cur; break; }
        if (cur->name && strcmp(cur->name, "arraySize") == 0) { asl = cur; break; }
        if (cur->child) { StackItem* it = (StackItem*)malloc(sizeof(StackItem)); it->node = cur->child; it->next = stack; stack = it; }
        for (struct ASTNode* sib = cur->sibling; sib; sib = sib->sibling) { StackItem* it = (StackItem*)malloc(sizeof(StackItem)); it->node = sib; it->next = stack; stack = it; }
    }
    if (!asl) return 0;
    size_t total_elems = 1;
    int found_any = 0;
    // iterate over arraySize children if this is a list node
    if (strcmp(asl->name, "arraySizeList") == 0) {
        for (struct ASTNode* a = asl->child; a; a = a->sibling) {
            if (!a) continue;
            if (strcmp(a->name, "arraySize") != 0) continue;
            // arraySize -> [ intLit ] | [ ]
            if (a->child && a->child->name && strcmp(a->child->name, "intLit") == 0 && a->child->lexeme) {
                long v = atol(a->child->lexeme);
                if (v <= 0) return 0; 
                total_elems *= (size_t)v;
                found_any = 1;
            } else {
                return 0; 
            }
        }
    } else if (strcmp(asl->name, "arraySize") == 0) {
        struct ASTNode* a = asl;
        if (a->child && a->child->name && strcmp(a->child->name, "intLit") == 0 && a->child->lexeme) {
            long v = atol(a->child->lexeme);
            if (v <= 0) return 0;
            total_elems *= (size_t)v; found_any = 1;
        } else return 0;
    }
    if (!found_any) return 0;
    size_t elem = compute_type_size(t->elementType);
    return elem * total_elems;
}

static void compute_scope_label(Scope* sc, char* buf, size_t n) {
    if (!sc) { snprintf(buf, n, "<null>"); return; }
    if (sc->level == 0) { snprintf(buf, n, "global"); return; }
    const char* parent_name = sc->parent ? sc->parent->name : "<none>";
    // look for a parent symbol with same name
    if (sc->parent) {
        for (SymbolEntry* pe = sc->parent->symbols; pe; pe = pe->next) {
            if (strcmp(pe->name, sc->name) == 0) {
                if (pe->kind == SYM_CLASS) { snprintf(buf, n, "class %s", sc->name); return; }
                if (pe->kind == SYM_FUNCTION) { snprintf(buf, n, "function %s", sc->name); return; }
            }
        }
    }
    snprintf(buf, n, "local (in %s)", parent_name);
}


// ================================== print symbol tablev===============================================================
void st_print(FILE* out) {
    fprintf(out, "Symbol table:\n\n");
    for (Scope* sc = all_scopes; sc; sc = sc->nextSibling) {
        // skip scopes with no symbols
        if (!sc->symbols) continue;
        char scope_label[128]; compute_scope_label(sc, scope_label, sizeof(scope_label));
        fprintf(out, "Scope(level=%d, name=%s)\n", sc->level, sc->name);
    fprintf(out, "%-28s | %-12s | %-22s | %-30s | %-8s | %-8s | %-4s\n",
        "Name", "Kind", "Type", "Scope", "Width", "Offset", "Line");
    fprintf(out, "%s\n", "----------------------------+--------------+----------------------+-------------------------------------------+--------+--------+----");
    for (SymbolEntry* e = sc->symbols; e; e = e->next) {
        char tbuf[128]; type_to_str(&e->type, tbuf, sizeof(tbuf));
        char widthbuf[32]; char offsetbuf[32];
        if (e->width > 0) snprintf(widthbuf, sizeof(widthbuf), "%zu", e->width); else snprintf(widthbuf, sizeof(widthbuf), "-");
        if (e->offset != INT_MIN) snprintf(offsetbuf, sizeof(offsetbuf), "%d", e->offset); else snprintf(offsetbuf, sizeof(offsetbuf), "-");
        fprintf(out, "%-28s | %-12s | %-22s | %-30s | %-8s | %-8s | %-4d\n",
            e->name, kind_to_str(e->kind), tbuf, scope_label, widthbuf, offsetbuf, e->line);
    }
        fprintf(out, "\n");
    }
}

// ============================================== calculating offsets ============================================================
int st_compute_frame_layout(const char* functionName) {
    if (!functionName) return -1;
    // find scope with matching name
    Scope* target = NULL;
    for (Scope* sc = all_scopes; sc; sc = sc->nextSibling) {
        if (strcmp(sc->name, functionName) == 0) { target = sc; break; }
    }
    if (!target) return -1;

    SymbolEntry* e;
    SymbolEntry** params = NULL; size_t params_cap = 0; size_t params_n = 0;
    for (e = target->symbols; e; e = e->next) {
        if (e->kind == SYM_PARAM) {
            if (params_n + 1 > params_cap) { params_cap = params_cap ? params_cap*2 : 8; params = realloc(params, params_cap * sizeof(*params)); }
            params[params_n++] = e;
        }
    }
    for (size_t i = 0; i < params_n / 2; ++i) { SymbolEntry* tmp = params[i]; params[i] = params[params_n-1-i]; params[params_n-1-i] = tmp; }
    // assign param offsets
    int param_offset = 8;
    for (size_t i = 0; i < params_n; ++i) {
        SymbolEntry* p = params[i];
        if (p->width == 0) p->width = compute_type_size(&p->type);
        size_t w = align_up_size(p->width, ARCH_ALIGN);
        p->offset = param_offset;
        param_offset += (int)w;
    }
    free(params);

    // assign local variables and allocate negative offsets
    SymbolEntry** locals = NULL; size_t locals_cap = 0; size_t locals_n = 0;
    for (e = target->symbols; e; e = e->next) {
        if (e->kind == SYM_VARIABLE || e->kind == SYM_ATTRIBUTE) {
            if (locals_n + 1 > locals_cap) { locals_cap = locals_cap ? locals_cap*2 : 8; locals = realloc(locals, locals_cap * sizeof(*locals)); }
            locals[locals_n++] = e;
        }
    }

    // assign negative offsets
    int local_cursor = 0; // will grow negative
    for (size_t i = 0; i < locals_n; ++i) {
        SymbolEntry* l = locals[i];
        if (l->width == 0) l->width = compute_type_size(&l->type);
        size_t w = align_up_size(l->width, ARCH_ALIGN);
        local_cursor -= (int)w;
        l->offset = local_cursor;
    }
    free(locals);

    int total_frame = align_up_size((size_t)(-local_cursor), ARCH_ALIGN);
    return total_frame;
}

// void st_compute_all_frame_layouts() {
//     for (Scope* sc = all_scopes; sc; sc = sc->nextSibling) {
//         // if the parent contains a function symbol with same name, treat sc as the function scope
//         if (sc->parent) {
//             for (SymbolEntry* e = sc->parent->symbols; e; e = e->next) {
//                 if (strcmp(e->name, sc->name) == 0 && e->kind == SYM_FUNCTION) {
//                     st_compute_frame_layout(sc->name);
//                     break;
//                 }
//             }
//         }
//     }
// }

void st_write_file(const char* path) {
    FILE* f = fopen(path, "w");
    if (!f) return;
    st_print(f);
    fclose(f);
}

Type make_basic_type(TypeKind k) {
    Type t; t.kind = k; t.name[0] = '\0'; t.elementType = NULL; t.dimensions = 0; t.dimSizes = NULL; t.parent_name[0] = '\0'; return t;
}

Type make_class_type(const char* className) {
    Type t; t.kind = TYPE_CLASS; strncpy(t.name, className, sizeof(t.name)-1); t.name[sizeof(t.name)-1] = '\0'; t.elementType = NULL; t.dimensions = 0; t.dimSizes = NULL; t.parent_name[0] = '\0'; return t;
}

int type_equal(const Type* a, const Type* b) {
    if (!a || !b) return 0;
    if (a->kind != b->kind) return 0;
    if (a->kind == TYPE_CLASS) return strcmp(a->name, b->name) == 0;
    if (a->kind == TYPE_ARRAY) {
        if (b->kind != TYPE_ARRAY) return 0;
        return type_equal(a->elementType, b->elementType) && a->dimensions == b->dimensions;
    }
    return 1;
}


// =========================================== stack structure for function data ==========================================================
static int compute_layout_for_scope_ptr(Scope* target) {
    if (!target) return -1;

    // --- Parameters get + offests
    int param_offset = 8;
    int param_count = 0;
    for (SymbolEntry* e = target->symbols; e; e = e->next) if (e->kind == SYM_PARAM) param_count++;
    
    if (param_count > 0) {
        SymbolEntry** params = (SymbolEntry**)malloc(sizeof(SymbolEntry*) * param_count);
        int i = 0;
        for (SymbolEntry* e = target->symbols; e; e = e->next) if (e->kind == SYM_PARAM) params[i++] = e;
        
        for (int j = 0; j < param_count / 2; j++) {
            SymbolEntry* temp = params[j];
            params[j] = params[param_count - 1 - j];
            params[param_count - 1 - j] = temp;
        }
        
        for (int j = 0; j < param_count; j++) {
            SymbolEntry* p = params[j];
            if (p->width == 0) p->width = compute_type_size(&p->type);
            size_t w = align_up_size(p->width, ARCH_ALIGN);
            p->offset = param_offset;
            param_offset += (int)w;
        }
        free(params);
    }

    // --- local variables get - offsets ---
    int local_cursor = 0;
    int local_count = 0;
    for (SymbolEntry* e = target->symbols; e; e = e->next) if (e->kind == SYM_VARIABLE) local_count++;

    if (local_count > 0) {
        SymbolEntry** locals = (SymbolEntry**)malloc(sizeof(SymbolEntry*) * local_count);
        int i = 0;
        for (SymbolEntry* e = target->symbols; e; e = e->next) if (e->kind == SYM_VARIABLE) locals[i++] = e;
        
        for (int j = 0; j < local_count; j++) {
            SymbolEntry* l = locals[j];
            if (l->width == 0) l->width = compute_type_size(&l->type);
            size_t w = align_up_size(l->width, ARCH_ALIGN);
            local_cursor -= (int)w;
            l->offset = local_cursor;
        }
        free(locals);
    }
    return 0;
}

// ============================== store class data ===================================================================
static void compute_class_layout(Scope* target) {
    if (!target) return;
    
    int current_offset = 0;
    
    int attr_count = 0;
    for (SymbolEntry* e = target->symbols; e; e = e->next) if (e->kind == SYM_ATTRIBUTE) attr_count++;
    
    if (attr_count == 0) return;

    SymbolEntry** attrs = (SymbolEntry**)malloc(sizeof(SymbolEntry*) * attr_count);
    int i = 0;
    for (SymbolEntry* e = target->symbols; e; e = e->next) if (e->kind == SYM_ATTRIBUTE) attrs[i++] = e;

    for (int j = 0; j < attr_count / 2; j++) {
        SymbolEntry* temp = attrs[j];
        attrs[j] = attrs[attr_count - 1 - j];
        attrs[attr_count - 1 - j] = temp;
    }

    // Assign Positive Offsets starting at 0
    for (int j = 0; j < attr_count; j++) {
        SymbolEntry* a = attrs[j];
        if (a->width == 0) a->width = compute_type_size(&a->type);
        
        a->offset = current_offset;
        
        size_t w = align_up_size(a->width, ARCH_ALIGN);
        current_offset += (int)w;
    }
    free(attrs);
}

// ================ driver function ====================================================================
void st_compute_all_frame_layouts() {
    for (Scope* sc = all_scopes; sc; sc = sc->nextSibling) {
        if (sc->parent) {
            for (SymbolEntry* e = sc->parent->symbols; e; e = e->next) {
                if (strcmp(e->name, sc->name) == 0) {
                    
                    if (e->kind == SYM_FUNCTION) {
                        // if a function => calculate Stack Frame like (-4, -8...)
                        compute_layout_for_scope_ptr(sc);
                    }
                    else if (e->kind == SYM_CLASS) {
                        // if a class => alculate Object Layout like (0, 4, 8...)
                        compute_class_layout(sc);
                    }
                    break;
                }
            }
        }
    }
}