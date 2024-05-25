#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

// Function prototypes for recursive descent parser
ASTNode* parse_primary(Token** tokens);
ASTNode* parse_term(Token** tokens);
ASTNode* parse_expression(Token** tokens);
ASTNode* parse_comparison(Token** tokens);

// Helper function to create a number node
ASTNode* create_number_node(double value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }
    node->type = AST_NUMBER;
    node->data.number = value;
    return node;
}
// Helper function to create identifier node
ASTNode* create_identifier_node(char* name) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }
    node->type = AST_IDENTIFIER;
    node->data.identifier = name;
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
ASTNode* create_logical_op_node(ASTNode* left, TokenType op, ASTNode* right) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_LOGICAL_OP;
    node->data.logical_op.left = left;
    node->data.logical_op.op = op;
    node->data.logical_op.right = right;
    return node;
}
// Helper function to create a assignment node
ASTNode* create_assignment_node(ASTNode* left, TokenType op, ASTNode* right) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }
    node->type = AST_ASSIGNMENT;
    node->data.assignment_op.left = left;
    node->data.assignment_op.op = op;
    node->data.assignment_op.right = right;
    return node;
}
// Helper function to create a unary operation node
ASTNode* create_unary_op_node(TokenType op, ASTNode* operand) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }
    node->type = AST_UNARY_OP;
    node->data.unary_op.op = op;
    node->data.unary_op.operand = operand;
    return node;
}

// Helper function to create a comparison node
ASTNode* create_comparison_node(ASTNode* left, TokenType op, ASTNode* right) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }
    node->type = AST_COMPARISON;
    node->data.comparison.left = left;
    node->data.comparison.op = op;
    node->data.comparison.right = right;
    return node;
}

// Parse primary expressions (numbers, parentheses, unary operators)
ASTNode* parse_primary(Token** tokens) {
    Token* token = *tokens;

    if (token->type == NUMBER) {
        double value = strtod(token->value, NULL);
        *tokens += 1; // Consume the number
        return create_number_node(value);
    } else if (token->type == LPAREN) {
        *tokens += 1; // Consume '('
        ASTNode* node = parse_expression(tokens);
        if ((*tokens)->type != RPAREN) {
            fprintf(stderr, "Error: Expected ')'\n");
            exit(1);
        }
        *tokens += 1; // Consume ')'
        return node;
    } else if (token->type == PLUS || token->type == MINUS || token->type == INCREMENT || token->type == DECREMENT || token->type == LOGICAL_NOT || token->type == BITWISE_NOT) {
        TokenType op = token->type;
        *tokens += 1; // Consume the unary operator
        return create_unary_op_node(op, parse_primary(tokens));
    }
    else if (token->type == IDENTIFIER) {
        *tokens += 1; // Consume the identifier
        return create_identifier_node(token->value);
    }
    
   

    fprintf(stderr, "Error: Unexpected token '%s'\n", token->value);
    exit(1);
}

// Parse exponentiation (right-associative)
ASTNode* parse_exponentiation(Token** tokens) {
    ASTNode* node = parse_primary(tokens);
    while ((*tokens)->type == EXPONENTIATION) {
        TokenType op = (*tokens)->type;
        *tokens += 1; // Consume '**'
        node = create_binary_op_node(node, op, parse_primary(tokens));
    }
    return node;
}

// Parse terms (multiplication, division, modulus, floor division)
ASTNode* parse_term(Token** tokens) {
    ASTNode* node = parse_exponentiation(tokens);
    while ((*tokens)->type == MULTIPLY || (*tokens)->type == DIVIDE || (*tokens)->type == MODULUS || (*tokens)->type == FLOOR_DIVISION) {
        TokenType op = (*tokens)->type;
        *tokens += 1; // Consume operator
        node = create_binary_op_node(node, op, parse_exponentiation(tokens));
    }
    return node;
}

// Parse comparisons
ASTNode* parse_comparison(Token** tokens) {
    ASTNode* node = parse_term(tokens);
    while ((*tokens)->type == EQUAL || (*tokens)->type == NOT_EQUAL || (*tokens)->type == GREATER || (*tokens)->type == LESS ||
           (*tokens)->type == GREATER_EQUAL || (*tokens)->type == LESS_EQUAL) {
        TokenType op = (*tokens)->type;
        *tokens += 1; // Consume operator
        node = create_comparison_node(node, op, parse_term(tokens));
    }
    return node;
}


// Function to parse a logical expression
ASTNode* parse_logical(Token** tokens) {
    ASTNode* left = parse_comparison(tokens);

    while ((*tokens)->type == LOGICAL_AND || (*tokens)->type == LOGICAL_OR || (*tokens)->type == BITWISE_AND || (*tokens)->type == BITWISE_OR || (*tokens)->type == BITWISE_XOR || (*tokens)->type == SHIFT_LEFT || (*tokens)->type == SHIFT_RIGHT) {
        Token* token = *tokens;
        (*tokens)++;
        ASTNode* right = parse_comparison(tokens);
        left = create_logical_op_node(left, token->type, right);
    }

    return left;
}

//create_assignment_node(left, op, right)
ASTNode* parse_assignment(Token** tokens) {
    ASTNode* left = parse_logical(tokens);
   
    while ((*tokens)->type == ASSIGN)
    {
        Token* token = *tokens;
        (*tokens)++;
        ASTNode* right = parse_expression(tokens);
        left = create_assignment_node(left, token->type, right);
        
    }
    return left;
    
}


// Parse expressions (addition and subtraction)
ASTNode* parse_expression(Token** tokens) {
    ASTNode* node = parse_assignment(tokens);
    while ((*tokens)->type == PLUS || (*tokens)->type == MINUS) {
        TokenType op = (*tokens)->type;
        *tokens += 1; // Consume operator
        node = create_binary_op_node(node, op, parse_comparison(tokens));
    }
    return node;
}

// Function to free memory allocated for an AST
void free_ast(ASTNode* node) {
    if (node == NULL) {
        return;
    }
    if (node->type == AST_BINARY_OP) {
        free_ast(node->data.binary_op.left);
        free_ast(node->data.binary_op.right);
    } else if (node->type == AST_UNARY_OP) {
        free_ast(node->data.unary_op.operand);
    } else if (node->type == AST_COMPARISON) {
        free_ast(node->data.comparison.left);
        free_ast(node->data.comparison.right);
    }
    free(node);
}
