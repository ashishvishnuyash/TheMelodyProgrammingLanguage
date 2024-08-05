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
        //comment #
        if (input[i] == '#') {
            size_t start = ++i;
            while (input[i] != '\n' && i < length) i++;
            if (i == length) {
                fprintf(stderr, "Error: Unterminated comment\n");
                free_tokens(tokens);
                exit(EXIT_FAILURE);
            }
            continue;
        }
        //comment multi line comment
        if (input[i] == '/' && input[i + 1] == '*') {
            size_t start = ++i;
            while (input[i] != '*' || input[i + 1] != '/') {
                i++;
                if (i == length) {
                    fprintf(stderr, "Error: Unterminated multi-line comment\n");
                    free_tokens(tokens);
                    exit(EXIT_FAILURE);
                }
            }
            i += 2;
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
            // printf("%s\n",  strndup(input + start, i - start));
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

           if (strncmp(input + start, "return", 2) == 0) {
            tokens[token_count].type = RETURN;
            tokens[token_count].value = strndup(input + start, i - start);
            token_count++;
            continue;

           } else if (strncmp(input + start, "if", 2) == 0) {
               tokens[token_count].type = IF;
               tokens[token_count].value = strndup(input + start, i - start);
               token_count++;
            continue;
           } else if (strncmp(input + start, "else", 4) == 0) {
               tokens[token_count].type = ELSE;               
               tokens[token_count].value = strndup(input + start, i - start);
               token_count++;
            continue;
           }else if (strncmp(input + start, "class", 5) == 0) {
               tokens[token_count].type = CLASS;
               tokens[token_count].value = strndup(input + start, i - start);
               token_count++;
            continue;}
            else if (strncmp(input + start, "this", 4) == 0) {
            tokens[token_count].type = THIS;
            tokens[token_count].value = strndup(input + start, i - start);
            token_count++;
            continue;
           }
           else if (strncmp(input + start, "del", 3) == 0) {
            tokens[token_count].type = DELETE;
            tokens[token_count].value = strndup(input + start, i - start);
            token_count++;
            continue;
           }
           
           else if (strncmp(input + start, "import", 6) == 0) {
            tokens[token_count].type = IMPORT;
            tokens[token_count].value = strndup(input + start, i - start);
            token_count++;
            continue;
           }
           else if (strncmp(input + start, "from", 4) == 0) {
            tokens[token_count].type = FROM;
            tokens[token_count].value = strndup(input + start, i - start);
            token_count++;
            continue;
           }
           else if (strncmp(input + start, "new", 3) == 0) {
            tokens[token_count].type = NEW;
            tokens[token_count].value = strndup(input + start, i - start);
            token_count++;
            continue;
           }
           else if (strncmp(input + start, "while", 5) == 0) {
            tokens[token_count].type = WHILE;
            tokens[token_count].value = strndup(input + start, i - start);
            token_count++;
            continue;
           }
           else if (strncmp(input + start, "for", 3) == 0) {
            tokens[token_count].type = FOR;
            tokens[token_count].value = strndup(input + start, i - start);
            token_count++;
            continue;
           }
           else if (strncmp(input + start, "print", 5) == 0) {
            tokens[token_count].type = PRINT;
            tokens[token_count].value = strndup(input + start, i - start);
            token_count++;
            continue;
           }
           else if(strncmp(input + start, "fopen", 5)==0){
            tokens[token_count].type = FOPEN;
            tokens[token_count].value = strndup(input + start, i - start);
            token_count++;
            continue;
           }
           else if(strncmp(input + start, "fclose", 6)==0){
            tokens[token_count].type = FCLOSE;
            tokens[token_count].value = strndup(input + start, i - start);
            token_count++;
            continue;
           }
           else if(strncmp(input + start, "fwrite", 6)==0){
            tokens[token_count].type = FWRITE;
            tokens[token_count].value = strndup(input + start, i - start);
            token_count++;
            continue;
           }
           else if(strncmp(input + start, "fread", 5)==0){
            tokens[token_count].type = FREAD;
            tokens[token_count].value = strndup(input + start, i - start);
            token_count++;
            continue;
           }
           else if(strncmp(input + start, "scan", 4)==0){
            tokens[token_count].type = SCAN;
            tokens[token_count].value = strndup(input + start, i - start);
            token_count++;
            continue;
           }
           else if(strncmp(input + start, "import", 6)==0){
            // printf("import\n");
            tokens[token_count].type = IMPORT;
            tokens[token_count].value = strndup(input + start, i - start);
            token_count++;
            continue;
           }
           


           else{
            
            tokens[token_count].type = IDENTIFIER;
            tokens[token_count].value = strndup(input + start, i - start);
            token_count++;
            continue;
           }
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
            case '.':
                tokens[token_count].type = DOT;
                tokens[token_count].value = strndup(input + i, 1);
                i++;
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
            case ';':
                tokens[token_count].type = SEMICOLON;
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
