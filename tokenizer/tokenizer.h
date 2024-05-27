#ifndef TOKENIZER_H
#define TOKENIZER_H

// Token types
typedef enum {
    NUMBER,
    CHAR,
    FLOAT,
    COMPLEX,
    STRING,
    BOOLEAN,
    LIST,
    TUPLE,
    DICT,
    SET,
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MODULUS,
    EXPONENTIATION,
    FLOOR_DIVISION,
    LPAREN,
    RPAREN,
    INCREMENT,
    DECREMENT,
    TOKEN_EOF,
    EQUAL,
    NOT_EQUAL,
    GREATER,
    LESS,
    GREATER_EQUAL,
    LESS_EQUAL,
    LOGICAL_AND,
    LOGICAL_OR,
    LOGICAL_NOT,
    BITWISE_AND,       // &
    BITWISE_OR,        // |
    BITWISE_XOR,       // ^
    BITWISE_NOT, 
    SHIFT_LEFT,        // <<
    SHIFT_RIGHT,
    IDENTIFIER,
    PLUS_ASSIGN,       // +=
    MINUS_ASSIGN,      // -=
    ASSIGN,
    MULTIPLY_ASSIGN,   // *=
    DIVIDE_ASSIGN,     // /=
    MODULUS_ASSIGN ,    // %=  
    LBRACKET,
    RBRACKET,
    LBRACE,
    RBRACE,
    COLON,
    SEMICOLON,
    COMMA,
    DOT,
    QUESTION_MARK,
    ARROW,
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
