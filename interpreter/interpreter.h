#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "../parser/parser.h"
#include <stdlib.h>

typedef struct {
    char* name;
    void* value;
    struct Variable* next;
} Variable;

extern Variable* variables;

// Function to evaluate an AST node
void* evaluate_ast(ASTNode* root);

// Function to set a variable's value
void set_variable(const char* name, void* value);

// Function to get a variable's value
void* get_variable(const char* name);

#endif /* INTERPRETER_H */
