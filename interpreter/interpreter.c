#include <stdio.h>
#include <stdlib.h>
#include "..\tokenizer\tokenizer.h"
#include "..\parser\parser.h"
#include "interpreter.h"

int interpret(ASTNode* node) {
    if (node == NULL) {
        fprintf(stderr, "Error: NULL node in AST\n");
        exit(1);
    }

    switch (node->type) {
        case AST_NUMBER:
            return node->data.number;

        case AST_BINARY_OP: {
            int left_value = interpret(node->data.binary_op.left);
            int right_value = interpret(node->data.binary_op.right);

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
                default:
                    fprintf(stderr, "Error: Unknown operator\n");
                    exit(1);
            }
        }

        default:
            fprintf(stderr, "Error: Unknown AST node type\n");
            exit(1);
    }
}