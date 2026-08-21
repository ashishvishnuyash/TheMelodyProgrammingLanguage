#ifndef MELODY_PARSER_H
#define MELODY_PARSER_H

#include "lexer.h"

typedef enum {
    NODE_INT_LIT,
    NODE_FLOAT_LIT,
    NODE_STRING_LIT,
    NODE_BOOL_LIT,
    NODE_NULL_LIT,
    NODE_IDENTIFIER,
    NODE_BINARY_OP,
    NODE_UNARY_OP,
    NODE_ASSIGN,
    NODE_COMPOUND_ASSIGN,
    NODE_LIST_LIT,
    NODE_DICT_LIT,
    NODE_INDEX,
    NODE_INDEX_ASSIGN,
    NODE_FUNC_DEF,
    NODE_FUNC_CALL,
    NODE_METHOD_CALL,
    NODE_RETURN,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_FOR_IN,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_BLOCK,
    NODE_PROGRAM,
    NODE_IMPORT,
    NODE_DEL,
    NODE_TRY_CATCH,
    NODE_THROW,
    NODE_CLASS_DEF,
} NodeType;

typedef struct ASTNode {
    NodeType type;
    int line;
    union {
        long long int_val;              /* NODE_INT_LIT */
        double float_val;               /* NODE_FLOAT_LIT */
        char* string_val;               /* NODE_STRING_LIT */
        int bool_val;                   /* NODE_BOOL_LIT */
        char* identifier;               /* NODE_IDENTIFIER */

        struct {                        /* NODE_BINARY_OP */
            TokenType op;
            struct ASTNode* left;
            struct ASTNode* right;
        } binary;

        struct {                        /* NODE_UNARY_OP */
            TokenType op;
            struct ASTNode* operand;
        } unary;

        struct {                        /* NODE_ASSIGN */
            struct ASTNode* target;
            struct ASTNode* value;
        } assign;

        struct {                        /* NODE_COMPOUND_ASSIGN */
            TokenType op;               /* TOK_PLUS_EQ etc */
            struct ASTNode* target;
            struct ASTNode* value;
        } compound;

        struct {                        /* NODE_LIST_LIT */
            struct ASTNode** items;
            int count;
        } list;

        struct {                        /* NODE_DICT_LIT */
            struct ASTNode** keys;
            struct ASTNode** values;
            int count;
        } dict;

        struct {                        /* NODE_INDEX */
            struct ASTNode* object;
            struct ASTNode* index;
        } index;

        struct {                        /* NODE_INDEX_ASSIGN */
            struct ASTNode* object;
            struct ASTNode* idx;
            struct ASTNode* value;
        } index_assign;

        struct {                        /* NODE_FUNC_DEF */
            char* name;
            char** params;
            int param_count;
            struct ASTNode* body;
        } func_def;

        struct {                        /* NODE_FUNC_CALL */
            struct ASTNode* callee;
            struct ASTNode** args;
            int arg_count;
        } call;

        struct {                        /* NODE_METHOD_CALL */
            struct ASTNode* object;
            char* method;
            struct ASTNode** args;
            int arg_count;
        } method;

        struct ASTNode* return_val;     /* NODE_RETURN */

        struct {                        /* NODE_IF */
            struct ASTNode* condition;
            struct ASTNode* then_block;
            struct ASTNode* else_block;
        } if_stmt;

        struct {                        /* NODE_WHILE */
            struct ASTNode* condition;
            struct ASTNode* body;
        } while_loop;

        struct {                        /* NODE_FOR */
            struct ASTNode* init;
            struct ASTNode* condition;
            struct ASTNode* increment;
            struct ASTNode* body;
        } for_loop;

        struct {                        /* NODE_FOR_IN */
            char* var_name;
            struct ASTNode* iterable;
            struct ASTNode* body;
        } for_in;

        struct {                        /* NODE_BLOCK / NODE_PROGRAM */
            struct ASTNode** stmts;
            int count;
        } block;

        struct ASTNode* import_path;    /* NODE_IMPORT */
        struct ASTNode* del_target;     /* NODE_DEL */

        struct {                        /* NODE_TRY_CATCH */
            struct ASTNode* try_block;
            char* catch_var;
            struct ASTNode* catch_block;
        } try_catch;

        struct ASTNode* throw_val;      /* NODE_THROW */

        struct {                        /* NODE_CLASS_DEF */
            char* name;
            char* superclass;           /* NULL if no extends */
            char** method_names;
            struct ASTNode** method_bodies;
            int method_count;
        } class_def;
    } data;
} ASTNode;

typedef struct {
    Token* tokens;
    int count;
    int pos;
    int had_error;
} Parser;

/* Main entry point */
ASTNode* parse(Token* tokens, int token_count);

/* Cleanup */
void ast_free(ASTNode* node);

#endif
