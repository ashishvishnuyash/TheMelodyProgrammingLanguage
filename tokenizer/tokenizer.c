#include "tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>


char *strndup(const char *s, size_t n) {
    size_t len = strnlen(s, n);
    char *new_s = malloc(len + 1);
    if (new_s) {
        memcpy(new_s, s, len);
        new_s[len] = '\0';
    }
    return new_s;
}

Token* tokenize(const char* input) {
    int length = strlen(input);
    
    Token* tokens = (Token*)malloc((length + 1) * sizeof(Token));
    if (tokens == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }

    int token_count = 0;
    int i = 0;

    while (i < length) {
        // Skip whitespace
        while (isspace(input[i])) {
            i++;
        }

        // Check for end of input
        if (i >= length) {
            break;
        }
        // Check for a number
        
        // print input

        if (input[i]=='+'){
            tokens[token_count].type = PLUS;
            tokens[token_count].value = (char*)malloc(2);
            tokens[token_count].value[0] = input[i];
            tokens[token_count].value[1] = '\0';
            token_count++;
            i++;
            continue;
        }
        if (input[i]=='-'){
            tokens[token_count].type = MINUS;
            tokens[token_count].value = (char*)malloc(2);
            tokens[token_count].value[0] = input[i];
            tokens[token_count].value[1] = '\0';
            token_count++;
            i++;
            continue;
        }
        if (input[i]=='*'){
            tokens[token_count].type = MULTIPLY;
            tokens[token_count].value = (char*)malloc(2);
            tokens[token_count].value[0] = input[i];
            tokens[token_count].value[1] = '\0';
            token_count++;
            i++;
            continue;
        }
        if (input[i]=='/'){
            tokens[token_count].type = DIVIDE;
            tokens[token_count].value = (char*)malloc(2);
            tokens[token_count].value[0] = input[i];
            tokens[token_count].value[1] = '\0';
            token_count++;
            i++;
            continue;
        }
        if (input[i] == '(') {
            tokens[token_count].type = LPAREN;
            tokens[token_count].value = (char*)malloc(2);
            tokens[token_count].value[0] = input[i];
            tokens[token_count].value[1] = '\0';
            token_count++;
            i++;
            continue;
        }
        if (input[i] == ')') {
            tokens[token_count].type = RPAREN;
            tokens[token_count].value = (char*)malloc(2);
            tokens[token_count].value[0] = input[i];
            tokens[token_count].value[1] = '\0';
            token_count++;
            i++;
            continue;
        }

        if (isdigit(input[i])) {
            int start = i;
            while (isdigit(input[i])) {
                i++;
            }
            int length = i - start;
            tokens[token_count].type = NUMBER;
            tokens[token_count].value = strndup(input + start, length);
            token_count++;
            
            continue;
        }


       printf("%c",input[i]);
       i++;
        
    }

    // Add EOF token
    tokens[token_count].type = TOKEN_EOF;
    tokens[token_count].value = NULL;
    token_count++;

    return tokens;
}

void free_tokens(Token* tokens) {
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        free(tokens[i].value);
    }
    free(tokens);
}
