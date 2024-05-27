#ifndef PARSER_H
#define PARSER_H

#include "..\tokenizer\tokenizer.h"



// AST node types
typedef enum {
    AST_NUMBER,
    AST_FLOAT,
    AST_COMPLEX,
    AST_STRING,
    AST_BOOLEAN,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_COMPARISON,
    AST_LOGICAL_OP,
    AST_IDENTIFIER,
    AST_ASSIGNMENT,
} ASTNodeType;


// AST node structure
typedef struct ASTNode {
    ASTNodeType type;
    union {
        int int_value;
        double float_value;
        struct { double real; double imag; } complex_value;
        char* string_value;
        int bool_value;
        struct ASTNode** list_value;
        struct ASTNode** tuple_value;
        struct { struct ASTNode** keys; struct ASTNode** values; } dict_value;
        struct ASTNode** set_value;
        double number;
        struct {
            struct ASTNode* left;
            TokenType op;
            struct ASTNode* right;
        } binary_op;
        struct {
            TokenType op;
            struct ASTNode* operand;
        } unary_op;
        struct {
            struct ASTNode* left;
            TokenType op;
            struct ASTNode* right;
        } comparison;
        struct {
            struct ASTNode* left;
            TokenType op;
            struct ASTNode* right;
        } logical_op;
        //identifier
        char* identifier;
        // assignment, variable, function call, etc.
        
        struct {
            struct ASTNode* left;
            TokenType op;
            struct ASTNode* right;
        } assignment_op;
     
    } data;
} ASTNode;

// Function to parse an expression and create an AST
ASTNode* parse(const char* input);

// Function to free memory allocated for an AST
void free_ast(ASTNode* node);

#endif /* PARSER_H */
