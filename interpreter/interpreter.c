#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>



double interpret(ASTNode* node) {
    if (node == NULL) {
        fprintf(stderr, "Error: NULL node in AST\n");
        exit(1);
    }

    switch (node->type) {
        case AST_NUMBER:
            return node->data.number;
        
       
        case AST_BINARY_OP: {
            double left_value = interpret(node->data.binary_op.left);
            double right_value = interpret(node->data.binary_op.right);

            switch (node->data.binary_op.op) {
                case PLUS:
                    return left_value + right_value;
                case MINUS:
                    return left_value - right_value;
                case MULTIPLY:
                    return left_value * right_value;
                case DIVIDE:
                    if (right_value == 0) {
                        fprintf(stderr, "Error: Division by zero\n");
                        exit(1);
                    }
                    return left_value / right_value;
                case MODULUS:
                    if (right_value == 0) {
                        fprintf(stderr, "Error: Division by zero\n");
                        exit(1);
                    }
                    return fmod(left_value, right_value);
                case EXPONENTIATION:
                    return pow(left_value, right_value);
                case FLOOR_DIVISION:
                    if (right_value == 0) {
                        fprintf(stderr, "Error: Division by zero\n");
                        exit(1);
                    }
                    return floor(left_value / right_value);
                default:
                    fprintf(stderr, "Error: Unknown operator\n");
                    exit(1);
            }
        }

        case AST_UNARY_OP: {
            double operand_value = interpret(node->data.unary_op.operand);

            switch (node->data.unary_op.op) {
                case PLUS:
                    return +operand_value;
                case MINUS:
                    return -operand_value;
                case INCREMENT:
                    return operand_value + 1;
                case DECREMENT:
                    return operand_value - 1;
                case LOGICAL_NOT:
                    return !operand_value;
                case BITWISE_NOT:
                    return ~(int)operand_value;
                default:
                    fprintf(stderr, "Error: Unknown unary operator\n");
                    exit(1);
            }
        }

        case AST_COMPARISON: {
            double left_value = interpret(node->data.comparison.left);
            double right_value = interpret(node->data.comparison.right);

            switch (node->data.comparison.op) {
                case EQUAL:
                    return left_value == right_value;
                case NOT_EQUAL:
                    return left_value != right_value;
                case GREATER:
                    return left_value > right_value;
                case LESS:
                    return left_value < right_value;
                case GREATER_EQUAL:
                    return left_value >= right_value;
                case LESS_EQUAL:
                    return left_value <= right_value;
                default:
                    fprintf(stderr, "Error: Unknown comparison operator\n");
                    exit(1);
            }
        }
        case AST_LOGICAL_OP: {
            double left = interpret(node->data.logical_op.left);
            double right = interpret(node->data.logical_op.right);
            switch (node->data.logical_op.op) {
                case LOGICAL_AND: return left && right;
                case LOGICAL_OR: return left || right;
                case BITWISE_AND: return (int)left & (int)right;
                case BITWISE_OR: return (int)left | (int)right;
                case BITWISE_XOR: return (int)left ^ (int)right;
                case SHIFT_LEFT: return (int)left << (int)right;
                case SHIFT_RIGHT: return (int)left >> (int)right;

                default: fprintf(stderr, "Error: Unknown logical operator\n"); exit(EXIT_FAILURE);
            }
        }
       
    
            
       
        default:
            fprintf(stderr, "Error: Unknown AST node type\n " );
            exit(1);
    }
}
