#include "parser.h"
#include "../tokenizer/tokenizer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static Token* current_token;
static Token* tokens;

static void advance() {
    current_token++;
}

static ASTNode* parse_expression();

static ASTNode* parse_primary() {
    ASTNode* node = NULL;

    switch (current_token->type) {
        case NUMBER:
            node = (ASTNode*)malloc(sizeof(ASTNode));
            node->type = AST_NUMBER;
            node->data.number = atoi(current_token->value);
            advance();
            break;
        case FLOAT:
            node = (ASTNode*)malloc(sizeof(ASTNode));
            node->type = AST_FLOAT;
            node->data.float_value = atof(current_token->value);
            advance();
            break;
        case STRING:
            node = (ASTNode*)malloc(sizeof(ASTNode));
            node->type = AST_STRING;
            node->data.string_value = strdup(current_token->value);
            advance();
            break;
        case BOOLEAN:
            node = (ASTNode*)malloc(sizeof(ASTNode));
            node->type = AST_BOOLEAN;
            node->data.bool_value = (strcmp(current_token->value, "true") == 0) ? 1 : 0;
            advance();
            break;
        case IDENTIFIER:
            node = (ASTNode*)malloc(sizeof(ASTNode));
            node->type = AST_IDENTIFIER;
            node->data.identifier = strdup(current_token->value);
            advance();
            break;
        case LPAREN:
            advance();
            node = parse_expression();
            if (current_token->type != RPAREN) {
                fprintf(stderr, "Error: Expected ')'\n");
                exit(EXIT_FAILURE);
            }
            advance();
            break;
        default:
            fprintf(stderr, "Error: Unexpected token '%s'\n", current_token->value);
            exit(EXIT_FAILURE);
    }

    return node;
}

static ASTNode* parse_unary() {
    if (current_token->type == PLUS || current_token->type == MINUS ||
        current_token->type == INCREMENT || current_token->type == DECREMENT ||
        current_token->type == LOGICAL_NOT) {
        ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
        node->type = AST_UNARY_OP;
        node->data.unary_op.op = (TokenType)current_token->type;
        advance();
        node->data.unary_op.operand = parse_unary();
        return node;
    }
    return parse_primary();
}

static ASTNode* parse_factor() {
    ASTNode* node = parse_unary();

    while (current_token->type == MULTIPLY || current_token->type == DIVIDE ||
           current_token->type == MODULUS || current_token->type == EXPONENTIATION ||
           current_token->type == FLOOR_DIVISION) {
        ASTNode* new_node = (ASTNode*)malloc(sizeof(ASTNode));
        new_node->type = AST_BINARY_OP;
        new_node->data.binary_op.op = (TokenType)current_token->type;
        new_node->data.binary_op.left = node;
        advance();
        new_node->data.binary_op.right = parse_unary();
        node = new_node;
    }

    return node;
}

static ASTNode* parse_term() {
    ASTNode* node = parse_factor();

    while (current_token->type == PLUS || current_token->type == MINUS) {
        ASTNode* new_node = (ASTNode*)malloc(sizeof(ASTNode));
        new_node->type = AST_BINARY_OP;
        new_node->data.binary_op.op = (TokenType)current_token->type;
        new_node->data.binary_op.left = node;
        advance();
        new_node->data.binary_op.right = parse_factor();
        node = new_node;
    }

    return node;
}

static ASTNode* parse_comparison() {
    ASTNode* node = parse_term();

    while (current_token->type == EQUAL || current_token->type == NOT_EQUAL ||
           current_token->type == GREATER || current_token->type == LESS ||
           current_token->type == GREATER_EQUAL || current_token->type == LESS_EQUAL) {
        ASTNode* new_node = (ASTNode*)malloc(sizeof(ASTNode));
        new_node->type = AST_COMPARISON;
        new_node->data.comparison.op = (TokenType)current_token->type;
        new_node->data.comparison.left = node;
        advance();
        new_node->data.comparison.right = parse_term();
        node = new_node;
    }

    return node;
}

static ASTNode* parse_logical() {
    ASTNode* node = parse_comparison();

    while (current_token->type == LOGICAL_AND || current_token->type == LOGICAL_OR) {
        ASTNode* new_node = (ASTNode*)malloc(sizeof(ASTNode));
        new_node->type = AST_LOGICAL_OP;
        new_node->data.logical_op.op = (TokenType)current_token->type;
        new_node->data.logical_op.left = node;
        advance();
        new_node->data.logical_op.right = parse_comparison();
        node = new_node;
    }

    return node;
}

static ASTNode* parse_expression() {
    return parse_logical();
}

ASTNode* parse(const char* input) {
    tokens = tokenize(input);
    current_token = tokens;
    ASTNode* root = parse_expression();

    if (current_token->type != TOKEN_EOF) {
        fprintf(stderr, "Error: Unexpected token '%s'\n", current_token->value);
        exit(EXIT_FAILURE);
    }

    return root;
}

void free_ast(ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case AST_NUMBER:
        case AST_FLOAT:
        case AST_STRING:
        case AST_BOOLEAN:
            free(node);
            break;
        case AST_BINARY_OP:
            free_ast(node->data.binary_op.left);
            free_ast(node->data.binary_op.right);
            free(node);
            break;
        case AST_UNARY_OP:
            free_ast(node->data.unary_op.operand);
            free(node);
            break;
        case AST_IDENTIFIER:
            free(node->data.identifier);
            free(node);
            break;
        case AST_ASSIGNMENT:
            free(node->data.assignment_op.left);
            free_ast(node->data.assignment_op.right);
            free(node);
            break;
        case AST_COMPARISON:
            free_ast(node->data.comparison.left);
            free_ast(node->data.comparison.right);
            free(node);
            break;
        case AST_LOGICAL_OP:
            free_ast(node->data.logical_op.left);
            free_ast(node->data.logical_op.right);
            free(node);
            break;
        default:
            fprintf(stderr, "Error: Unexpected AST node type %d\n", node->type);
            exit(EXIT_FAILURE);
    }
}
