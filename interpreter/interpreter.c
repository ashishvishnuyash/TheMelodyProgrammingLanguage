#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

Variable *variables = NULL;
static Function* functions = NULL;
char* concat(const char *s1, const char *s2)
{
    const size_t len1 = strlen(s1);
    const size_t len2 = strlen(s2);
    char *result = malloc(len1 + len2 + 1); // +1 for the null-terminator
    // in real code you would check for errors in malloc here
    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2 + 1); // +1 to copy the null-terminator
    return result;
}

//return data type of the value if integer, ot
//return 0 if float

Literal* set_variable(const char* name, Literal* value) {
    
    Variable* var = variables;
    
    // printf("set_variable: %s\n", name);
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
    // printf(">>%s",variables->name);
    // printf("<<%d",*((int*)value->value));
    return value;
}



Literal* get_variable(const char* name) {
    Variable* var = variables;
    while (var) {
        if (strcmp(var->name, name) == 0) {
            // return var->value;
            // printf("get_variable: %s\n", name);
            return var->value;
        }
        var = var->next;
    }
    fprintf(stderr, "Error: Variable '%s' not found\n", name);
    exit(EXIT_FAILURE);
}
static Function* get_function(const char* name) {
    Function* func = functions;
    while (func) {
        if (strcmp(func->name, name) == 0) {
            return func;
        }
        func = func->next;
    }
    fprintf(stderr, "Error: Undefined function '%s'\n", name);
    exit(EXIT_FAILURE);
}
static void set_function(const char* name, ASTNode* body, char** parameters, int param_count) {
    Function* func = functions;
    while (func) {
        if (strcmp(func->name, name) == 0) {
            func->body = body;
            func->parameters = parameters;
            func->param_count = param_count;
            return;
        }
        func = func->next;
    }

    func = (Function*)malloc(sizeof(Function));
    func->name = strdup(name);
    func->body = body;
    func->parameters = parameters;
    func->param_count = param_count;
    func->next = functions;
    functions = func;
    // print body ast
    
    // printf("set_function: %s\n", name);
}



 Literal* evaluate_literal(ASTNode* node) {
    void* value = NULL;
    switch (node->type) {
        case AST_NUMBER: {
            value = malloc(sizeof(int));
           *((int*)value) =node->data.number;
           Literal* literal = (Literal*)malloc(sizeof(Literal));
           literal->type = TYPE_INT;
           literal->value = value;
           return literal;
            
            // break;
        }
        case AST_FLOAT: {
            value = malloc(sizeof(double));
            *((double*)value) =node->data.float_number;
            Literal* literal = (Literal*)malloc(sizeof(Literal));
            literal->type = TYPE_FLOAT;
            literal->value = value;
            return literal;
            // break;
        }
        case AST_STRING: {
            value = malloc(sizeof(char*));
            *((char**)value) = strdup(node->data.string);
            Literal* literal = (Literal*)malloc(sizeof(Literal));
            literal->type = TYPE_STRING;
            literal->value = value;
            return literal;
        }
        
        
       
        default:
            fprintf(stderr, "Error: Unknown AST node type  %d\n ", node->type);
            exit(1);
    }
    // return value;
}

Literal* evaluate_binary_op(ASTNode* node) {
    void* value = NULL;
    Literal* left = interpret(node->data.binary_op.left);
    Literal* right = interpret(node->data.binary_op.right);
        
    switch (node->data.binary_op.op){
        case PLUS:{
            
                       
            if (left->type == TYPE_INT && right->type == TYPE_INT){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left->value) + *((int*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
                // return value;
                
            }else if (left->type == TYPE_FLOAT && right->type == TYPE_FLOAT || left->type == TYPE_INT && right->type == TYPE_FLOAT || left->type == TYPE_FLOAT && right->type == TYPE_INT){
                value = malloc(sizeof(double));
                *((double*)value) = (*((double*)left->value) + *((double*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_FLOAT;
                literal->value = value;
                return literal;
                
            }else if(left->type==TYPE_STRING && right->type== TYPE_STRING){
                value = malloc(sizeof(char*));
                *((char**)value) = concat(*((char**)left->value),*((char**)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_STRING;
                literal->value = value;
                return literal;
            }
            }
            
            
        case MINUS:
             if (left->type == TYPE_INT && right->type == TYPE_INT){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left->value) - *((int*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
                
                
            }else if (left->type == TYPE_FLOAT && right->type == TYPE_FLOAT || left->type == TYPE_INT && right->type == TYPE_FLOAT || left->type == TYPE_FLOAT && right->type == TYPE_INT){
                value = malloc(sizeof(double));
                *((double*)value) = (*((double*)left->value) - *((double*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_FLOAT;
                literal->value = value;
                return literal;

            }
        case MULTIPLY:
             if (left->type == TYPE_INT && right->type == TYPE_INT){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left->value) * *((int*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
                
            }else if (left->type == TYPE_FLOAT && right->type == TYPE_FLOAT || left->type == TYPE_INT && right->type == TYPE_FLOAT || left->type == TYPE_FLOAT && right->type == TYPE_INT){
                value = malloc(sizeof(double));
                *((double*)value) = (*((double*)left->value) * *((double*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_FLOAT;
                literal->value = value;
                return literal;
                
            }
        case DIVIDE:
             if (left->type == TYPE_INT && right->type == TYPE_INT){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left->value) / *((int*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
                
            }else if (left->type == TYPE_FLOAT && right->type == TYPE_FLOAT || left->type == TYPE_INT && right->type == TYPE_FLOAT || left->type == TYPE_FLOAT && right->type == TYPE_INT){
                value = malloc(sizeof(double));
                *((double*)value) = (*((double*)left->value) / *((double*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_FLOAT;
                literal->value = value;
                return literal;
            }
        case EXPONENTIATION:
            if (left->type == TYPE_INT && right->type == TYPE_INT){
                value = malloc(sizeof(int));
                *((int*)value) = (int)pow(*((int*)left->value),*((int*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
            }else if (left->type == TYPE_FLOAT && right->type == TYPE_FLOAT || left->type == TYPE_INT && right->type == TYPE_FLOAT || left->type == TYPE_FLOAT && right->type == TYPE_INT){
                value = malloc(sizeof(double));
                *((double*)value) = (double)pow(*((double*)left->value), *((double*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_FLOAT;
                literal->value = value;
                return literal;
            }
            
        case MODULUS:
            if (left->type == TYPE_INT && right->type == TYPE_INT){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left->value) % *((int*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
            }
        case FLOOR_DIVISION:
             if (left->type == TYPE_INT && right->type == TYPE_INT){
                value = malloc(sizeof(int));
                *((int*)value) = (int)floor(*((int*)left->value) / *((int*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;

             }else if (left->type == TYPE_FLOAT && right->type == TYPE_FLOAT || left->type == TYPE_INT && right->type == TYPE_FLOAT || left->type == TYPE_FLOAT && right->type == TYPE_INT){
                value = malloc(sizeof(double));
                *((double*)value) = (double)floor(*((double*)left->value) / *((double*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_FLOAT;
                literal->value = value;
                return literal;
             }
        case EQUAL:
            if (left->type == TYPE_INT && right->type == TYPE_INT){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left->value) == *((int*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
            }else if (left->type == TYPE_FLOAT && right->type == TYPE_FLOAT || left->type == TYPE_INT && right->type == TYPE_FLOAT || left->type == TYPE_FLOAT && right->type == TYPE_INT){
                value = malloc(sizeof(double));
                *((int*)value) = (*((double*)left->value) == *((double*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
            }
        case NOT_EQUAL:
            if (left->type == TYPE_INT && right->type == TYPE_INT){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left->value) != *((int*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
            } else if (left->type == TYPE_FLOAT && right->type == TYPE_FLOAT || left->type == TYPE_INT && right->type == TYPE_FLOAT || left->type == TYPE_FLOAT && right->type == TYPE_INT){
                value = malloc(sizeof(double));
                *((int*)value) = (*((double*)left->value) != *((double*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
            }
        case LESS:
            if (left->type == TYPE_INT && right->type == TYPE_INT){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left->value) < *((int*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
            }else if (left->type == TYPE_FLOAT && right->type == TYPE_FLOAT || left->type == TYPE_INT && right->type == TYPE_FLOAT || left->type == TYPE_FLOAT && right->type == TYPE_INT){
                value = malloc(sizeof(double));
                *((int*)value) = (*((double*)left->value) < *((double*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
            }
        case GREATER:
            if (left->type == TYPE_INT && right->type == TYPE_INT){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left->value) > *((int*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
            }else if (left->type == TYPE_FLOAT && right->type == TYPE_FLOAT || left->type == TYPE_INT && right->type == TYPE_FLOAT || left->type == TYPE_FLOAT && right->type == TYPE_INT){
                value = malloc(sizeof(double));
                *((int*)value) = (*((double*)left->value) > *((double*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
            }
        case LESS_EQUAL:
            if (left->type == TYPE_INT && right->type == TYPE_INT){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left->value) <= *((int*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
            }else if (left->type == TYPE_FLOAT && right->type == TYPE_FLOAT || left->type == TYPE_INT && right->type == TYPE_FLOAT || left->type == TYPE_FLOAT && right->type == TYPE_INT){
                value = malloc(sizeof(double));
                *((int*)value) = (*((double*)left->value) <= *((double*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
            }
            
        case GREATER_EQUAL:
            if (left->type == TYPE_INT && right->type == TYPE_INT){
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left) >= *((int*)right));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
            }else if (left->type == TYPE_FLOAT && right->type == TYPE_FLOAT || left->type == TYPE_INT && right->type == TYPE_FLOAT || left->type == TYPE_FLOAT && right->type == TYPE_INT){
                value = malloc(sizeof(double));
                *((int*)value) = (*((double*)left->value) >= *((double*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
                
            }
            
        case LOGICAL_AND:
            if (left->type == TYPE_INT && right->type == TYPE_INT)
            {
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)left->value) && *((int*)right->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
            }
            
            
            
        case LOGICAL_OR:
            if (left->type == TYPE_INT && right->type == TYPE_INT){
            value = malloc(sizeof(int));
            *((int*)value) = (*((int*)left->value) || *((int*)right->value));
            Literal *literal = (Literal*)malloc(sizeof(Literal));
            literal->type = TYPE_INT;
            literal->value = value;
            return literal;
            }
        case BITWISE_AND:
            if (left->type == TYPE_INT && right->type == TYPE_INT){
            value = malloc(sizeof(int));
            *((int*)value) = (*((int*)left->value) & *((int*)right->value));
            Literal *literal = (Literal*)malloc(sizeof(Literal));
            literal->type = TYPE_INT;
            literal->value = value;
            return literal;
            }
        case BITWISE_OR:
            if (left->type == TYPE_INT && right->type == TYPE_INT){
            value = malloc(sizeof(int));
            *((int*)value) = (*((int*)left->value) | *((int*)right->value));
            Literal *literal = (Literal*)malloc(sizeof(Literal));
            literal->type = TYPE_INT;
            literal->value = value;
            return literal;
            }
        case BITWISE_XOR:
            if (left->type == TYPE_INT && right->type == TYPE_INT){
            value = malloc(sizeof(int));
            *((int*)value) = (*((int*)left->value) ^ *((int*)right->value));
            Literal *literal = (Literal*)malloc(sizeof(Literal));
            literal->type = TYPE_INT;
            literal->value = value;
            return literal;
            }
        case SHIFT_LEFT:
            if(left->type == TYPE_INT && right->type == TYPE_INT){
            value = malloc(sizeof(int));
            *((int*)value) = (*((int*)left->value) << *((int*)right->value));
            
            Literal *literal = (Literal*)malloc(sizeof(Literal));
            literal->type = TYPE_INT;
            literal->value = value;
            return literal;
            }
        case SHIFT_RIGHT:
            if(left->type == TYPE_INT && right->type == TYPE_INT){
            value = malloc(sizeof(int));
            *((int*)value) = (*((int*)left->value) >> *((int*)right->value));
            Literal *literal = (Literal*)malloc(sizeof(Literal));
            literal->type = TYPE_INT;
            literal->value = value;
            return literal;
            }
        default:
            fprintf(stderr, "Error: Unknown AST node type gv %d\n ", node->type);
            exit(1);
    }
        }

       
      


Literal* evaluate_unary_op(ASTNode* node) {
    void* value = NULL;
    Literal* operand = interpret(node->data.unary_op.operand);


    switch (node->data.unary_op.op){
        case MINUS:
            if (operand->type == TYPE_INT){

            value = malloc(sizeof(int));
            *((int*)value) = -(*((int*)operand->type));
            Literal *literal = (Literal*)malloc(sizeof(Literal));
            literal->type = TYPE_INT;
            literal->value = value;
            return literal;
            }
        case PLUS:
            if (operand->type == TYPE_INT){

            value = malloc(sizeof(int));
            *((int*)value) = (*((int*)operand->type));
            Literal *literal = (Literal*)malloc(sizeof(Literal));
            literal->type = TYPE_INT;
            literal->value = value;
            return literal;
            }
        case INCREMENT:
            if (operand->type==TYPE_INT){
                // printf("Incrementing%d\n", *((int*)operand->value)+1);

                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)operand->value))+1;
                // printf("Incremented to %d\n", *((int*)value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
            }
            // return value;
        case DECREMENT:
            if(operand->type==TYPE_INT){

            
                value = malloc(sizeof(int));
                *((int*)value) = (*((int*)operand->value))-1;
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
                }
        case LOGICAL_NOT:
            if(operand->type==TYPE_INT){
                value = malloc(sizeof(int));
                *((int*)value) = !(*((int*)operand->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
            }
            
        case BITWISE_NOT:
            if(operand->type==TYPE_INT){
                value = malloc(sizeof(int));
                *((int*)value) = ~(*((int*)operand->value));
                Literal *literal = (Literal*)malloc(sizeof(Literal));
                literal->type = TYPE_INT;
                literal->value = value;
                return literal;
            }
             
        
        default:
            fprintf(stderr, "Error: Unknown AST node type v %d\n ", node->type);
            exit(1);
    }
}

Literal* evaluate_assignment(ASTNode* node) {
    void* value = NULL;
    char* left = node->data.assignment_op.left->data.identifier;
    Literal* right = interpret(node->data.assignment_op.right);
    // printf("Left: %s\n", left);
    // printf("Right: %d\n", *((int*)right->value));
    // if (left == NULL) {
    //     fprintf(stderr, "Error: Left hand side of assignment is not a variable\n");
    //     exit(1);
    // }
    // printf("Left: %d\n", *((int*)left->value));
    switch (node->data.assignment_op.op) {
        case ASSIGN:
            // printf("Left: %s\n", left);
            // printf("Right: %d\n", *((int*)right->value));
            
            set_variable(left, right);

            return right;
        case PLUS_ASSIGN:{
            // printf("Left++: %s\n", left);
                Literal *left_literal = get_variable(left);
                if(left_literal->type == TYPE_INT && right->type == TYPE_INT){
                    // printf("Left:++++ %s\n", *((char**)left_literal->value));
                    value = malloc(sizeof(int));
                    *((int*)value) = (*((int*)left_literal->value)) + (*((int*)right->value));
                    Literal *literal = (Literal*)malloc(sizeof(Literal));
                    literal->type = TYPE_INT;
                    literal->value = value;               

                    set_variable(left, literal);
                    return literal;
                }
                else if(left_literal->type == TYPE_STRING && right->type == TYPE_STRING){
                    // printf("+++%s\n",*((char**)left_literal->value));
                    // printf("%s\n",*((char**)right->value));
                    
                    value = malloc(sizeof(char*));
                    *((char**)value) = concat(*((char**)left_literal->value),*((char**)right->value));
                    Literal *literal = (Literal*)malloc(sizeof(Literal));
                    literal->type = TYPE_STRING;
                    literal->value = value;
                    set_variable(left, literal);
                    // return literal;
                    return literal;
                }
                else if(left_literal->type == TYPE_FLOAT && right->type == TYPE_FLOAT || left_literal->type == TYPE_FLOAT && right->type == TYPE_INT || left_literal->type == TYPE_INT && right->type == TYPE_FLOAT){
                    value = malloc(sizeof(float));
                    *((float*)value) = (*((float*)left_literal->value)) + (*((float*)right->value));
                    set_variable(left, value);
                    return value;
                }
                else{
                    fprintf(stderr, "Error: Invalid types for + assignment\n");
                    exit(1);
                }}
                

            
        default:
            fprintf(stderr, "Error: Unknown AST node type v %d\n ", node->type);
            exit(1);
    }
}

Literal* print_statement(ASTNode* node) {
    // printf("Print Statement - %s\n", node->data.function_call.function_name);
    // printf("%d\n",node->data.function_call.arg_count);
    for (int i = 0; i < node->data.function_call.arg_count; i++) {

        Literal* arg = interpret( node->data.function_call.arguments[i]);
        if(arg->type == TYPE_INT){
            printf("%d",*((int*)arg->value));
        }
        if (arg->type == TYPE_STRING) {
            printf("%s",*((char**)arg->value));
        }
        if (arg->type == TYPE_FLOAT) {
            printf("%f",*((float*)arg->value));
        }
        
    }
    printf("\n");
    return NULL;
}

Literal* evaluate_function_def(ASTNode* node) {
    set_function(node->data.function_def.name, node->data.function_def.body, node->data.function_def.parameters, node->data.function_def.param_count);
    return NULL;
}
Literal* evaluate_function_call(ASTNode* node) {
    // printf("Function Call: %s\n", node->data.function_call.function_name);
    

    Function* func = get_function(node->data.function_call.function_name);
   
   
    if (func == NULL) {
        fprintf(stderr, "Error: Function '%s' not defined\n", node->data.function_call.function_name);
        exit(EXIT_FAILURE);
    }

    if (func->param_count != node->data.function_call.arg_count) {
        fprintf(stderr, "Error: Incorrect number of arguments for function '%s'\n", node->data.function_call.function_name);
        exit(EXIT_FAILURE);
    }

    Variable* previous_variables = variables;
    variables = NULL;

    for (int i = 0; i < func->param_count; i++) {
        Literal* arg_value = interpret(node->data.function_call.arguments[i]);
        // printf("Arg Value: %d\n", *((int*)arg_value->value));
        set_variable(func->parameters[i], arg_value);
    }
    // printf("cll %d",func->body->type);

    Literal* result = interpret(func->body);
    // printf("cll %d",0);

    

    // printf(">%d",result->type);
   
    

    // free_variables();
    variables = previous_variables;

    return result;
}

Literal* evaluate_if_statement(ASTNode* node) {
    Literal* condition = interpret(node->data.if_statement.condition);
    int condition_result = *((int*)condition->value);
    // printf("%d",condition_result);
    if (condition_result) {
        // printf("Result: %d\n", node->data.if_statement.then_branch->type== AST_STATEMENT_LIST);
        Literal* result = interpret(node->data.if_statement.then_branch);
        printf("h");
        if (result!= NULL) {
            printf("h>>");

            return result;
        }
        // return result;
    } else if (node->data.if_statement.else_branch) {
        Literal* result = interpret(node->data.if_statement.else_branch);
        if (result != NULL) {
            return result;
        }
        // return interpret(node->data.if_statement.else_branch);
    }
    return NULL;
}

Literal* interpret(ASTNode* node) {
    if (node == NULL) {
        fprintf(stderr, "Error: NULL node in AST\n");
        exit(1);
    }
    Literal* valued = NULL;

    switch (node->type) {
        case AST_NUMBER:
        case AST_STRING:
        case AST_FLOAT:
            // printf("Literal\n");
            return evaluate_literal(node);
        
        case AST_BINARY_OP:
            // printf("Binary\n");
            return evaluate_binary_op(node); 
        case AST_UNARY_OP:
            return evaluate_unary_op(node);
        case AST_IDENTIFIER:
            // printf("Identifier\n");
            // return NULL;
            Literal* value;
            value=get_variable(node->data.identifier);
            // printf("%d\n",*((int*)value->value));
            return value;
        case AST_PRINT:
            // printf(">> %s",node->data.function_call.function_name);
            
            return print_statement(node);
        case AST_ASSIGNMENT:{
            // printf("Assignment\n");
            
            Literal* value;
            // value = interpret(node->data.assignment_op.right);
            // set_variable(node->data.assignment_op.left->data.identifier, value);
            value=evaluate_assignment(node);
            // printf("%d\n",*((int*)value->value));
            // printf("%s\n",*((char**)value->value));

            return value;
            // return NULL;
        }
        case AST_FUNCTION_DEF:
            printf("Function Definition\n");
            // printf("%s\n",node->data.function_def.name);
            // printf("%d\n",node->data.function_def.param_count);
            // printf("%d\n",node->data.function_def.body->type);
            // for (int i = 0; i < node->data.function_def.param_count; i++) {
            //     printf("%s\n",node->data.function_def.parameters[i]);
            // }
            return evaluate_function_def(node);
        case AST_FUNCTION_CALL:
            printf("Function Call\n");
            // Literal* value;
            // value = ;
            // printf("%s",*((char**)value->value));       
            // printf("%s\n",node->data.function_def.name);
            return evaluate_function_call(node);
        case AST_RETURN:
            // printf("...Return\n");
            // Literal* value = 
            // printf("%d\n",*((int*)value->value));

            return interpret(node->data.return_value);;


            // printf("%d\n",node->data.function_def.parameters[0]->type);
        case AST_IF_STATEMENT:
            printf("If Statement\n");
            // printf("%d\n",node->data.if_statement.condition->type);
            valued = evaluate_if_statement(node);
            // if (valued != NULL) {
            //     // printf("%s\n",*((char**)valued->value));
            //     return valued;
            // }
            return valued;
            // return set_function(node);
        case AST_STATEMENT_LIST:
            // printf("Statement List\n");
            // printf(">>>>>>>>%d\n",node->data.statement_list.statement_count);
            for (int i = 0; i < node->data.statement_list.statement_count; i++) {
                // printf("Q");                
                Literal* res;
                res = interpret(node->data.statement_list.statements[i]);
                if (node->data.statement_list.statements[i]->type==AST_RETURN) {
                    printf("...Return\n");
                    return res;
                }
                if (node->data.statement_list.statements[i]->type==AST_IF_STATEMENT) {
                    if (res != NULL){

                    return res;
                    }
                }
                
                
                
                // printf("%d\n",*((int*)res->value));
                // printf("%s\n",*((char**)res->value));
                // return res;
                // printf("%d\n",*((int*)res->value));
              
                
            }            
            return NULL;

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