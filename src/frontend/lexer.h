#ifndef MELODY_LEXER_H
#define MELODY_LEXER_H

typedef enum {
    /* Literals */
    TOK_INT,
    TOK_FLOAT,
    TOK_STRING,
    TOK_IDENTIFIER,

    /* Keywords */
    TOK_TRUE,
    TOK_FALSE,
    TOK_NULL,
    TOK_IF,
    TOK_ELSE,
    TOK_ELIF,
    TOK_WHILE,
    TOK_FOR,
    TOK_IN,
    TOK_BREAK,
    TOK_CONTINUE,
    TOK_RETURN,
    TOK_FN,
    TOK_IMPORT,
    TOK_DEL,
    TOK_TRY,
    TOK_CATCH,
    TOK_THROW,
    TOK_CLASS,
    TOK_EXTENDS,
    TOK_SUPER,

    /* Operators */
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,
    TOK_POWER,       /* ** */
    TOK_FLOOR_DIV,   /* // */
    TOK_BANG,         /* ! */
    TOK_DOUBLE_AMP,  /* && */
    TOK_DOUBLE_PIPE,  /* || */
    TOK_AMPERSAND,   /* & */
    TOK_PIPE,        /* | */
    TOK_CARET,       /* ^ */
    TOK_TILDE,       /* ~ */
    TOK_SHL,         /* << */
    TOK_SHR,         /* >> */

    /* Comparison */
    TOK_EQ,          /* == */
    TOK_NEQ,         /* != */
    TOK_LT,          /* < */
    TOK_GT,          /* > */
    TOK_LTE,         /* <= */
    TOK_GTE,         /* >= */

    /* Assignment */
    TOK_ASSIGN,      /* = */
    TOK_PLUS_EQ,     /* += */
    TOK_MINUS_EQ,    /* -= */
    TOK_STAR_EQ,     /* *= */
    TOK_SLASH_EQ,    /* /= */
    TOK_PERCENT_EQ,  /* %= */

    /* Delimiters */
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_COMMA,
    TOK_DOT,
    TOK_COLON,
    TOK_SEMICOLON,

    /* Special */
    TOK_EOF,
    TOK_ERROR,
} TokenType;

typedef struct {
    TokenType type;
    char* value;
    int line;
    int col;
} Token;

typedef struct {
    const char* source;
    int length;
    int pos;
    int line;
    int col;
    Token* tokens;
    int token_count;
    int token_capacity;
} Lexer;

Lexer* lexer_new(const char* source);
void lexer_free(Lexer* lexer);
Token* lexer_tokenize(Lexer* lexer);
int lexer_token_count(Lexer* lexer);
const char* token_type_name(TokenType type);

#endif
