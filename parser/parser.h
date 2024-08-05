#ifndef PARSER_H
#define PARSER_H

#include "..\tokenizer\tokenizer.h"
// #include ".\includes\headfiles\class.h"


// AST node types
typedef enum {
    AST_FLOAT,
    AST_NUMBER,
    AST_STRING,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_LIST,
    AST_COMPARISON,
    AST_LOGICAL_OP,
    AST_IDENTIFIER,
    AST_ASSIGNMENT,
    AST_LIST_INDEX,
    AST_LIST_UPDATE,
    AST_LIST_DELETE,
    AST_STATEMENT_LIST,
    AST_FUNCTION_DEF,
    AST_FUNCTION_CALL,
    AST_IF_STATEMENT,
    AST_PRINT,
    AST_RETURN,
    AST_WHILE_LOOP,
    AST_FOR_LOOP,
    AST_DICTIONARY,
    AST_CLASS_DEF,
    AST_CLASS_INSTANCE,
    AST_METHOD_DEF,
    AST_METHOD_CALL,
    AST_READ_FILE,
    AST_WRITE_FILE,
    AST_OPEN_FILE,
    AST_CLOSE_FILE,
    AST_SCAN,
    AST_IMPORT,

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
            char* name;
            int param_count;
            char** parameters;
            struct ASTNode* body;
        } function_def;
        struct {
            char* function_name;
            int arg_count;
            struct ASTNode** arguments;
        } function_call;
        
        struct {
            struct ASTNode** statements;
            int statement_count;
            
        } statement_list;
        struct {
            struct ASTNode* condition;
            struct ASTNode* then_branch;
            struct ASTNode* else_branch; // Can be NULL
        } if_statement;

         struct ASTNode* return_value;
         struct {
            int count;
            struct ASTNode** elements;
        } list;
        struct {
            struct ASTNode* list;
            struct ASTNode* index;
        } ASTListIndex;

        struct {
            struct ASTNode* condition;
            struct ASTNode* body;
        } ASTWhileLoop;

        struct {
            struct ASTNode* initializer;
            struct ASTNode* condition;
            struct ASTNode* increment;
            struct ASTNode* body;
        } ASTForLoop;
        struct {
            struct ASTNode** keys;
            struct ASTNode** values;
            int count;
        } ASTDictionary;
        struct {
            
            struct ASTNode* target;
            struct ASTNode* value;
        } ASTListUpdate;
        struct {
            struct ASTNode* target;
            
        } ASTListDelete;

        struct {
            char* class_name;
            struct ASTNode** members;
            size_t member_count;
        } class_def;
        struct {
            char* class_name;
            struct ASTNode** args;
            size_t arg_count;
        } class_instance;
        struct {
            char* method_name;
            struct ASTNode** params;
            size_t param_count;
            struct ASTNode* body;
        } method_def;
        struct {
            struct ASTNode* instance;
            char* method_name;
            struct ASTNode** args;
            size_t arg_count;
        } method_call;
        
    } data;
} ASTNode;

// Function to parse an expression and create an AST
ASTNode* parse_expression(Token** tokens);

// Function to free memory allocated for an AST
void free_ast(ASTNode* node);

#endif /* PARSER_H */