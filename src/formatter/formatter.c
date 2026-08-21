#include "formatter.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>

#ifdef _WIN32
  #define PATH_SEP '\\'
#else
  #define PATH_SEP '/'
#endif

#define INDENT_SIZE 4

/* ========== Helpers ========== */

static char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* buf = (char*)malloc(size + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

static int is_mdy_file(const char* name) {
    int len = (int)strlen(name);
    return (len >= 4 && strcmp(name + len - 4, ".mdy") == 0);
}

/* ========== Token-based formatter ========== */

/* Grow buffer helper */
typedef struct {
    char* data;
    int len;
    int cap;
} Buffer;

static void buf_init(Buffer* b) {
    b->cap = 4096;
    b->data = (char*)malloc(b->cap);
    b->len = 0;
    b->data[0] = '\0';
}

static void buf_append(Buffer* b, const char* s, int slen) {
    while (b->len + slen + 1 > b->cap) {
        b->cap *= 2;
        b->data = (char*)realloc(b->data, b->cap);
    }
    memcpy(b->data + b->len, s, slen);
    b->len += slen;
    b->data[b->len] = '\0';
}

static void buf_append_char(Buffer* b, char c) { buf_append(b, &c, 1); }

static void buf_append_str(Buffer* b, const char* s) { buf_append(b, s, (int)strlen(s)); }

static void buf_indent(Buffer* b, int depth) {
    for (int i = 0; i < depth * INDENT_SIZE; i++) buf_append_char(b, ' ');
}

static int is_binary_op(TokenType t) {
    return t == TOK_PLUS || t == TOK_MINUS || t == TOK_STAR || t == TOK_SLASH ||
           t == TOK_PERCENT || t == TOK_POWER || t == TOK_FLOOR_DIV ||
           t == TOK_EQ || t == TOK_NEQ || t == TOK_LT || t == TOK_GT ||
           t == TOK_LTE || t == TOK_GTE || t == TOK_DOUBLE_AMP || t == TOK_DOUBLE_PIPE ||
           t == TOK_AMPERSAND || t == TOK_PIPE || t == TOK_CARET ||
           t == TOK_SHL || t == TOK_SHR ||
           t == TOK_ASSIGN || t == TOK_PLUS_EQ || t == TOK_MINUS_EQ ||
           t == TOK_STAR_EQ || t == TOK_SLASH_EQ || t == TOK_PERCENT_EQ;
}

static void emit_token_value(Buffer* buf, Token* t) {
    if (t->type == TOK_STRING) {
        buf_append_char(buf, '"');
        /* Escape special characters in string content */
        const char* s = t->value;
        while (*s) {
            switch (*s) {
                case '\n': buf_append_str(buf, "\\n"); break;
                case '\t': buf_append_str(buf, "\\t"); break;
                case '\r': buf_append_str(buf, "\\r"); break;
                case '\\': buf_append_str(buf, "\\\\"); break;
                case '"':  buf_append_str(buf, "\\\""); break;
                default:   buf_append_char(buf, *s); break;
            }
            s++;
        }
        buf_append_char(buf, '"');
    } else {
        buf_append_str(buf, t->value);
    }
}

/* Extract comment from a source line (everything from # to end of line) */
static const char* extract_comment(const char* source, int line_num) {
    int cur_line = 1;
    const char* p = source;
    /* Advance to the target line */
    while (*p && cur_line < line_num) {
        if (*p == '\n') cur_line++;
        p++;
    }
    /* Now p is at the start of line_num. Look for # outside of strings */
    int in_string = 0;
    char string_char = 0;
    while (*p && *p != '\n') {
        if (!in_string && (*p == '"' || *p == '\'')) {
            in_string = 1;
            string_char = *p;
        } else if (in_string && *p == string_char && *(p-1) != '\\') {
            in_string = 0;
        } else if (!in_string && *p == '#') {
            return p;
        }
        p++;
    }
    return NULL;
}

static char* format_source(const char* source) {
    Lexer* lexer = lexer_new(source);
    Token* tokens = lexer_tokenize(lexer);
    int count = lexer_token_count(lexer);

    Buffer buf;
    buf_init(&buf);

    int indent = 0;
    int at_line_start = 1;
    int prev_line = 0;
    int prev_type = -1;
    int prev_was_top_level_block_end = 0;

    for (int i = 0; i < count; i++) {
        Token* t = &tokens[i];

        if (t->type == TOK_EOF) break;

        /* Handle line change — insert newlines when token is on a different line */
        if (prev_line > 0 && t->line > prev_line) {
            /* Before inserting the newline, check if prev_line had a comment */
            const char* comment = extract_comment(source, prev_line);
            if (comment) {
                /* Emit space + comment (without trailing newline) */
                if (!at_line_start) buf_append_str(&buf, " ");
                const char* end = comment;
                while (*end && *end != '\n') end++;
                /* Ensure space after # */
                if (end - comment >= 2 && comment[1] != ' ') {
                    buf_append_char(&buf, '#');
                    buf_append_char(&buf, ' ');
                    buf_append(&buf, comment + 1, (int)(end - comment - 1));
                } else {
                    buf_append(&buf, comment, (int)(end - comment));
                }
            }

            if (!at_line_start) {
                buf_append_char(&buf, '\n');
                at_line_start = 1;
            }

            /* Insert blank lines for gaps > 1, but cap at 1 blank line */
            if (t->line > prev_line + 1 && indent == 0) {
                buf_append_char(&buf, '\n');
            }
        }

        /* Also check if this line is a comment-only line (no tokens on it) */
        /* We handle comment-only lines between token lines */
        if (prev_line > 0) {
            for (int line = prev_line + 1; line < t->line; line++) {
                const char* comment = extract_comment(source, line);
                if (comment) {
                    buf_indent(&buf, indent);
                    const char* end = comment;
                    while (*end && *end != '\n') end++;
                    if (end - comment >= 2 && comment[1] != ' ') {
                        buf_append_char(&buf, '#');
                        buf_append_char(&buf, ' ');
                        buf_append(&buf, comment + 1, (int)(end - comment - 1));
                    } else {
                        buf_append(&buf, comment, (int)(end - comment));
                    }
                    buf_append_char(&buf, '\n');
                }
            }
        } else {
            /* Handle comment-only lines before first token */
            for (int line = 1; line < t->line; line++) {
                const char* comment = extract_comment(source, line);
                if (comment) {
                    buf_indent(&buf, indent);
                    const char* end = comment;
                    while (*end && *end != '\n') end++;
                    if (end - comment >= 2 && comment[1] != ' ') {
                        buf_append_char(&buf, '#');
                        buf_append_char(&buf, ' ');
                        buf_append(&buf, comment + 1, (int)(end - comment - 1));
                    } else {
                        buf_append(&buf, comment, (int)(end - comment));
                    }
                    buf_append_char(&buf, '\n');
                }
            }
        }

        /* Closing brace: decrease indent before writing */
        if (t->type == TOK_RBRACE) {
            indent--;
            if (indent < 0) indent = 0;

            if (!at_line_start) {
                buf_append_char(&buf, '\n');
                at_line_start = 1;
            }
            buf_indent(&buf, indent);
            buf_append_str(&buf, "}");
            at_line_start = 0;
            prev_type = t->type;
            prev_line = t->line;
            if (indent == 0) prev_was_top_level_block_end = 1;
            continue;
        }

        /* Blank line between top-level blocks */
        if (prev_was_top_level_block_end && at_line_start && indent == 0) {
            prev_was_top_level_block_end = 0;
        }

        /* Opening brace */
        if (t->type == TOK_LBRACE) {
            buf_append_str(&buf, " {");
            buf_append_char(&buf, '\n');
            indent++;
            at_line_start = 1;
            prev_type = t->type;
            prev_line = t->line;
            prev_was_top_level_block_end = 0;
            continue;
        }

        /* Handle line start indent */
        if (at_line_start) {
            buf_indent(&buf, indent);
            at_line_start = 0;
        }

        /* Binary operators: space around */
        if (is_binary_op(t->type)) {
            buf_append_str(&buf, " ");
            buf_append_str(&buf, t->value);
            buf_append_str(&buf, " ");
            prev_type = t->type;
            prev_line = t->line;
            prev_was_top_level_block_end = 0;
            continue;
        }

        /* Comma: space after */
        if (t->type == TOK_COMMA) {
            buf_append_str(&buf, ", ");
            prev_type = t->type;
            prev_line = t->line;
            prev_was_top_level_block_end = 0;
            continue;
        }

        /* Semicolons: newline */
        if (t->type == TOK_SEMICOLON) {
            buf_append_char(&buf, '\n');
            at_line_start = 1;
            prev_type = t->type;
            prev_line = t->line;
            prev_was_top_level_block_end = 0;
            continue;
        }

        /* Regular token — add space if needed */
        if (prev_type != -1 && prev_type != TOK_LPAREN && prev_type != TOK_LBRACKET &&
            prev_type != TOK_DOT && prev_type != TOK_BANG && prev_type != TOK_TILDE &&
            prev_type != TOK_COMMA &&
            t->type != TOK_LPAREN && t->type != TOK_RPAREN &&
            t->type != TOK_LBRACKET && t->type != TOK_RBRACKET &&
            t->type != TOK_DOT && t->type != TOK_COMMA && t->type != TOK_SEMICOLON &&
            !is_binary_op((TokenType)prev_type) &&
            !at_line_start) {
            if (buf.len > 0 && buf.data[buf.len - 1] != ' ' && buf.data[buf.len - 1] != '\n')
                buf_append_char(&buf, ' ');
        }

        emit_token_value(&buf, t);
        prev_type = t->type;
        prev_line = t->line;
        prev_was_top_level_block_end = 0;
    }

    /* Handle trailing comment on last line */
    if (prev_line > 0) {
        const char* comment = extract_comment(source, prev_line);
        if (comment) {
            if (!at_line_start) buf_append_str(&buf, " ");
            const char* end = comment;
            while (*end && *end != '\n') end++;
            buf_append(&buf, comment, (int)(end - comment));
        }
    }

    /* Ensure trailing newline */
    if (buf.len > 0 && buf.data[buf.len - 1] != '\n')
        buf_append_char(&buf, '\n');

    /* Remove excessive trailing blank lines */
    while (buf.len >= 2 && buf.data[buf.len-1] == '\n' && buf.data[buf.len-2] == '\n')
        buf.len--;
    buf.data[buf.len] = '\0';

    lexer_free(lexer);
    return buf.data;
}

/* ========== Public API ========== */

int formatter_format_file(const char* path) {
    char* source = read_file(path);
    if (!source) {
        fprintf(stderr, "Error: cannot read '%s'\n", path);
        return -1;
    }

    char* formatted = format_source(source);
    free(source);

    FILE* f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "Error: cannot write '%s'\n", path);
        free(formatted);
        return -1;
    }
    fputs(formatted, f);
    fclose(f);
    free(formatted);

    printf("  formatted %s\n", path);
    return 0;
}

int formatter_format_dir(const char* dir_path) {
    DIR* dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "Error: cannot open directory '%s'\n", dir_path);
        return 1;
    }

    int errors = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s%c%s", dir_path, PATH_SEP, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) continue;

        if (st.st_mode & S_IFDIR) {
            errors += formatter_format_dir(full_path);
        } else if (is_mdy_file(entry->d_name)) {
            if (formatter_format_file(full_path) != 0) errors++;
        }
    }
    closedir(dir);
    return errors;
}
