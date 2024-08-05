#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "../parser/parser.h"
#include "../includes/readfile.h"
#include "../tokenizer/tokenizer.h"

// #include "class.h"

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_LIST,
    TYPE_DICT,
    TYPE_FILE,
} LiteralType;
// Define the Variable struct and use typedef for convenience

// Declare the head of the linked list
typedef struct Literal
{
    void *value;
    LiteralType type;
    struct{
        int count;
        struct Literal** elements;
    } List;
    struct MapEntry {
        struct Literal* key;
        struct Literal* value;
    } MapEntry;

    struct {
        struct Literal* entries;
        int count;
        int capacity;
    } Map;

}Literal;

typedef struct Variable {
    char *name;
    Literal *value;
    struct Variable *next;  // Use 'struct Variable' here
} Variable;

typedef struct Function {
    char* name;
    ASTNode* body;
    char** parameters;
    int param_count;
    struct Function* next;
} Function;
typedef struct {
    Literal* value;
    int is_return;
} ReturnValue;

// Global class registry



// Function to interpret an AST and return the result
Literal* interpret(ASTNode* node);

#endif /* INTERPRETER_H */