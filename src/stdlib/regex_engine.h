#ifndef MELODY_REGEX_ENGINE_H
#define MELODY_REGEX_ENGINE_H

typedef enum {
    RE_LITERAL, RE_DOT, RE_CARET, RE_DOLLAR, RE_CLASS, RE_NCLASS,
    RE_DIGIT, RE_NOT_DIGIT, RE_WORD, RE_NOT_WORD, RE_SPACE, RE_NOT_SPACE,
} ReNodeType;

typedef struct {
    ReNodeType type;
    char ch;
    char class_chars[64];
    int class_len;
    int quantifier; /* 0=none, '*', '+', '?' */
} ReNode;

typedef struct {
    ReNode nodes[256];
    int count;
} RePattern;

int re_compile(const char* pattern, RePattern* out);
int re_search(const RePattern* pat, const char* text, int* match_len);
int re_match_full(const RePattern* pat, const char* text);

#endif
