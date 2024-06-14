#include <stdio.h>
#include ".\includes\readfile.c"
#include ".\tokenizer\tokenizer.c"
#include ".\parser\parser.c"
#include ".\interpreter\interpreter.c"
void print_ast(ASTNode* node) {
    if (node == NULL) {
        return;
    }
    if (node->type == AST_NUMBER) {
        printf("%f", node->data.number);
    } else if (node->type == AST_BINARY_OP) {
        printf("(");
        print_ast(node->data.binary_op.left);
        switch (node->data.binary_op.op) {
            case PLUS: printf(" + "); break;
            case MINUS: printf(" - "); break;
            case MULTIPLY: printf(" * "); break;
            case DIVIDE: printf(" / "); break;
            default: printf(" ? "); break;
        }
        print_ast(node->data.binary_op.right);
        printf(")");
    } else if (node->type == AST_UNARY_OP) {
        printf("(");
        switch (node->data.unary_op.op) {
            case PLUS: printf("+"); break;
            case MINUS: printf("-"); break;
            default: printf("?"); break;
        }
        print_ast(node->data.unary_op.operand);
        printf(")");
    } else if (node->type == AST_IDENTIFIER) {
        printf("%s", node->data.identifier);
    } else if (node->type == AST_ASSIGNMENT) {
        print_ast(node->data.assignment_op.left);
        printf(" = " );
        print_ast(node->data.assignment_op.right);
    } 
}
int main(int argc, char const *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    char* content = read_file(argv[1]);
    if (content == NULL) {
        return 1;
    }
    Token* tokens = tokenize(content);
   if (tokens == NULL) {
    printf("Error: Failed to tokenize input.\n");
    return 1;
   }
   Token* token_ptr = tokens;
   ASTNode* ast = parse_program(&token_ptr);
    // double show =interpret(ast);
    // printf("Interpreted result: %f\n", show);
   
    interpret(ast);
    print_ast(ast);
    

    // int value = *((int*)result->value);

    // printf("Value: %d\n", value);
    // double valued = *((double*)result->value);

    // printf("Value: %f\n", valued);

   // Print the tokens
   for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
    printf("Token type: %d, Value: %s\n", tokens[i].type, tokens[i].value);
    }
    free_tokens(tokens);
    free_ast(ast);

    // Print the tokens
    // printf("%s",content);
    free_file_content(content);
    // print token
   
    
    return 0;
}