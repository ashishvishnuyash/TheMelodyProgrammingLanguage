#include "lexer.h"
#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ---------- Keyword Table ---------- */

typedef struct {
    const char* word;
    int length;
    TokenType type;
} Keyword;

static Keyword keywords[] = {
    {"true",     4, TOK_TRUE},
    {"false",    5, TOK_FALSE},
    {"null",     4, TOK_NULL},
    {"if",       2, TOK_IF},
    {"else",     4, TOK_ELSE},
    {"elif",     4, TOK_ELIF},
    {"while",    5, TOK_WHILE},
    {"for",      3, TOK_FOR},
    {"in",       2, TOK_IN},
    {"break",    5, TOK_BREAK},
    {"continue", 8, TOK_CONTINUE},
    {"return",   6, TOK_RETURN},
    {"fn",       2, TOK_FN},
    {"import",   6, TOK_IMPORT},
    {"del",      3, TOK_DEL},
    {"and",      3, TOK_DOUBLE_AMP},   /* 'and' is alias for && */
    {"or",       2, TOK_DOUBLE_PIPE},  /* 'or' is alias for || */
    {"not",      3, TOK_BANG},         /* 'not' is alias for ! */
    {"try",      3, TOK_TRY},
    {"catch",    5, TOK_CATCH},
    {"throw",    5, TOK_THROW},
    {"class",    5, TOK_CLASS},
    {"extends",  7, TOK_EXTENDS},
    {"super",    5, TOK_SUPER},
    {NULL, 0, TOK_EOF},
};

/*
 * FIX for the critical keyword bug:
 * The old code used strncmp with wrong lengths, e.g. strncmp(s, "return", 2)
 * which matched any identifier starting with "re" (result, response, etc.).
 * We now match the FULL keyword length AND check word boundaries.
 */
static TokenType check_keyword(const char* word, int length) {
    for (int i = 0; keywords[i].word != NULL; i++) {
        if (keywords[i].length == length &&
            memcmp(keywords[i].word, word, length) == 0) {
            return keywords[i].type;
        }
    }
    return TOK_IDENTIFIER;
}

/* ---------- Lexer Helpers ---------- */

static void add_token(Lexer* lex, TokenType type, const char* value, int line, int col) {
    if (lex->token_count >= lex->token_capacity) {
        lex->token_capacity *= 2;
        lex->tokens = (Token*)realloc(lex->tokens, sizeof(Token) * lex->token_capacity);
    }
    Token* t = &lex->tokens[lex->token_count++];
    t->type = type;
    t->value = value ? melody_strdup(value) : NULL;
    t->line = line;
    t->col = col;
}

static void add_token_n(Lexer* lex, TokenType type, const char* start, int len, int line, int col) {
    if (lex->token_count >= lex->token_capacity) {
        lex->token_capacity *= 2;
        lex->tokens = (Token*)realloc(lex->tokens, sizeof(Token) * lex->token_capacity);
    }
    Token* t = &lex->tokens[lex->token_count++];
    t->type = type;
    t->value = melody_strndup(start, len);
    t->line = line;
    t->col = col;
}

static char peek(Lexer* lex) {
    if (lex->pos >= lex->length) return '\0';
    return lex->source[lex->pos];
}

static char peek_next(Lexer* lex) {
    if (lex->pos + 1 >= lex->length) return '\0';
    return lex->source[lex->pos + 1];
}

static char advance_char(Lexer* lex) {
    char c = lex->source[lex->pos++];
    if (c == '\n') { lex->line++; lex->col = 1; }
    else lex->col++;
    return c;
}

static void skip_whitespace(Lexer* lex) {
    while (lex->pos < lex->length) {
        char c = peek(lex);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance_char(lex);
        }
        /* # single-line comment */
        else if (c == '#') {
            while (lex->pos < lex->length && peek(lex) != '\n')
                advance_char(lex);
        }
        /* multi-line comment */
        else if (c == '/' && peek_next(lex) == '*') {
            advance_char(lex); advance_char(lex); /* skip / * */
            while (lex->pos < lex->length) {
                if (peek(lex) == '*' && peek_next(lex) == '/') {
                    advance_char(lex); advance_char(lex);
                    break;
                }
                advance_char(lex);
            }
        }
        else break;
    }
}

/* ---------- Lex Specific Token Types ---------- */

static void lex_number(Lexer* lex) {
    int start = lex->pos;
    int line = lex->line, col = lex->col;
    int is_float = 0;

    while (lex->pos < lex->length && isdigit(peek(lex)))
        advance_char(lex);

    if (peek(lex) == '.' && isdigit(peek_next(lex))) {
        is_float = 1;
        advance_char(lex); /* consume '.' */
        while (lex->pos < lex->length && isdigit(peek(lex)))
            advance_char(lex);
    }

    /* Scientific notation: 1e10, 2.5e-3 */
    if (peek(lex) == 'e' || peek(lex) == 'E') {
        is_float = 1;
        advance_char(lex);
        if (peek(lex) == '+' || peek(lex) == '-') advance_char(lex);
        while (lex->pos < lex->length && isdigit(peek(lex)))
            advance_char(lex);
    }

    int len = lex->pos - start;
    add_token_n(lex, is_float ? TOK_FLOAT : TOK_INT,
                lex->source + start, len, line, col);
}

static void lex_string(Lexer* lex) {
    int line = lex->line, col = lex->col;
    advance_char(lex); /* consume opening " */

    /* Build string with escape sequence processing */
    int cap = 64;
    char* buf = (char*)malloc(cap);
    int len = 0;

    while (lex->pos < lex->length && peek(lex) != '"') {
        char c = advance_char(lex);
        if (c == '\\' && lex->pos < lex->length) {
            char esc = advance_char(lex);
            switch (esc) {
                case 'n':  c = '\n'; break;
                case 't':  c = '\t'; break;
                case 'r':  c = '\r'; break;
                case '\\': c = '\\'; break;
                case '"':  c = '"';  break;
                case '0':  c = '\0'; break;
                default:   c = esc;  break;
            }
        }
        if (len + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
        buf[len++] = c;
    }

    if (lex->pos >= lex->length) {
        fprintf(stderr, "Error at line %d: Unterminated string literal\n", line);
        free(buf);
        exit(EXIT_FAILURE);
    }
    advance_char(lex); /* consume closing " */
    buf[len] = '\0';

    if (lex->token_count >= lex->token_capacity) {
        lex->token_capacity *= 2;
        lex->tokens = (Token*)realloc(lex->tokens, sizeof(Token) * lex->token_capacity);
    }
    Token* t = &lex->tokens[lex->token_count++];
    t->type = TOK_STRING;
    t->value = buf; /* transfer ownership */
    t->line = line;
    t->col = col;
}

static void lex_identifier(Lexer* lex) {
    int start = lex->pos;
    int line = lex->line, col = lex->col;

    while (lex->pos < lex->length && (isalnum(peek(lex)) || peek(lex) == '_'))
        advance_char(lex);

    int len = lex->pos - start;
    TokenType type = check_keyword(lex->source + start, len);
    add_token_n(lex, type, lex->source + start, len, line, col);
}

/* ---------- Main Tokenize ---------- */

Lexer* lexer_new(const char* source) {
    Lexer* lex = (Lexer*)calloc(1, sizeof(Lexer));
    lex->source = source;
    lex->length = (int)strlen(source);
    lex->pos = 0;
    lex->line = 1;
    lex->col = 1;
    lex->token_capacity = 256;
    lex->token_count = 0;
    lex->tokens = (Token*)malloc(sizeof(Token) * lex->token_capacity);
    return lex;
}

void lexer_free(Lexer* lex) {
    if (!lex) return;
    for (int i = 0; i < lex->token_count; i++)
        free(lex->tokens[i].value);
    free(lex->tokens);
    free(lex);
}

Token* lexer_tokenize(Lexer* lex) {
    while (lex->pos < lex->length) {
        skip_whitespace(lex);
        if (lex->pos >= lex->length) break;

        char c = peek(lex);
        int line = lex->line, col = lex->col;

        /* Numbers */
        if (isdigit(c) || (c == '.' && isdigit(peek_next(lex)))) {
            lex_number(lex);
            continue;
        }

        /* Strings */
        if (c == '"') {
            lex_string(lex);
            continue;
        }

        /* Single-char string (character literal) */
        if (c == '\'') {
            advance_char(lex);
            if (lex->pos < lex->length && peek_next(lex) == '\'') {
                char ch = advance_char(lex);
                advance_char(lex); /* closing ' */
                char s[2] = { ch, '\0' };
                add_token(lex, TOK_STRING, s, line, col);
            } else {
                fprintf(stderr, "Error at line %d: Invalid character literal\n", line);
                exit(EXIT_FAILURE);
            }
            continue;
        }

        /* Identifiers and keywords */
        if (isalpha(c) || c == '_') {
            lex_identifier(lex);
            continue;
        }

        /* Operators and delimiters */
        advance_char(lex);
        switch (c) {
            case '+':
                if (peek(lex) == '=') { advance_char(lex); add_token(lex, TOK_PLUS_EQ, "+=", line, col); }
                else add_token(lex, TOK_PLUS, "+", line, col);
                break;
            case '-':
                if (peek(lex) == '=') { advance_char(lex); add_token(lex, TOK_MINUS_EQ, "-=", line, col); }
                else add_token(lex, TOK_MINUS, "-", line, col);
                break;
            case '*':
                if (peek(lex) == '*') { advance_char(lex); add_token(lex, TOK_POWER, "**", line, col); }
                else if (peek(lex) == '=') { advance_char(lex); add_token(lex, TOK_STAR_EQ, "*=", line, col); }
                else add_token(lex, TOK_STAR, "*", line, col);
                break;
            case '/':
                if (peek(lex) == '/') { advance_char(lex); add_token(lex, TOK_FLOOR_DIV, "//", line, col); }
                else if (peek(lex) == '=') { advance_char(lex); add_token(lex, TOK_SLASH_EQ, "/=", line, col); }
                else add_token(lex, TOK_SLASH, "/", line, col);
                break;
            case '%':
                if (peek(lex) == '=') { advance_char(lex); add_token(lex, TOK_PERCENT_EQ, "%=", line, col); }
                else add_token(lex, TOK_PERCENT, "%", line, col);
                break;
            case '=':
                if (peek(lex) == '=') { advance_char(lex); add_token(lex, TOK_EQ, "==", line, col); }
                else add_token(lex, TOK_ASSIGN, "=", line, col);
                break;
            case '!':
                if (peek(lex) == '=') { advance_char(lex); add_token(lex, TOK_NEQ, "!=", line, col); }
                else add_token(lex, TOK_BANG, "!", line, col);
                break;
            case '<':
                if (peek(lex) == '=') { advance_char(lex); add_token(lex, TOK_LTE, "<=", line, col); }
                else if (peek(lex) == '<') { advance_char(lex); add_token(lex, TOK_SHL, "<<", line, col); }
                else add_token(lex, TOK_LT, "<", line, col);
                break;
            case '>':
                if (peek(lex) == '=') { advance_char(lex); add_token(lex, TOK_GTE, ">=", line, col); }
                else if (peek(lex) == '>') { advance_char(lex); add_token(lex, TOK_SHR, ">>", line, col); }
                else add_token(lex, TOK_GT, ">", line, col);
                break;
            case '&':
                if (peek(lex) == '&') { advance_char(lex); add_token(lex, TOK_DOUBLE_AMP, "&&", line, col); }
                else add_token(lex, TOK_AMPERSAND, "&", line, col);
                break;
            case '|':
                if (peek(lex) == '|') { advance_char(lex); add_token(lex, TOK_DOUBLE_PIPE, "||", line, col); }
                else add_token(lex, TOK_PIPE, "|", line, col);
                break;
            case '^':  add_token(lex, TOK_CARET, "^", line, col); break;
            case '~':  add_token(lex, TOK_TILDE, "~", line, col); break;
            case '(':  add_token(lex, TOK_LPAREN, "(", line, col); break;
            case ')':  add_token(lex, TOK_RPAREN, ")", line, col); break;
            case '[':  add_token(lex, TOK_LBRACKET, "[", line, col); break;
            case ']':  add_token(lex, TOK_RBRACKET, "]", line, col); break;
            case '{':  add_token(lex, TOK_LBRACE, "{", line, col); break;
            case '}':  add_token(lex, TOK_RBRACE, "}", line, col); break;
            case ',':  add_token(lex, TOK_COMMA, ",", line, col); break;
            case '.':  add_token(lex, TOK_DOT, ".", line, col); break;
            case ':':  add_token(lex, TOK_COLON, ":", line, col); break;
            case ';':  add_token(lex, TOK_SEMICOLON, ";", line, col); break;
            default:
                fprintf(stderr, "Error at line %d col %d: Unexpected character '%c'\n",
                        line, col, c);
                exit(EXIT_FAILURE);
        }
    }

    add_token(lex, TOK_EOF, NULL, lex->line, lex->col);
    return lex->tokens;
}

int lexer_token_count(Lexer* lex) {
    return lex->token_count;
}

const char* token_type_name(TokenType type) {
    switch (type) {
        case TOK_INT: return "INT";
        case TOK_FLOAT: return "FLOAT";
        case TOK_STRING: return "STRING";
        case TOK_IDENTIFIER: return "IDENTIFIER";
        case TOK_TRUE: return "true";
        case TOK_FALSE: return "false";
        case TOK_NULL: return "null";
        case TOK_IF: return "if";
        case TOK_ELSE: return "else";
        case TOK_ELIF: return "elif";
        case TOK_WHILE: return "while";
        case TOK_FOR: return "for";
        case TOK_IN: return "in";
        case TOK_BREAK: return "break";
        case TOK_CONTINUE: return "continue";
        case TOK_RETURN: return "return";
        case TOK_FN: return "fn";
        case TOK_IMPORT: return "import";
        case TOK_DEL: return "del";
        case TOK_TRY: return "try";
        case TOK_CATCH: return "catch";
        case TOK_THROW: return "throw";
        case TOK_CLASS: return "class";
        case TOK_EXTENDS: return "extends";
        case TOK_SUPER: return "super";
        case TOK_ASSIGN: return "=";
        case TOK_SEMICOLON: return ";";
        case TOK_EOF: return "EOF";
        default: return "TOKEN";
    }
}
