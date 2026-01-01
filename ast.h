#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// forward-declare the Type struct (defined in symbol_table.h)
typedef struct Type Type;

// AST struct data structure ==========================================
typedef struct ASTNode {
    char name[64];                
    char lexeme[64];              
    Type* type;                
    int line;             
    struct ASTNode* child;        
    struct ASTNode* sibling;      
} ASTNode;

// Functions for managing AST =============================================
ASTNode* createNode(const char* name, const char* lexeme);
void addChild(ASTNode* parent, ASTNode* child);
void addSibling(ASTNode* node, ASTNode* sibling);
void printAST(ASTNode* root, int level, FILE* output_file);
void printFormattedAST(ASTNode* root, FILE* output_file);
void printDetailedAST(ASTNode* root, FILE* output_file);
void freeAST(ASTNode* root);


#endif
