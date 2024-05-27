#include <stdio.h>
#include ".\includes\readfile.c"
#include ".\tokenizer\tokenizer.c"
#include ".\parser\parser.c"
#include ".\interpreter\interpreter.c"
void print_ast(ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case AST_NUMBER:
            printf("Number: %d\n", node->data.number);
            break;
        case AST_FLOAT:
            printf("Float: %f\n", node->data.float_value);
            break;
        case AST_STRING:
            printf("String: %s\n", node->data.string_value);
            break;
        case AST_BOOLEAN:
            printf("Boolean: %d\n", node->data.bool_value);
            break;
        case AST_BINARY_OP:
            printf("Binary Operation: %d\n", node->data.binary_op.op);
            print_ast(node->data.binary_op.left);
            print_ast(node->data.binary_op.right);
            break;
        case AST_UNARY_OP:
            printf("Unary Operation: %d\n", node->data.unary_op.op);
            print_ast(node->data.unary_op.operand);
            break;
        case AST_IDENTIFIER:
            printf("Variable: %s\n", node->data.identifier);
            break;
        case AST_ASSIGNMENT:
            printf("Assignment: %s = \n", node->data.assignment_op.left);
            print_ast(node->data.assignment_op.right);
            break;
        case AST_COMPARISON:
            printf("Comparison: %d\n", node->data.comparison.op);
            print_ast(node->data.comparison.left);
            print_ast(node->data.comparison.right);
            break;
        case AST_LOGICAL_OP:
            printf("Logical Operation: %d\n", node->data.logical_op.op);
            print_ast(node->data.logical_op.left);
            print_ast(node->data.logical_op.right);
            break;
        default:
            printf("Unknown AST node type: %d\n", node->type);
            break;
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
//     Token* tokens = tokenize(content);
//    if (tokens == NULL) {
//     printf("Error: Failed to tokenize input.\n");
//     return 1;
//    }
    ASTNode* ast = parse(content);
    if (!ast) {
        fprintf(stderr, "Error: Failed to parse input\n");
        return EXIT_FAILURE;
    }

    print_ast(ast);

    // void* result = evaluate_ast(ast);
    // if (result) {
    //     printf("Result: %d\n", *((int*)result));
    //     free(result);
    // }

    free_ast(ast);
    free_variables();

    return EXIT_SUCCESS;
}