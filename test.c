#include <stdio.h>
#include <stdlib.h>

void* data(void* arg) {
    void* result = NULL;
    result = malloc(sizeof(int));
    *((int*)result) =(*(int*)arg)+1;
    return result;

}
int main(int argc, char const *argv[]) {
   int i =5;
   void* ptr = data(&i);

  // Check if
  if ((*tokens+1)->type == LPAREN) {
            (*tokens)++;
             if ((*tokens)->type != LPAREN) {
                fprintf(stderr, "Error: Expected identifier in parameter list.\n");
                exit(1);
                }

            (*tokens)++;
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
            if ((*tokens)->type != LBRACE) {
                fprintf(stderr, "Error: Expected '{' after parameter list.\n");
                exit(1);
            }

            (*tokens)++;
            Token** token_body = NULL;
            int token_count = 0;
            while ((*tokens)->type != RBRACE) {
                token_body = realloc(token_body, (token_count + 1) * sizeof(Token));
                token_body[token_count] = *tokens;
                token_count++;
                
                (*tokens)++;
            }
            for (int i = 0; i < token_count; i++) {
                printf("%s\n", token_body[i]->value);
            }
        //     if ((*tokens)->type != RBRACE) {
        //         fprintf(stderr, "Error: Expected ')' after function body.\n");
        //         exit(1);
        //     }

            (*tokens)++;
        //     (*tokens)++;
        //    printf("%s",(*tokens)->value);



            ASTNode *body = parse_primary(token_body);

            

            ASTNode* node = malloc(sizeof(ASTNode));
            node->type = AST_FUNCTION_DEF;
            node->data.function_def.name = left->data.identifier;
            node->data.function_def.param_count = param_count;
            node->data.function_def.parameters = parameters;
            node->data.function_def.body = body;
            return node;
                
        }
         data function returned a valid pointer
  if (ptr == NULL) {
    fprintf(stderr, "Error: data() failed\n");
    return 1; // Exit with an error code
  }

  // Access the stored integer value safely
  int value = *((int*)ptr);

  printf("Value: %d\n", value);

  // Free the allocated memory (important to prevent memory leaks)
  free(ptr);

  return 0;
}