#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

// Function prototypes for recursive descent parser
ASTNode* parse_primary(Token** tokens);
ASTNode* parse_term(Token** tokens);
ASTNode* parse_expression(Token** tokens);
ASTNode* parse_comparison(Token** tokens);
ASTNode* parse_program(Token** tokens) ;
ASTNode* parse_statememt_list(Token** tokens) ;
ASTNode* parse_statement(Token** tokens) ;
ASTNode* parse_block(Token** tokens) ;
ASTNode* parse_if_statement(Token** tokens) ;



ASTNode* create_number_node(int value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }
    // printf("Creating number node with value %d\n", value);
    node->type = AST_NUMBER;
    node->data.number = value;
    return node;
}
// Helper function to create a string node
ASTNode* create_string_node(char* value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }
    // printf("Creating number node with value %d\n", value);
    node->type = AST_STRING;
    node->data.string = strndup(value, strlen(value));
    return node;
}

ASTNode* create_float_node(double value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }
    // printf("Creating number node with value %d\n", value);
    node->type = AST_FLOAT;
    node->data.float_number = value;
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
    node->data.identifier = strndup(name, strlen(name));
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
    node->type = AST_BINARY_OP;
    node->data.binary_op.left = left;
    node->data.binary_op.op = op;
    node->data.binary_op.right = right;
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
    node->type = AST_BINARY_OP;
    node->data.binary_op.left = left;
    node->data.binary_op.op = op;
    node->data.binary_op.right = right;
    return node;
}


ASTNode* parse_if_statement(Token** tokens) {
    // Expect 'if'

    if ((*tokens)->type != IF) {
        fprintf(stderr, "Error: Expected 'if'.\n");
        exit(EXIT_FAILURE);
    }
    (*tokens)++;
    // printf("if %s\n", (*tokens)->value);
    // Expect '('
    if ( (*tokens)->type != LPAREN) {
        fprintf(stderr, "Error: Expected '('.\n");
        exit(EXIT_FAILURE);
    }
    (*tokens)++;

    // Parse condition
    ASTNode* condition = parse_expression(tokens);
    (*tokens)++;
    // printf(">> %s\n", (*tokens)->value);


    // Expect ')'
    // if ((*tokens)->type != RPAREN) {
    //     fprintf(stderr, "Error: Expected .')'.\n");
    //     exit(EXIT_FAILURE);
    // }
    // (*tokens)++;
    // printf("if %s\n", (*tokens)->value);

    // Expect '{'
    if ((*tokens)->type!= LBRACE) {
        fprintf(stderr, "Error: Expected '{'.\n");
        exit(EXIT_FAILURE);
    }

    // Parse then branch
    ASTNode* then_branch = parse_block(tokens);

    // Expect '}'
    // if ((*tokens)->type != RBRACE) {
    //     fprintf(stderr, "Error: Expected .'}'.\n");
    //     exit(EXIT_FAILURE);
    // }

    // Parse else or else if (if any)
    ASTNode* else_branch = NULL;
    (*tokens)++;
    if ((*tokens)->type == ELSE) {
        (*tokens)++;
        if ((*tokens)->type == IF) {
            else_branch = parse_if_statement(tokens);
        } else if ((*tokens)->type == LBRACE) {
            else_branch = parse_block(tokens);
            // if ((*tokens)->type != RBRACE) {
            //     fprintf(stderr, "Error: Expected '}'.\n");
            //     exit(EXIT_FAILURE);
            // }
        } else {
            fprintf(stderr, "Error: Expected 'if' or '{' after 'else'.\n");
            exit(EXIT_FAILURE);
        }
    }

    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_IF_STATEMENT;
    node->data.if_statement.condition = condition;
    node->data.if_statement.then_branch = then_branch;
    node->data.if_statement.else_branch = else_branch;

    return node;
}
// Parse primary expressions (numbers, parentheses, unary operators)
ASTNode* parse_primary(Token** tokens) {
    Token* token = *tokens;

    if (token->type == NUMBER) {
        int value = strtod(token->value, NULL);
        *tokens += 1; // Consume the number
        return create_number_node(value);
    }
    else if (token->type == FLOAT) {
        double value = strtod(token->value, NULL);
        *tokens += 1; // Consume the number
        return create_float_node(value);
    }
    else if (token->type == STRING) {
        char* value = token->value;
        *tokens += 1; // Consume the string
        return create_string_node(value);
    }
    else if (token->type == IF) {
        // printf("Parsing if statement\n");
        return parse_if_statement(tokens);
    }
    else if (token->type == RETURN) {
        *tokens += 1; // Consume'return'
        // printf("Parsing return statement\n");
        // printf("Parsing return value%s\n",(*tokens)->value);
        ASTNode* return_value = parse_expression(tokens);
        ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
        node->type = AST_RETURN;
        node->data.return_value = return_value;

        return node;
    }
    
    else if (token->type == LPAREN) {
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
        if ((*tokens + 1)->type == LPAREN) {
            *tokens += 1; // Consume '('
            // *tokens += 1; // Consume '('
            // printf("identifier: %s\n",(*tokens)->value);
            ASTNode** arguments = NULL;
            int num_args = 0;
            
            if (((*tokens)++)->type != RPAREN) {
                do {
                    if ((*tokens)->type  == RPAREN) {
                        break;
                        
                    }
                    arguments = realloc(arguments, sizeof(ASTNode*) * (num_args + 1));
                    arguments[num_args++] = parse_expression(tokens);
                    // *tokens += 1; // Consume '('

                } while (((*tokens)++)->type == COMMA);
                // printf("num_args: %d - %s\n",num_args,(*tokens)->value);
                
                // if ((*tokens)->type != RPAREN) {
                //     fprintf(stderr, "Error: Expected ')' after argument list.\n");
                //     exit(EXIT_FAILURE);
                // }
            }
            *tokens += 1; // Consume ')'
           
            // printf("num_args %s\n",token->value);
            ASTNode* node = malloc(sizeof(ASTNode));
            node->type = AST_FUNCTION_CALL;
            node->data.function_call.function_name = token->value;
            node->data.function_call.arguments = arguments;
            node->data.function_call.arg_count = num_args;
            return node;
   

        }
        *tokens += 1; // Consume the identifier
        
        return create_identifier_node(token->value);
    }
    else if (token->type == PRINT) {
        if ((*tokens + 1)->type == LPAREN) {
            *tokens += 1; // Consume '('
            // *tokens += 1; // Consume '('
            // printf("identifier: %s\n", token->value);
            ASTNode** arguments = NULL;
            int num_args = 0;
            
            if (((*tokens)++)->type != RPAREN) {
                do {
                    // printf(">>argument: %s\n",(*tokens)->value);
                    arguments = realloc(arguments, sizeof(ASTNode*) * (num_args + 1));
                    arguments[num_args++] = parse_expression(tokens);
                    // *tokens += 1; // Consume '('

                } while (((*tokens)++)->type == COMMA);
                // printf("num_args: %d - %s\n",num_args,(*tokens)->value);
                
                // if ((*tokens)->type != RPAREN) {
                //     fprintf(stderr, "Error: Expected ')' after argument list.\n");
                //     exit(EXIT_FAILURE);
                // }
            }
            // printf("num_args %s\n",token->value);
            ASTNode* node = malloc(sizeof(ASTNode));
            node->type = AST_PRINT;
            node->data.function_call.function_name = token->value;
            node->data.function_call.arguments = arguments;
            node->data.function_call.arg_count = num_args;
            return node;
   

        }
        // *tokens += 1; // Consume the identifier
        
        // return create_identifier_node(token->value);
    }
    else if (token->type == LBRACE) {
        *tokens += 1; // Consume '('
        
        ASTNode* node = parse_program(tokens);
        if ((*tokens)->type != RBRACE) {
            fprintf(stderr, "Error: Expected '}'\n");
            exit(1);
        }
        *tokens += 1; // Consume ')'
        return node;
        }
    
    
   

    fprintf(stderr, "Error: Unexpected token '%s'\n", token->value);
    exit(0);
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

//    (*tokens)->type == ADD_ASSIGN || (*tokens)->type == SUBTRACT_ASSIGN || (*tokens)->type == MULTIPLY_ASSIGN || (*tokens)->type == DIVIDE_ASSIGN || (*tokens)->type == MODULUS_ASSIGN || (*tokens)->type == BITWISE_AND_ASSIGN || (*tokens)->type == BITWISE_OR_ASSIGN || (*tokens)->type == BITWISE_XOR_ASSIGN || (*tokens)->type == SHIFT_LEFT_ASSIGN || (*tokens)->type == SHIFT_RIGHT_ASSIGN
    while ((*tokens)->type == ASSIGN || ((*tokens)->type == PLUS_ASSIGN || (*tokens)->type == MINUS_ASSIGN || (*tokens)->type == MULTIPLY_ASSIGN || (*tokens)->type == DIVIDE_ASSIGN || (*tokens)->type == MODULUS_ASSIGN ))
    {
        if ((*tokens +1)->type == LPAREN) {
            // printf("function definition\n");
            (*tokens)++; // Consume '='
            if ((*tokens)->type != LPAREN) {
                    fprintf(stderr, "Error: Expected identifier in parameter list.\n");
                    exit(1);
                }
            (*tokens)++;
            // printf("param: %s\n", (*tokens)->value);

            char** parameters = NULL;
            int param_count = 0;
            while ((*tokens)->type != RPAREN) {
                if ((*tokens)->type == COMMA) {
                    (*tokens)++;
                    
                }
                if ((*tokens)->type != IDENTIFIER) {
                    fprintf(stderr, "Error: Expected identifier in parameter list.\n");
                    exit(1);
                }
                char* param = (*tokens)->value;
                parameters = realloc(parameters, (param_count + 1) * sizeof(char*));
                parameters[param_count] = param;
                param_count++;
                (*tokens)++;
            }

            
            (*tokens)++;
            // (*tokens)++;
           

            // printf("param count: %s\n", (*tokens)->value);
            
            ASTNode* body = parse_block(tokens);

            
            // printf("param count: %d\n", param_count);
            // printf("param: %s\n", (*tokens)->value);
            ASTNode* node = malloc(sizeof(ASTNode));
            node->type = AST_FUNCTION_DEF;
            node->data.function_def.name = left->data.identifier;
            node->data.function_def.param_count = param_count;
            node->data.function_def.parameters = parameters;
            node->data.function_def.body = body;
            // printf("%s", node->data.identifier);
            return node;
            
        }
        
        Token* token = *tokens;
        (*tokens)++;
        ASTNode* right = parse_expression(tokens);
        // printf("left: %s, right: %d\n", left->data.identifier, right->data.number);
        left = create_assignment_node(left, token->type, right);
        return left;
        
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
// ASTNode* parse_if_statement(Token** tokens) {
    


ASTNode* parse_block(Token** tokens) {
    // Expect '{'
    if ((*tokens)->type != LBRACE) {
        fprintf(stderr, "Error: Expected '{'\n");
        return NULL;
    }
    (*tokens)++;

    // Parse statements inside the block
    ASTNode** statements = NULL;
    int statement_count = 0;
    while ((*tokens)->type != RBRACE) {
        statements = realloc(statements, sizeof(ASTNode*) * (statement_count + 1));
        statements[statement_count] = parse_statement(tokens);
        statement_count++;
    }

    // Expect '}'
    if ((*tokens)->type != RBRACE) {
        fprintf(stderr, "Error: Expected '}'\n");
        return NULL;
    }
    (*tokens)++;

    ASTNode* statement_list =  malloc(sizeof(ASTNode));
    statement_list->type = AST_STATEMENT_LIST;
    statement_list->data.statement_list.statements = statements;
    statement_list->data.statement_list.statement_count = statement_count;

    return statement_list;
}


ASTNode* parse_statement(Token** tokens) {
    ASTNode* node = parse_expression(tokens);
    if ((*tokens)->type == SEMICOLON  ) {
        *tokens += 1; // Consume ';'
        
        
    }
    return node;
}

ASTNode* parse_statememt_list(Token** tokens) {
    // size_t length = strlen(tokens);
     ASTNode** statements = (ASTNode**)malloc(sizeof(ASTNode*) * 100);
    // ASTNode* node = parse_statement(tokens);
    
    int count =0;
    while ((*tokens)->type != TOKEN_EOF) {
        statements[count++] = parse_statement(tokens);    

    }
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }
    node->type = AST_STATEMENT_LIST;
    node->data.statement_list.statements = statements;
    node->data.statement_list.statement_count = count;
    return node;

}



ASTNode* parse_program(Token** tokens) {
    ASTNode* node = parse_statememt_list(tokens);

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
    } 
    free(node);
}