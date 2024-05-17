#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

// Function prototypes for recursive descent parser
ASTNode* parse_primary(Token** tokens);
ASTNode* parse_term(Token** tokens);
ASTNode* parse_expression(Token** tokens);

// Helper function to create a number node
ASTNode* create_number_node(int value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }
    node->type = AST_NUMBER;
    node->data.number = value;
    return node;
}

// Helper function to create a binary operation node
ASTNode* create_binary_op_node(ASTNode* left, TokenType op, ASTNode* right) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }
    node->type = AST_BINARY_OP;
    node->data.binary_op.left = left;
    node->data.binary_op.op = op;
    node->data.binary_op.right = right;
    return node;
}

// Parse primary expressions (numbers and parenthesized expressions)
ASTNode* parse_primary(Token** tokens) {
    Token* token = *tokens;
    if (token->type == NUMBER) {
        int value = atoi(token->value);
        *tokens += 1; // Advance to next token
        return create_number_node(value);
    } else if (token->type == LPAREN) {
        *tokens += 1; // Consume '('
        ASTNode* expr = parse_expression(tokens);
        if ((*tokens)->type == RPAREN) {
            *tokens += 1; // Consume ')'
            return expr;
        } else {
            fprintf(stderr, "Error: Expected ')'\n");
            free_ast(expr);
            return NULL;
        }
    } else {
        fprintf(stderr, "Error: Unexpected token '%s'\n", token->value);
        return NULL;
    }
}

// Parse multiplication and division
ASTNode* parse_term(Token** tokens) {
    ASTNode* node = parse_primary(tokens);
    while ((*tokens)->type == MULTIPLY || (*tokens)->type == DIVIDE) {
        TokenType op = (*tokens)->type;
        *tokens += 1; // Consume operator
        ASTNode* right = parse_primary(tokens);
        node = create_binary_op_node(node, op, right);
    }
    return node;
}

// Parse addition and subtraction
ASTNode* parse_expression(Token** tokens) {
    ASTNode* node = parse_term(tokens);
    while ((*tokens)->type == PLUS || (*tokens)->type == MINUS) {
        TokenType op = (*tokens)->type;
        *tokens += 1; // Consume operator
        ASTNode* right = parse_term(tokens);
        node = create_binary_op_node(node, op, right);
    }
    return node;
}

// Free memory allocated for an AST
void free_ast(ASTNode* node) {
    if (node == NULL) {
        return;
    }
    if (node->type == AST_BINARY_OP) {
        free_ast(node->data.binary_op.left);
        free_ast(node->data.binary_op.right);
    }
    free(node);
}
