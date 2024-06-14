#ifndef PARSER_H
#define PARSER_H

#include "..\tokenizer\tokenizer.h"



// AST node types
typedef enum {
    AST_NUMBER,
    AST_FLOAT,
    AST_STRING,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_COMPARISON,
    AST_LOGICAL_OP,
    AST_IDENTIFIER,
    AST_ASSIGNMENT,
    AST_STATEMENT_LIST,
    AST_FUNCTION_DEF,
} ASTNodeType;

// AST node structure
typedef struct ASTNode {
    ASTNodeType type;
    union {
        int number;
        double float_number;
        char* string;
        struct {
            struct ASTNode* left;
            TokenType op;
            struct ASTNode* right;
        } binary_op;
        struct {
            TokenType op;
            struct ASTNode* operand;
        } unary_op;
        
        //identifier
        char* identifier;
        // assignment, variable, function call, etc.
        
        struct {
            struct ASTNode* left;
            TokenType op;
            struct ASTNode* right;
        } assignment_op;
        
        struct {
            struct ASTNode** statements;
            int statement_count;
            
        } statement_list;
     
    } data;
} ASTNode;

// Function to parse an expression and create an AST
ASTNode* parse_expression(Token** tokens);

// Function to free memory allocated for an AST
void free_ast(ASTNode* node);

#endif /* PARSER_H */