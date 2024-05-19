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
                default:
                    fprintf(stderr, "Error: Unknown unary operator\n");
                    exit(1);
            }
        }

        default:
            fprintf(stderr, "Error: Unknown AST node type\n");
            exit(1);
    }
}
