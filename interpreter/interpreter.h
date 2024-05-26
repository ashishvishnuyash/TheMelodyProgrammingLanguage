#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "..\parser\parser.h"

typedef struct {
    char* name;
    double* value;
    struct Variable* next;
} Variable;

extern Variable* variables;

// Function to interpret an AST and return the result
double interpret(ASTNode* node);

#endif /* INTERPRETER_H */
