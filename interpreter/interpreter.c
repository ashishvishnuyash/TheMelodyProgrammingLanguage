#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

Variable *variables = NULL;

void* set_variable(const char* name, void* value) {
    Variable* var = variables;
    while (var) {
        if (strcmp(var->name, name) == 0) {
            var->value = value;
            return value;
        }
        var = var->next;  // Correctly use var->next
    }
    Variable* new_var = (Variable*)malloc(sizeof(Variable));
    new_var->name = strdup(name);
    new_var->value = value;
    new_var->next = variables;  // Correctly use new_var->next
    variables = new_var;
    return value;
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



static void* evaluate_literal(ASTNode* node) {
    void* value = NULL;
    switch (node->type) {
        case AST_NUMBER: {
            value = malloc(sizeof(int));
           *((int*)value) =node->data.number;
            
            break;
        }
        case AST_FLOAT: {
            value = malloc(sizeof(double));
            *((double*)value) =node->data.float_number;
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
    void* left = interpret(node->data.binary_op.left);
    void* right = interpret(node->data.binary_op.right);
    
    switch (node->data.binary_op.op){
        case PLUS:
            if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER){
            value = malloc(sizeof(int));
            *((int*)value) = (*((int*)left) + *((int*)right));
            return value;
                
            }else if (node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_FLOAT || node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_NUMBER || node->data.binary_op.left->type == AST_NUMBER  && node->data.binary_op.right->type == AST_FLOAT){
                value = malloc(sizeof(double));
                *((double*)value) = (*((double*)left) + *((double*)right));
                return value;
            }
            
            
        case MINUS:
             if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left) - *((int*)right));
                return value;
                
            }else if (node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_FLOAT || node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_NUMBER || node->data.binary_op.left->type == AST_NUMBER  && node->data.binary_op.right->type == AST_FLOAT){
                value = malloc(sizeof(double));
                *((double*)value) = (*((double*)left) - *((double*)right));
                return value;
            }
        case MULTIPLY:
             if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left) * *((int*)right));
                return value;
                
            }else if (node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_FLOAT || node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_NUMBER || node->data.binary_op.left->type == AST_NUMBER  && node->data.binary_op.right->type == AST_FLOAT){
                value = malloc(sizeof(double));
                *((double*)value) = (*((double*)left) * *((double*)right));
                return value;
            }
        case DIVIDE:
             if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left) / *((int*)right));
                return value;
                
            }else if (node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_FLOAT || node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_NUMBER || node->data.binary_op.left->type == AST_NUMBER  && node->data.binary_op.right->type == AST_FLOAT){
                value = malloc(sizeof(double));
                *((double*)value) = (*((double*)left) / *((double*)right));
                return value;
            }
        case EXPONENTIATION:
            if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER){
                value = malloc(sizeof(int));
                *((int*)value) = (int)pow(*((int*)left),*((int*)right));
                return value;
            }else if (node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_FLOAT || node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_NUMBER || node->data.binary_op.left->type == AST_NUMBER  && node->data.binary_op.right->type == AST_FLOAT){
                value = malloc(sizeof(double));
                *((double*)value) = (double)pow(*((double*)left), *((double*)right));
                return value;
            }
            
        case MODULUS:
            if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left) % *((int*)right));
                return value;
            }
        case FLOOR_DIVISION:
             if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER){
                value = malloc(sizeof(int));
                *((int*)value) = (int)floor(*((int*)left) / *((int*)right));
                return value;
             }else if (node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_FLOAT || node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_NUMBER || node->data.binary_op.left->type == AST_NUMBER  && node->data.binary_op.right->type == AST_FLOAT){
                value = malloc(sizeof(double));
                *((double*)value) = (double)floor(*((double*)left) / *((double*)right));
                return value;
             }
        case EQUAL:
            if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left) == *((int*)right));
                return value;
            }else if (node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_FLOAT || node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_NUMBER || node->data.binary_op.left->type == AST_NUMBER  && node->data.binary_op.right->type == AST_FLOAT){
                value = malloc(sizeof(double));
                *((int*)value) = (*((double*)left) == *((double*)right));
                return value;
            }
        case NOT_EQUAL:
            if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left) != *((int*)right));
                return value;
            } else if (node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_FLOAT || node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_NUMBER || node->data.binary_op.left->type == AST_NUMBER  && node->data.binary_op.right->type == AST_FLOAT){
                value = malloc(sizeof(double));
                *((int*)value) = (*((double*)left) != *((double*)right));
                return value;
            }
        case LESS:
            if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left) < *((int*)right));
                return value;
            }else if (node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_FLOAT || node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_NUMBER || node->data.binary_op.left->type == AST_NUMBER  && node->data.binary_op.right->type == AST_FLOAT){
                value = malloc(sizeof(double));
                *((int*)value) = (*((double*)left) < *((double*)right));
                return value;
            }
        case GREATER:
            if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left) > *((int*)right));
                return value;
            }else if (node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_FLOAT || node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_NUMBER || node->data.binary_op.left->type == AST_NUMBER  && node->data.binary_op.right->type == AST_FLOAT){
                value = malloc(sizeof(double));
                *((int*)value) = (*((double*)left) > *((double*)right));
                return value;
            }
        case LESS_EQUAL:
            if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left) <= *((int*)right));
                return value;
            }else if (node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_FLOAT || node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_NUMBER || node->data.binary_op.left->type == AST_NUMBER  && node->data.binary_op.right->type == AST_FLOAT){
                value = malloc(sizeof(double));
                *((int*)value) = (*((double*)left) <= *((double*)right));
                return value;
            }
            
        case GREATER_EQUAL:
            if (node->data.binary_op.left->type == AST_NUMBER && node->data.binary_op.right->type == AST_NUMBER){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left) >= *((int*)right));
                return value;
            }else if (node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_FLOAT || node->data.binary_op.left->type == AST_FLOAT && node->data.binary_op.right->type == AST_NUMBER || node->data.binary_op.left->type == AST_NUMBER  && node->data.binary_op.right->type == AST_FLOAT){
                value = malloc(sizeof(double));
                *((int*)value) = (*((double*)left) >= *((double*)right));
                return value;
            }
            
        case LOGICAL_AND:
            value = malloc(sizeof(int));
            *((int*)value) = (*((int*)left) && *((int*)right));
            return value;
        case LOGICAL_OR:
            value = malloc(sizeof(int));
            *((int*)value) = (*((int*)left) || *((int*)right));
            return value;
        case BITWISE_AND:
            value = malloc(sizeof(int));
            *((int*)value) = (*((int*)left) & *((int*)right));
            return value;
        case BITWISE_OR:
            value = malloc(sizeof(int));
            *((int*)value) = (*((int*)left) | *((int*)right));
            return value;
        case BITWISE_XOR:
            value = malloc(sizeof(int));
            *((int*)value) = (*((int*)left) ^ *((int*)right));
            return value;
        case SHIFT_LEFT:
            value = malloc(sizeof(int));
            *((int*)value) = (*((int*)left) << *((int*)right));
            return value;
        case SHIFT_RIGHT:
            value = malloc(sizeof(int));
            *((int*)value) = (*((int*)left) >> *((int*)right));
            return value;
            default:
            fprintf(stderr, "Error: Unknown AST node type gv %d\n ", node->type);
            exit(1);
    }
        }

       
      


void* evaluate_unary_op(ASTNode* node) {
    void* value = NULL;


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

       


void* interpret(ASTNode* node) {
    if (node == NULL) {
        fprintf(stderr, "Error: NULL node in AST\n");
        exit(1);
    }

    switch (node->type) {
        case AST_NUMBER:
        case AST_FLOAT:
            return evaluate_literal(node);
        
        case AST_BINARY_OP:
            return evaluate_binary_op(node); 
        case AST_UNARY_OP:
            return evaluate_unary_op(node);
        case AST_IDENTIFIER:
            // return get_variable(node);
        case AST_ASSIGNMENT:{
          
            void* value;
            value = interpret(node->data.assignment_op.right);
            set_variable(node->data.assignment_op.left->data.identifier, value);
            return value;
        }
        // case AST_COMPARISON:
        //     return evaluate_comparison(node);
       
        default:
            fprintf(stderr, "Error: Unknown AST node type  %d\n ", node->type);
            exit(1);
    }
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