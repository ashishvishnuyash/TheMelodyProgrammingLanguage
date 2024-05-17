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
        printf("%d", node->data.number);
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
   ASTNode* ast = parse_expression(&token_ptr);
    int show =interpret(ast);
    printf("Interpreted result: %d\n", show);
   
    if (ast != NULL) {
        printf("Parsed AST: ");
        print_ast(ast);
        printf("\n");
        free_ast(ast);
    } else {
        fprintf(stderr, "Error: Parsing failed\n");
    }


   // Print the tokens
   for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
    printf("Token type: %d, Value: %s\n", tokens[i].type, tokens[i].value);
    }
    free_tokens(tokens);
    // Print the tokens
    // printf("%s",content);
    free_file_content(content);
    // print token
   
    
    return 0;
}
