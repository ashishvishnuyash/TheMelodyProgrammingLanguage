#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

Variable *variables = NULL;


static void* evaluate_literal(ASTNode* node) {
    void* value = NULL;
    switch (node->type) {
        case AST_NUMBER: {
            value = malloc(sizeof(int));
           *((int*)value) =node->data.number;
            
            break;
        }
       
        default:
            fprintf(stderr, "Error: Unknown AST node type v %d\n ", node->type);
            exit(1);
    }
    return value;
}

static void* evaluate_binary_op(ASTNode* node) {
    void* value = NULL;
    switch (node->type) {
        case AST_BINARY_OP: {
            switch (node->data.binary_op.op){
                case PLUS:
                    
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) + *((int*)interpret(node->data.binary_op.right)));
                    return value;
                case MINUS:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) - *((int*)interpret(node->data.binary_op.right)));
                    return value;
                case MULTIPLY:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) * *((int*)interpret(node->data.binary_op.right)));
                    return value;
                case DIVIDE:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) / *((int*)interpret(node->data.binary_op.right)));
                    return value;
                case EXPONENTIATION:
                    value = malloc(sizeof(int));
                    *((int*)value) = (int)pow(*((int*)interpret(node->data.binary_op.left)),*((int*)interpret(node->data.binary_op.right)));
                    return value;
                case MODULUS:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) % *((int*)interpret(node->data.binary_op.right)));
                    return value;
                case FLOOR_DIVISION:
                    value = malloc(sizeof(int));
                    *((int*)value) = (int)floor(*((int*)interpret(node->data.binary_op.left)) / *((int*)interpret(node->data.binary_op.right)));
                    return value;
                case EQUAL:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) == *((int*)interpret(node->data.binary_op.right)));
                    return value;
                case NOT_EQUAL:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) != *((int*)interpret(node->data.binary_op.right)));
                    return value;
                case LESS:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) < *((int*)interpret(node->data.binary_op.right)));
                    return value;
                case GREATER:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) > *((int*)interpret(node->data.binary_op.right)));
                    return value;
                case LESS_EQUAL:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) <= *((int*)interpret(node->data.binary_op.right)));
                    return value;
                case GREATER_EQUAL:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) >= *((int*)interpret(node->data.binary_op.right)));
                    return value;
                case LOGICAL_AND:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) && *((int*)interpret(node->data.binary_op.right)));
                    return value;
                case LOGICAL_OR:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) || *((int*)interpret(node->data.binary_op.right)));
                    return value;
                case BITWISE_AND:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) & *((int*)interpret(node->data.binary_op.right)));
                    return value;
                case BITWISE_OR:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) | *((int*)interpret(node->data.binary_op.right)));
                    return value;
                case BITWISE_XOR:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) ^ *((int*)interpret(node->data.binary_op.right)));
                    return value;
                case SHIFT_LEFT:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) << *((int*)interpret(node->data.binary_op.right)));
                    return value;
                case SHIFT_RIGHT:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) >> *((int*)interpret(node->data.binary_op.right)));
                    return value;
                 default:
                    fprintf(stderr, "Error: Unknown AST node type gv %d\n ", node->type);
                    exit(1);
            }
        }

        default:
            fprintf(stderr, "Error: Unknown AST node typeg v %d\n ", node->type);
            exit(1);

    }
   
}

void* evaluate_unary_op(ASTNode* node) {
    void* value = NULL;

    switch (node->type) {
        case AST_UNARY_OP: {
            switch (node->data.unary_op.op){
                case MINUS:
                    value = malloc(sizeof(int));
                    *((int*)value) = -(*((int*)interpret(node->data.unary_op.operand)));
                    return value;
                case PLUS:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.unary_op.operand)));
                    return value;
                case INCREMENT:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.unary_op.operand)))+1;
                    return value;
                case DECREMENT:
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)interpret(node->data.unary_op.operand)))-1;
                    return value;
                case LOGICAL_NOT:
                    value = malloc(sizeof(int));
                    *((int*)value) = !(*((int*)interpret(node->data.unary_op.operand)));
                case BITWISE_NOT:
                    value = malloc(sizeof(int));
                    *((int*)value) = ~(*((int*)interpret(node->data.unary_op.operand)));
                
                default:
                    fprintf(stderr, "Error: Unknown AST node type v %d\n ", node->type);
                    exit(1);
            }
        }

        default:
            fprintf(stderr, "Error: Unknown AST node type v %d\n ", node->type);
            exit(1);

    }

}
// void* evaluate_comparison(ASTNode* node) {
    void* value = NULL;

//     switch (node->type) {
//         case AST_COMPARISON: {
//             switch (node->data.binary_op.op){
//                 case EQUAL:
//                     value = malloc(sizeof(int));
//                     *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) == *((int*)interpret(node->data.binary_op.right)));
//                     return value;
//                 case NOT_EQUAL:
//                     value = malloc(sizeof(int));
//                     *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) != *((int*)interpret(node->data.binary_op.right)));
//                     return value;
//                 case LESS:
//                     value = malloc(sizeof(int));
//                     *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) < *((int*)interpret(node->data.binary_op.right)));
//                     return value;
//                 case GREATER:
//                     value = malloc(sizeof(int));
//                     *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) > *((int*)interpret(node->data.binary_op.right)));
//                     return value;
//                 case LESS_EQUAL:
//                     value = malloc(sizeof(int));
//                     *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) <= *((int*)interpret(node->data.binary_op.right)));
//                     return value;
//                 case GREATER_EQUAL:
//                     value = malloc(sizeof(int));
//                     *((int*)value) = (*((int*)interpret(node->data.binary_op.left)) >= *((int*)interpret(node->data.binary_op.right)));
//                     return value;
//                 default:
//                     fprintf(stderr, "Error: Unknown AST node type v %d\n ", node->type);
//                     exit(1);
//             };
        
//         }
//         default:
//             fprintf(stderr, "Error: Unknown AST node type v %d\n ", node->type);
//             exit(1);
//     }
// }
// void* evaluate_logistic(ASTNode* node) {
//     void* value = NULL;

//     switch (node->type) {
//         case AST_LOGICAL_OP: {
//             switch (node->data.logical_op.op){
//                 case LOGICAL_AND:
//                     value = malloc(sizeof(int));
//                     *((int*)value) = (*((int*)interpret(node->data.logical_op.left)) && *((int*)interpret(node->data.logical_op.right)));
//                     return value;
//                 case LOGICAL_OR:
//                     value = malloc(sizeof(int));
//                     *((int*)value) = (*((int*)interpret(node->data.logical_op.left)) || *((int*)interpret(node->data.logical_op.right)));
//                     return value;
                
//             }
//         }
//     }
// }

void* interpret(ASTNode* node) {
    if (node == NULL) {
        fprintf(stderr, "Error: NULL node in AST\n");
        exit(1);
    }

    switch (node->type) {
        case AST_NUMBER:
            return evaluate_literal(node);
        case AST_BINARY_OP:
            return evaluate_binary_op(node); 
        // case AST_UNARY_OP:
        //     return evaluate_unary_op(node);
        // case AST_COMPARISON:
        //     return evaluate_comparison(node);
       
        default:
            fprintf(stderr, "Error: Unknown AST node type v %d\n ", node->type);
            exit(1);
    }
}