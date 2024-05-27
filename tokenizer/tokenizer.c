#include "tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Utility function to duplicate a string with a specified length
char *strndup(const char *s, size_t n) {
    char *result = (char *)malloc(n + 1);
    if (result == NULL) return NULL;
    result[n] = '\0';
    return (char *)memcpy(result, s, n);
}

// Tokenize the input string into an array of tokens
Token* tokenize(const char* input) {
    size_t length = strlen(input);
    Token* tokens = (Token*)malloc(sizeof(Token) * (length + 1));
    if (!tokens) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    int token_count = 0;
    for (size_t i = 0; i < length;) {
        if (isspace(input[i])) {
            i++;
            continue;
        }

        if (isdigit(input[i]) || (input[i] == '.' && isdigit(input[i + 1]))) {
            size_t start = i;
            int is_float = 0;
            int is_complex = 0;
            while (isdigit(input[i]) || input[i] == '.' || input[i] == 'j' || input[i] == 'J') {
                if (input[i] == '.') {
                    is_float = 1;
                }
                if (input[i] == 'j' || input[i] == 'J') {
                    is_complex = 1;
                    is_float = 0; // complex number has priority
                    i++;
                    break;
                }
                i++;
            }
            tokens[token_count].type = is_complex ? COMPLEX : (is_float ? FLOAT : NUMBER);
            tokens[token_count].value = strndup(input + start, i - start);
            token_count++;
            continue;
        }

        if (input[i] == '\'') {
            size_t start = ++i;
            if (input[i + 1] == '\'') {
                tokens[token_count].type = CHAR;
                tokens[token_count].value = strndup(input + start, 1);
                i += 2;
            } else {
                fprintf(stderr, "Error: Invalid character literal\n");
                free_tokens(tokens);
                exit(EXIT_FAILURE);
            }
            token_count++;
            continue;
        }

        if (input[i] == '"') {
            size_t start = ++i;
            while (input[i] != '"' && i < length) i++;
            if (i == length) {
                fprintf(stderr, "Error: Unterminated string literal\n");
                free_tokens(tokens);
                exit(EXIT_FAILURE);
            }
            tokens[token_count].type = STRING;
            tokens[token_count].value = strndup(input + start, i - start);
            i++;
            token_count++;
            continue;
        }

        if (strncmp(input + i, "true", 4) == 0 || strncmp(input + i, "false", 5) == 0) {
            tokens[token_count].type = BOOLEAN;
            size_t bool_len = (input[i] == 't') ? 4 : 5;
            tokens[token_count].value = strndup(input + i, bool_len);
            i += bool_len;
            token_count++;
            continue;
        }

        if (isalpha(input[i]) || input[i] == '_') {
            size_t start = i;
            while (isalnum(input[i]) || input[i] == '_') i++;
            tokens[token_count].type = IDENTIFIER;
            tokens[token_count].value = strndup(input + start, i - start);
            token_count++;
            continue;
        }

        switch (input[i]) {
            case '+':
                if (input[i + 1] == '+') {
                    tokens[token_count].type = INCREMENT;
                    tokens[token_count].value = strndup(input + i, 2);
                    i += 2;
                } else if (input[i + 1] == '=') {
                    tokens[token_count].type = PLUS_ASSIGN;
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
                } else if (input[i + 1] == '=') {
                    tokens[token_count].type = MINUS_ASSIGN;
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
                } else if (input[i + 1] == '=') {
                    tokens[token_count].type = MULTIPLY_ASSIGN;
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
                } else if (input[i + 1] == '=') {
                    tokens[token_count].type = DIVIDE_ASSIGN;
                    tokens[token_count].value = strndup(input + i, 2);
                    i += 2;
                } else {
                    tokens[token_count].type = DIVIDE;
                    tokens[token_count].value = strndup(input + i, 1);
                    i++;
                }
                break;
            case '%':
                if (input[i + 1] == '=') {
                    tokens[token_count].type = MODULUS_ASSIGN;
                    tokens[token_count].value = strndup(input + i, 2);
                    i += 2;
                } else {
                    tokens[token_count].type = MODULUS;
                    tokens[token_count].value = strndup(input + i, 1);
                    i++;
                }
                break;
            case '=':
                if (input[i + 1] == '=') {
                    tokens[token_count].type = EQUAL;
                    tokens[token_count].value = strndup(input + i, 2);
                    i += 2;
                } else {
                    tokens[token_count].type = ASSIGN;
                    tokens[token_count].value = strndup(input + i, 1);
                    i++;
                }
                break;
            case '!':
                if (input[i + 1] == '=') {
                    tokens[token_count].type = NOT_EQUAL;
                    tokens[token_count].value = strndup(input + i, 2);
                    i += 2;
                } else {
                    tokens[token_count].type = LOGICAL_NOT;
                    tokens[token_count].value = strndup(input + i, 1);
                    i++;
                }
                break;
            case '&':
                if (input[i + 1] == '&') {
                    tokens[token_count].type = LOGICAL_AND;
                    tokens[token_count].value = strndup(input + i, 2);
                    i += 2;
                } else {
                    tokens[token_count].type = BITWISE_AND;
                    tokens[token_count].value = strndup(input + i, 1);
                    i++;
                }
                break;
            case '|':
                if (input[i + 1] == '|') {
                    tokens[token_count].type = LOGICAL_OR;
                    tokens[token_count].value = strndup(input + i, 2);
                    i += 2;
                } else {
                    tokens[token_count].type = BITWISE_OR;
                    tokens[token_count].value = strndup(input + i, 1);
                    i++;
                }
                break;
            case '^':
                tokens[token_count].type = BITWISE_XOR;
                tokens[token_count].value = strndup(input + i, 1);
                i++;
                break;
            case '~':
                tokens[token_count].type = BITWISE_NOT;
                tokens[token_count].value = strndup(input + i, 1);
                i++;
                break;
            case '<':
                if (input[i + 1] == '<') {
                    tokens[token_count].type = SHIFT_LEFT;
                    tokens[token_count].value = strndup(input + i, 2);
                    i += 2;
                } else if (input[i + 1] == '=') {
                    tokens[token_count].type = LESS_EQUAL;
                    tokens[token_count].value = strndup(input + i, 2);
                    i += 2;
                } else {
                    tokens[token_count].type = LESS;
                    tokens[token_count].value = strndup(input + i, 1);
                    i++;
                }
                break;
            case '>':
                if (input[i + 1] == '>') {
                    tokens[token_count].type = SHIFT_RIGHT;
                    tokens[token_count].value = strndup(input + i, 2);
                    i += 2;
                } else if (input[i + 1] == '=') {
                    tokens[token_count].type = GREATER_EQUAL;
                    tokens[token_count].value = strndup(input + i, 2);
                    i += 2;
                } else {
                    tokens[token_count].type = GREATER;
                    tokens[token_count].value = strndup(input + i, 1);
                    i++;
                }
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
            case '[':
                tokens[token_count].type = LBRACKET;
                tokens[token_count].value = strndup(input + i, 1);
                i++;
                break;
            case ']':
                tokens[token_count].type = RBRACKET;
                tokens[token_count].value = strndup(input + i, 1);
                i++;
                break;
            case '{':
                tokens[token_count].type = LBRACE;
                tokens[token_count].value = strndup(input + i, 1);
                i++;
                break;
            case '}':
                tokens[token_count].type = RBRACE;
                tokens[token_count].value = strndup(input + i, 1);
                i++;
                break;
            case ':':
                tokens[token_count].type = COLON;
                tokens[token_count].value = strndup(input + i, 1);
                i++;
                break;
            case ',':
                tokens[token_count].type = COMMA;
                tokens[token_count].value = strndup(input + i, 1);
                i++;
                break;
            default:
                fprintf(stderr, "Error: Unrecognized character '%c'\n", input[i]);
                free_tokens(tokens);
                exit(EXIT_FAILURE);
        }
        token_count++;
    }
    tokens[token_count].type = TOKEN_EOF;
    tokens[token_count].value = NULL;
    return tokens;
}

// Free the memory allocated for the tokens
void free_tokens(Token* tokens) {
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        free(tokens[i].value);
    }
    free(tokens);
}
