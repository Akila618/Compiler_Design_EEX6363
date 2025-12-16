#include "ast.h"

// create new AST node with given name and lexeme =========================
ASTNode* createNode(const char* name, const char* lexeme) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) return NULL;
    strncpy(node->name, name, sizeof(node->name)-1);
    node->name[sizeof(node->name)-1] = '\0';
    if (lexeme) {
        strncpy(node->lexeme, lexeme, sizeof(node->lexeme)-1);
        node->lexeme[sizeof(node->lexeme)-1] = '\0';
    } else {
        node->lexeme[0] = '\0';
    }
    node->type = NULL;
    node->line = 0;
    node->child = NULL;
    node->sibling = NULL;
    return node;
}

void addChild(ASTNode* parent, ASTNode* child) {
    if (parent == NULL || child == NULL) return;
    if (parent->child == NULL)
        parent->child = child;
    else {
        ASTNode* temp = parent->child;
        while (temp->sibling != NULL)
            temp = temp->sibling;
        temp->sibling = child;
    }
}

void addSibling(ASTNode* node, ASTNode* sibling) {
    if (node == NULL || sibling == NULL) return;
    while (node->sibling != NULL)
        node = node->sibling;
    node->sibling = sibling;
}

void printAST(ASTNode* root, int level, FILE* output_file) {
    if (root == NULL) return;

    for (int i = 0; i < level; i++) fprintf(output_file, "  ");
    fprintf(output_file, "%s", root->name);
    if (strlen(root->lexeme) > 0)
        fprintf(output_file, " (%s)", root->lexeme);
    fprintf(output_file, "\n");

    printAST(root->child, level + 1, output_file);
    printAST(root->sibling, level, output_file);
}

void freeAST(ASTNode* root) {
    if (root == NULL) return;
    freeAST(root->child);
    freeAST(root->sibling);
    free(root);
}

// printing the created datastructure recursively till end
static void printTreeRecursive(ASTNode* node, const char* prefix, int isLast, FILE* out) {
    if (!node) return;
    fprintf(out, "%s", prefix);
    if (isLast) fprintf(out, "`-- "); else fprintf(out, "|-- ");
    if (strlen(node->lexeme) > 0)
        fprintf(out, "%s -> %s\n", node->name, node->lexeme);
    else
        fprintf(out, "%s\n", node->name);
    char childPrefix[1024];
    if (isLast)
        snprintf(childPrefix, sizeof(childPrefix), "%s    ", prefix);
    else
        snprintf(childPrefix, sizeof(childPrefix), "%s|   ", prefix);
    ASTNode* child = node->child;
    int count = 0;
    for (ASTNode* c = child; c; c = c->sibling) count++;

    int idx = 0;
    for (ASTNode* c = child; c; c = c->sibling) {
        idx++;
        printTreeRecursive(c, childPrefix, (idx == count), out);
    }
}

void printFormattedAST(ASTNode* root, FILE* output_file) {
    if (!root || !output_file) return;
    fprintf(output_file, "%s", root->name);
    if (strlen(root->lexeme) > 0) fprintf(output_file, " -> %s", root->lexeme);
    fprintf(output_file, "\n");
    ASTNode* child = root->child;
    int count = 0;
    for (ASTNode* c = child; c; c = c->sibling) count++;

    int idx = 0;
    for (ASTNode* c = child; c; c = c->sibling) {
        idx++;
        printTreeRecursive(c, "", (idx == count), output_file);
    }
}

// decide whether a node is a wrapper that can be collapsed.
static int isWrapperNode(ASTNode* n) {
    if (!n) return 0;
    // Common wrapper names from the grammar that don't add semantic meaning
    const char* wrappers[] = {
        "classOrImplOrFuncList", "classOrImplOrFunc", "funcDefList",
        "varDeclOrStmtList", "varDeclOrStmt", "idnestList", "indiceList",
        "arraySizeList", "fParamsTailList", "aParamsTailList", "fParams",
        "fParamsTail", "aParams", "aParamsTail", "arithExprTail", "termTail",
        "statmentList", "visibilitymemberDeclList", "idnestTail",
        "idTail", "isaIdOpt", "returnType", "type"
    };
    int nwrap = sizeof(wrappers)/sizeof(wrappers[0]);
    for (int i = 0; i < nwrap; ++i) if (strcmp(n->name, wrappers[i])==0) return 1;
    return 0;
}

// if node is with a single child, skip.
static ASTNode* collapseNode(ASTNode* n) {
    while (n && isWrapperNode(n) && n->child && n->child->sibling == NULL) n = n->child;
    return n;
}
static void printSimplifiedRec(ASTNode* node, const char* prefix, int isLast, FILE* output_file);
void printSimplifiedAST(ASTNode* root, FILE* output_file) {
    if (!root || !output_file) return;
    fprintf(output_file, "%s\n", root->name);
    ASTNode* child = root->child;
    ASTNode* directChildren[128]; int dcount = 0;
    for (ASTNode* c = child; c; c = c->sibling) directChildren[dcount++] = c;
    for (int i = 0; i < dcount; ++i) printSimplifiedRec(directChildren[i], "", (i==dcount-1), output_file);
}

static void printSimplifiedRec(ASTNode* node, const char* prefix, int isLast, FILE* output_file) {
    if (!node) return;
    fprintf(output_file, "%s", prefix);
    fprintf(output_file, isLast ? "`-- " : "|-- ");
    if (strlen(node->lexeme) > 0) fprintf(output_file, "%s -> %s\n", node->name, node->lexeme);
    else fprintf(output_file, "%s\n", node->name);

    char childPrefix[1024];
    snprintf(childPrefix, sizeof(childPrefix), "%s%s", prefix, (isLast ? "    " : "|   "));

    ASTNode* visible[128]; int vcount = 0;
    for (ASTNode* c = node->child; c; c = c->sibling) visible[vcount++] = c;
    for (int i = 0; i < vcount; ++i) printSimplifiedRec(visible[i], childPrefix, (i==vcount-1), output_file);
}

// decide if a node should be consider as a terminal in the detailed AST
static int isTerminalNode(ASTNode* n) {
    if (!n) return 0;
    if (strlen(n->lexeme) > 0) return 1;
    const char* terminals[] = {"+","-","*","/",":=","<","<=",">",">=","<>","not","and","or","SELF","ID","intLit","floatLit","integer","float","void","PUBLIC","PRIVATE"};
    int tcount = sizeof(terminals)/sizeof(terminals[0]);
    for (int i = 0; i < tcount; ++i) if (strcmp(n->name, terminals[i])==0) return 1;
    return 0;
}

// collapse wrapper nodes but preserve a node if it is terminal or has multiple children
static ASTNode* collapseForDetailed(ASTNode* n) {
    if (!n) return NULL;
    if (isTerminalNode(n)) return n;
    if (!n->child) return n;
    
    ASTNode* child = n->child;
    int visible = 0;
    ASTNode* lastVis = NULL;
    for (ASTNode* c = child; c; c = c->sibling) {
        ASTNode* cv = collapseForDetailed(c);
        if (cv) { visible++; lastVis = cv; }
        if (visible > 1) break;
    }
    if (visible == 1) return lastVis;
    return n;
}

// terminals are leaves.
static void printDetailedRec(ASTNode* node, const char* prefix, int isLast, FILE* out) {
    if (!node) return;
    
    fprintf(out, "%s", prefix);
    fprintf(out, isLast ? "`-- " : "|-- ");
    if (strlen(node->lexeme) > 0) fprintf(out, "%s -> %s\n", node->name, node->lexeme);
    else fprintf(out, "%s\n", node->name);

    char childPrefix[1024];
    snprintf(childPrefix, sizeof(childPrefix), "%s%s", prefix, (isLast ? "    " : "|   "));

    ASTNode* children[256]; int ccount = 0;
    for (ASTNode* c = node->child; c; c = c->sibling) children[ccount++] = c;
    for (int i = 0; i < ccount; ++i) printDetailedRec(children[i], childPrefix, (i == ccount-1), out);
}

void printDetailedAST(ASTNode* root, FILE* output_file) {
    if (!root || !output_file) return;
    fprintf(output_file, "%s\n", root->name);
    
    ASTNode* children[256]; int ccount = 0;
    for (ASTNode* c = root->child; c; c = c->sibling) children[ccount++] = c;
    for (int i = 0; i < ccount; ++i) printDetailedRec(children[i], "", (i==ccount-1), output_file);
}
