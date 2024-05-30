#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "..\parser\parser.h"

// Define the Variable struct and use typedef for convenience
typedef struct Variable {
    char *name;
    void *value;
    struct Variable *next;  // Use 'struct Variable' here
} Variable;

// Declare the head of the linked list


// Function to interpret an AST and return the result
void* interpret(ASTNode* node);

#endif /* INTERPRETER_H */