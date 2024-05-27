#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

Variable* variables = NULL;
void set_variable(const char* name, void* value);

void* get_variable(const char* name);

static void* evaluate_literal(ASTNode* node) {
    void* value = NULL;
    switch (node->type) {
        case AST_NUMBER:
            value = malloc(sizeof(int));
            memcpy(value, &node->data.number, sizeof(int));
            break;
        case AST_FLOAT:
            value = malloc(sizeof(double));
            memcpy(value, &node->data.float_value, sizeof(double));
            break;
        case AST_COMPLEX:
            value = malloc(2 * sizeof(double));
            ((double*)value)[0] = node->data.complex_value.real;
            ((double*)value)[1] = node->data.complex_value.imag;
            break;
        case AST_STRING:
            value = strdup(node->data.string_value);
            break;
        case AST_BOOLEAN:
            value = malloc(sizeof(int));
            memcpy(value, &node->data.bool_value, sizeof(int));
            break;
        default:
            fprintf(stderr, "Error: Unexpected literal type %d\n", node->type);
            exit(EXIT_FAILURE);
    }
    return value;
}

static void* evaluate_unary_op(ASTNode* node) {
    void* operand_value = evaluate_ast(node->data.unary_op.operand);
    void* result = NULL;

    switch (node->data.unary_op.op) {
        case PLUS:
            result = malloc(sizeof(int));
            *((int*)result) = +(*((int*)operand_value));
            break;
        case MINUS:
            result = malloc(sizeof(int));
            *((int*)result) = -(*((int*)operand_value));
            break;
        case INCREMENT:
            result = malloc(sizeof(int));
            *((int*)result) = (*((int*)operand_value)) + 1;
            break;
        case DECREMENT:
            result = malloc(sizeof(int));
            *((int*)result) = (*((int*)operand_value)) - 1;
            break;
        case LOGICAL_NOT:
            result = malloc(sizeof(int));
            *((int*)result) = !(*((int*)operand_value));
            break;
        default:
            fprintf(stderr, "Error: Unsupported unary operator %d\n", node->data.unary_op.op);
            exit(EXIT_FAILURE);

    }
    return result;
}

static void* evaluate_binary_op(ASTNode* node) {
    void* left = evaluate_ast(node->data.binary_op.left);
    void* right = evaluate_ast(node->data.binary_op.right);
    void* result = NULL;

    switch (node->data.binary_op.op) {
        case PLUS:
            if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER) {
                result = malloc(sizeof(int));
                *((int*)result) = *((int*)left) + *((int*)right);
            } else if (node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_FLOAT) {
                result = malloc(sizeof(double));
                *((double*)result) = *((double*)left) + *((double*)right);
            }
            break;
        case MINUS:
            if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER) {
                result = malloc(sizeof(int));
                *((int*)result) = *((int*)left) - *((int*)right);
            } else if (node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_FLOAT) {
                result = malloc(sizeof(double));
                *((double*)result) = *((double*)left) - *((double*)right);
            }
            break;
        case MULTIPLY:
            if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER) {
                result = malloc(sizeof(int));
                *((int*)result) = *((int*)left) * *((int*)right);
            } else if (node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_FLOAT) {
                result = malloc(sizeof(double));
                *((double*)result) = *((double*)left) * *((double*)right);
            }
            break;
        case DIVIDE:
            if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER) {
                result = malloc(sizeof(int));
                *((int*)result) = *((int*)left) / *((int*)right);
            } else if (node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_FLOAT) {
                result = malloc(sizeof(double));
                *((double*)result) = *((double*)left) / *((double*)right);
            }
            break;
        case MODULUS:
            if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER) {
                result = malloc(sizeof(int));
                *((int*)result) = *((int*)left) % *((int*)right);
            }
            break;
        case EXPONENTIATION:
            if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER) {
                result = malloc(sizeof(int));
                *((int*)result) = (int)pow(*((int*)left), *((int*)right));
            } else if (node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_FLOAT) {
                result = malloc(sizeof(double));
                *((double*)result) = pow(*((double*)left), *((double*)right));
            }
            break;
        case FLOOR_DIVISION:
            if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER) {
                result = malloc(sizeof(int));
                *((int*)result) = *((int*)left) / *((int*)right); // integer division
            } else if (node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_FLOAT) {
                result = malloc(sizeof(double));
                *((double*)result) = floor(*((double*)left) / *((double*)right));
            }
            break;
        default:
            fprintf(stderr, "Error: Unsupported binary operator %d\n", node->data.binary_op.op);
            exit(EXIT_FAILURE);
    }

    free(left);
    free(right);
    return result;
}

static void* evaluate_comparison(ASTNode* node) {
    void* left = evaluate_ast(node->data.comparison.left);
    void* right = evaluate_ast(node->data.comparison.right);
    void* result = malloc(sizeof(int));

    switch (node->data.comparison.op) {
        case EQUAL:
            if (node->data.comparison.left->type == AST_NUMBER && node->data.comparison.right->type == AST_NUMBER) {
                *((int*)result) = (*((int*)left) == *((int*)right));
            } else if (node->data.comparison.left->type == AST_FLOAT && node->data.comparison.right->type == AST_FLOAT) {
                *((int*)result) = (*((double*)left) == *((double*)right));
            } else if (node->data.comparison.left->type == AST_STRING && node->data.comparison.right->type == AST_STRING) {
                *((int*)result) = (strcmp((char*)left, (char*)right) == 0);
            }
            break;
        case NOT_EQUAL:
            if (node->data.comparison.left->type == AST_NUMBER && node->data.comparison.right->type == AST_NUMBER) {
                *((int*)result) = (*((int*)left) != *((int*)right));
            } else if (node->data.comparison.left->type == AST_FLOAT && node->data.comparison.right->type == AST_FLOAT) {
                *((int*)result) = (*((double*)left) != *((double*)right));
            } else if (node->data.comparison.left->type == AST_STRING && node->data.comparison.right->type == AST_STRING) {
                *((int*)result) = (strcmp((char*)left, (char*)right) != 0);
            }
            break;
        case GREATER:
            if (node->data.comparison.left->type == AST_NUMBER && node->data.comparison.right->type == AST_NUMBER) {
                *((int*)result) = (*((int*)left) > *((int*)right));
            } else if (node->data.comparison.left->type == AST_FLOAT && node->data.comparison.right->type == AST_FLOAT) {
                *((int*)result) = (*((double*)left) > *((double*)right));
            }
            break;
        case LESS:
            if (node->data.comparison.left->type == AST_NUMBER && node->data.comparison.right->type == AST_NUMBER) {
                *((int*)result) = (*((int*)left) < *((int*)right));
            } else if (node->data.comparison.left->type == AST_FLOAT && node->data.comparison.right->type == AST_FLOAT) {
                *((int*)result) = (*((double*)left) < *((double*)right));
            }
            break;
        case GREATER_EQUAL:
            if (node->data.comparison.left->type == AST_NUMBER && node->data.comparison.right->type == AST_NUMBER) {
                *((int*)result) = (*((int*)left) >= *((int*)right));
            } else if (node->data.comparison.left->type == AST_FLOAT && node->data.comparison.right->type == AST_FLOAT) {
                *((int*)result) = (*((double*)left) >= *((double*)right));
            }
            break;
        case LESS_EQUAL:
            if (node->data.comparison.left->type == AST_NUMBER && node->data.comparison.right->type == AST_NUMBER) {
                *((int*)result) = (*((int*)left) <= *((int*)right));
            } else if (node->data.comparison.left->type == AST_FLOAT && node->data.comparison.right->type == AST_FLOAT) {
                *((int*)result) = (*((double*)left) <= *((double*)right));
            }
            break;
        default:
            fprintf(stderr, "Error: Unsupported comparison operator %d\n", node->data.comparison.op);
            exit(EXIT_FAILURE);
    }

    free(left);
    free(right);
    return result;
}

static void* evaluate_logical_op(ASTNode* node) {
    void* left = evaluate_ast(node->data.logical_op.left);
    void* right = evaluate_ast(node->data.logical_op.right);
    void* result = malloc(sizeof(int));

    switch (node->data.logical_op.op) {
        case LOGICAL_AND:
            *((int*)result) = (*((int*)left) && *((int*)right));
            break;
        case LOGICAL_OR:
            *((int*)result) = (*((int*)left) || *((int*)right));
            break;
        case LOGICAL_NOT:
            *((int*)result) = !(*((int*)left));
            break;
        default:
            fprintf(stderr, "Error: Unsupported logical operator %d\n", node->data.logical_op.op);
            exit(EXIT_FAILURE);
    }

    free(left);
    free(right);
    return result;
}

void* evaluate_ast(ASTNode* root) {
    switch (root->type) {
        case AST_NUMBER:
        case AST_FLOAT:
        case AST_COMPLEX:
        case AST_STRING:
        case AST_BOOLEAN:
            return evaluate_literal(root);
        case AST_BINARY_OP:
            return evaluate_binary_op(root);
        case AST_UNARY_OP:
            return evaluate_unary_op(root);
        case AST_COMPARISON:
            return evaluate_comparison(root);
        case AST_LOGICAL_OP:
            return evaluate_logical_op(root);
        case AST_IDENTIFIER:
            return get_variable(root->data.identifier);
        case AST_ASSIGNMENT: {
            void* value = evaluate_ast(root->data.assignment_op.right);
            set_variable(root->data.assignment_op.left->data.identifier, value);
            return value;
        }
        default:
            fprintf(stderr, "Error: Unexpected AST node type %d\n", root->type);
            exit(EXIT_FAILURE);
    }

    return NULL;
}

void set_variable(const char* name, void* value) {
    Variable* var = variables;
    while (var) {
        if (strcmp(var->name, name) == 0) {
            var->value = value;
            return;
        }
        var = var->next;
    }
    Variable* new_var = (Variable*)malloc(sizeof(Variable));
    new_var->name = strdup(name);
    new_var->value = value;
    new_var->next = variables;
    variables = new_var;
}

void* get_variable(const char* name) {
    Variable* var = variables;
    while (var) {
        if (strcmp(var->name, name) == 0) {
            return var->value;
        }
        var = var->next;
    }
    fprintf(stderr, "Error: Variable '%s' not found\n", name);
    exit(EXIT_FAILURE);
}

void free_variables() {
    Variable* var = variables;
    while (var) {
        Variable* next = var->next;
        free(var->name);
        free(var->value);
        free(var);
        var = next;
    }
    variables = NULL;
}
