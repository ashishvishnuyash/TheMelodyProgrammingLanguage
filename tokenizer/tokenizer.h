#ifndef TOKENIZER_H
#define TOKENIZER_H

// Token types
typedef enum {
    NUMBER,
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MODULUS,
    EXPONENTIATION,
    FLOOR_DIVISION,
    LPAREN,
    RPAREN,
    TOKEN_EOF,
    INCREMENT,
    DECREMENT,
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    char* value;
} Token;
char *strndup(const char *s, size_t n);

// Function to tokenize a string
Token* tokenize(const char* input);

// Function to free memory allocated for tokens
void free_tokens(Token* tokens);

#endif /* TOKENIZER_H */
