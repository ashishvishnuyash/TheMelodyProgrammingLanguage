#include "regex_engine.h"
#include <string.h>
#include <ctype.h>

int re_compile(const char* pattern, RePattern* out) {
    memset(out, 0, sizeof(RePattern));
    int i = 0, len = (int)strlen(pattern);

    while (i < len && out->count < 256) {
        ReNode* n = &out->nodes[out->count];
        memset(n, 0, sizeof(ReNode));

        if (pattern[i] == '\\' && i + 1 < len) {
            i++;
            switch (pattern[i]) {
                case 'd': n->type = RE_DIGIT; break;
                case 'D': n->type = RE_NOT_DIGIT; break;
                case 'w': n->type = RE_WORD; break;
                case 'W': n->type = RE_NOT_WORD; break;
                case 's': n->type = RE_SPACE; break;
                case 'S': n->type = RE_NOT_SPACE; break;
                default: n->type = RE_LITERAL; n->ch = pattern[i]; break;
            }
            i++;
        } else if (pattern[i] == '.') { n->type = RE_DOT; i++; }
        else if (pattern[i] == '^') { n->type = RE_CARET; i++; out->count++; continue; }
        else if (pattern[i] == '$') { n->type = RE_DOLLAR; i++; out->count++; continue; }
        else if (pattern[i] == '[') {
            i++;
            if (i < len && pattern[i] == '^') { n->type = RE_NCLASS; i++; }
            else { n->type = RE_CLASS; }
            n->class_len = 0;
            while (i < len && pattern[i] != ']' && n->class_len < 62) {
                if (i + 2 < len && pattern[i+1] == '-' && pattern[i+2] != ']') {
                    char lo = pattern[i], hi = pattern[i+2];
                    for (char c = lo; c <= hi && n->class_len < 62; c++)
                        n->class_chars[n->class_len++] = c;
                    i += 3;
                } else { n->class_chars[n->class_len++] = pattern[i++]; }
            }
            if (i < len && pattern[i] == ']') i++;
        } else { n->type = RE_LITERAL; n->ch = pattern[i]; i++; }

        if (i < len && (pattern[i] == '*' || pattern[i] == '+' || pattern[i] == '?')) {
            n->quantifier = pattern[i]; i++;
        }
        out->count++;
    }
    return 0;
}

static int node_matches_char(const ReNode* n, char c) {
    if (c == '\0') return 0;
    switch (n->type) {
        case RE_LITERAL:   return c == n->ch;
        case RE_DOT:       return c != '\n';
        case RE_DIGIT:     return isdigit((unsigned char)c);
        case RE_NOT_DIGIT: return !isdigit((unsigned char)c);
        case RE_WORD:      return isalnum((unsigned char)c) || c == '_';
        case RE_NOT_WORD:  return !(isalnum((unsigned char)c) || c == '_');
        case RE_SPACE:     return isspace((unsigned char)c);
        case RE_NOT_SPACE: return !isspace((unsigned char)c);
        case RE_CLASS:
            for (int i = 0; i < n->class_len; i++) if (n->class_chars[i] == c) return 1;
            return 0;
        case RE_NCLASS:
            for (int i = 0; i < n->class_len; i++) if (n->class_chars[i] == c) return 0;
            return 1;
        default: return 0;
    }
}

static int try_match(const RePattern* pat, int ni, const char* text, int ti) {
    int tlen = (int)strlen(text);
    while (ni < pat->count) {
        const ReNode* n = &pat->nodes[ni];
        if (n->type == RE_CARET) { ni++; continue; }
        if (n->type == RE_DOLLAR) { return (text[ti] == '\0') ? 0 : -1; }

        if (n->quantifier == '*') {
            int max_count = 0;
            while (ti + max_count < tlen && node_matches_char(n, text[ti + max_count])) max_count++;
            for (int k = max_count; k >= 0; k--) {
                int rest = try_match(pat, ni + 1, text, ti + k);
                if (rest >= 0) return k + rest;
            }
            return -1;
        }
        if (n->quantifier == '+') {
            int max_count = 0;
            while (ti + max_count < tlen && node_matches_char(n, text[ti + max_count])) max_count++;
            if (max_count == 0) return -1;
            for (int k = max_count; k >= 1; k--) {
                int rest = try_match(pat, ni + 1, text, ti + k);
                if (rest >= 0) return k + rest;
            }
            return -1;
        }
        if (n->quantifier == '?') {
            if (node_matches_char(n, text[ti])) {
                int rest = try_match(pat, ni + 1, text, ti + 1);
                if (rest >= 0) return 1 + rest;
            }
            int rest = try_match(pat, ni + 1, text, ti);
            if (rest >= 0) return rest;
            return -1;
        }
        if (!node_matches_char(n, text[ti])) return -1;
        ni++; ti++;
    }
    return 0;
}

int re_search(const RePattern* pat, const char* text, int* match_len) {
    int tlen = (int)strlen(text);
    int anchored = (pat->count > 0 && pat->nodes[0].type == RE_CARET);
    int start_limit = anchored ? 0 : tlen;
    for (int i = 0; i <= start_limit; i++) {
        int mlen = try_match(pat, 0, text, i);
        if (mlen >= 0) { if (match_len) *match_len = mlen; return i; }
        if (anchored) break;
    }
    if (match_len) *match_len = 0;
    return -1;
}

int re_match_full(const RePattern* pat, const char* text) {
    int mlen;
    int pos = re_search(pat, text, &mlen);
    return (pos == 0 && mlen == (int)strlen(text));
}
