#include "tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Function to duplicate a string with a specified length
char *strndup(const char *s, size_t n) {
    char *result = (char *)malloc(n + 1);
    if (result == NULL) {
        return NULL;
    }
    strncpy(result, s, n);
    result[n] = '\0';
    return result;
}

// Function to tokenize a string
Token* tokenize(const char* input) {
    size_t length = strlen(input);
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

        // Handle numbers (integers and floats)
        if (isdigit(input[i]) || (input[i] == '.' && isdigit(input[i+1]))) {
            int start = i;
            while (isdigit(input[i])) {
                i++;
            }
            if (input[i] == '.') {
                i++;
                while (isdigit(input[i])) {
                    i++;
                }
            }
            int len = i - start;
            tokens[token_count].type = NUMBER;
            tokens[token_count].value = strndup(input + start, len);
            token_count++;
            continue;
        }

        // Handle operators and parentheses
        switch (input[i]) {
            case '+':
                if (input[i + 1] == '+') {
                    tokens[token_count].type = INCREMENT;
                    tokens[token_count].value = strndup(input + i, 2);
                    i += 2;
                } else {
                    tokens[token_count].type = PLUS;
                    tokens[token_count].value = strndup(input + i, 1);
                    i++;
                }
                break;
            case '-':
                if (input[i + 1] == '-') {
                    tokens[token_count].type = DECREMENT;
                    tokens[token_count].value = strndup(input + i, 2);
                    i += 2;
                } else {
                    tokens[token_count].type = MINUS;
                    tokens[token_count].value = strndup(input + i, 1);
                    i++;
                }
                break;
            case '*':
                if (input[i + 1] == '*') {
                    tokens[token_count].type = EXPONENTIATION;
                    tokens[token_count].value = strndup(input + i, 2);
                    i += 2;
                } else {
                    tokens[token_count].type = MULTIPLY;
                    tokens[token_count].value = strndup(input + i, 1);
                    i++;
                }
                break;
            case '/':
                if (input[i + 1] == '/') {
                    tokens[token_count].type = FLOOR_DIVISION;
                    tokens[token_count].value = strndup(input + i, 2);
                    i += 2;
                } else {
                    tokens[token_count].type = DIVIDE;
                    tokens[token_count].value = strndup(input + i, 1);
                    i++;
                }
                break;
            case '%':
                tokens[token_count].type = MODULUS;
                tokens[token_count].value = strndup(input + i, 1);
                i++;
                break;
            case '(':
                tokens[token_count].type = LPAREN;
                tokens[token_count].value = strndup(input + i, 1);
                i++;
                break;
            case ')':
                tokens[token_count].type = RPAREN;
                tokens[token_count].value = strndup(input + i, 1);
                i++;
                break;
            case '=':
                if (input[i + 1] == '=') {
                    tokens[token_count].type = EQUAL;
                    tokens[token_count].value = strndup(input + i, 2);
                    i += 2;
                } else {
                    fprintf(stderr, "Error: Unknown token '%c'\n", input[i]);
                    i++;
                }
                break;
            case '!':
                if (input[i + 1] == '=') {
                    tokens[token_count].type = NOT_EQUAL;
                    tokens[token_count].value = strndup(input + i, 2);
                    i += 2;
                } else {
                    fprintf(stderr, "Error: Unknown token '%c'\n", input[i]);
                    i++;
                }
                break;
            case '>':
                if (input[i + 1] == '=') {
                    tokens[token_count].type = GREATER_EQUAL;
                    tokens[token_count].value = strndup(input + i, 2);
                    i += 2;
                } else {
                    tokens[token_count].type = GREATER;
                    tokens[token_count].value = strndup(input + i, 1);
                    i++;
                }
                break;
            case '<':
                if (input[i + 1] == '=') {
                    tokens[token_count].type = LESS_EQUAL;
                    tokens[token_count].value = strndup(input + i, 2);
                    i += 2;
                } else {
                    tokens[token_count].type = LESS;
                    tokens[token_count].value = strndup(input + i, 1);
                    i++;
                }
                break;
            default:
                fprintf(stderr, "Unknown token: %c\n", input[i]);
                i++;
                continue;
        }
        token_count++;
    }

    // Add EOF token
    tokens[token_count].type = TOKEN_EOF;
    tokens[token_count].value = NULL;
    token_count++;

    return tokens;
}

// Function to free memory allocated for tokens
void free_tokens(Token* tokens) {
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        free(tokens[i].value);
    }
    free(tokens);
}
