#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

// run_semantic returns 1 if semantic analysis succeeded with no errors,
// or 0 if semantic errors were found (check files/semantic_errors.txt).
int run_semantic(ASTNode* root);

#endif
