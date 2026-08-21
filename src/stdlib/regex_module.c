#include "regex_module.h"
#include "regex_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void runtime_error(int line, const char* fmt, ...);

static void module_add_fn(Value* mod, const char* name, BuiltinFn fn, int min, int max) {
    Value* key = value_new_string(name);
    Value* fn_val = value_new_builtin(name, fn, min, max);
    dict_set(mod, key, fn_val);
}

static Value* builtin_regex_match(Value** args, int argc) {
    if (args[0]->type != VAL_STRING || args[1]->type != VAL_STRING)
        runtime_error(0, "regex.match(): both arguments must be strings");
    RePattern pat;
    if (re_compile(args[0]->as.string, &pat) != 0)
        runtime_error(0, "regex.match(): invalid pattern");
    return value_new_bool(re_match_full(&pat, args[1]->as.string));
}

static Value* builtin_regex_search(Value** args, int argc) {
    if (args[0]->type != VAL_STRING || args[1]->type != VAL_STRING)
        runtime_error(0, "regex.search(): both arguments must be strings");
    RePattern pat;
    if (re_compile(args[0]->as.string, &pat) != 0)
        runtime_error(0, "regex.search(): invalid pattern");
    int mlen;
    int pos = re_search(&pat, args[1]->as.string, &mlen);
    if (pos < 0) return value_new_null();
    Value* result = value_new_dict();
    dict_set(result, value_new_string("match"),
             value_new_string_take(melody_strndup(args[1]->as.string + pos, mlen)));
    dict_set(result, value_new_string("start"), value_new_int(pos));
    dict_set(result, value_new_string("end"), value_new_int(pos + mlen));
    return result;
}

static Value* builtin_regex_findall(Value** args, int argc) {
    if (args[0]->type != VAL_STRING || args[1]->type != VAL_STRING)
        runtime_error(0, "regex.findall(): both arguments must be strings");
    RePattern pat;
    if (re_compile(args[0]->as.string, &pat) != 0)
        runtime_error(0, "regex.findall(): invalid pattern");
    Value* list = value_new_list();
    const char* text = args[1]->as.string;
    int offset = 0, tlen = (int)strlen(text);
    while (offset <= tlen) {
        int mlen;
        int pos = re_search(&pat, text + offset, &mlen);
        if (pos < 0) break;
        list_append(list, value_new_string_take(melody_strndup(text + offset + pos, mlen)));
        offset += pos + (mlen > 0 ? mlen : 1);
    }
    return list;
}

static Value* builtin_regex_replace(Value** args, int argc) {
    if (args[0]->type != VAL_STRING || args[1]->type != VAL_STRING || args[2]->type != VAL_STRING)
        runtime_error(0, "regex.replace(): all arguments must be strings");
    RePattern pat;
    if (re_compile(args[0]->as.string, &pat) != 0)
        runtime_error(0, "regex.replace(): invalid pattern");
    const char* text = args[1]->as.string;
    const char* repl = args[2]->as.string;
    int repl_len = (int)strlen(repl), tlen = (int)strlen(text);
    size_t cap = tlen * 2 + 64;
    char* result = (char*)malloc(cap);
    int rpos = 0, offset = 0;
    while (offset <= tlen) {
        int mlen;
        int pos = re_search(&pat, text + offset, &mlen);
        if (pos < 0) {
            int remain = tlen - offset;
            while (rpos + remain + 1 > (int)cap) { cap *= 2; result = (char*)realloc(result, cap); }
            memcpy(result + rpos, text + offset, remain);
            rpos += remain; break;
        }
        while (rpos + pos + repl_len + 1 > (int)cap) { cap *= 2; result = (char*)realloc(result, cap); }
        memcpy(result + rpos, text + offset, pos); rpos += pos;
        memcpy(result + rpos, repl, repl_len); rpos += repl_len;
        offset += pos + (mlen > 0 ? mlen : 1);
    }
    result[rpos] = '\0';
    return value_new_string_take(result);
}

static Value* builtin_regex_split(Value** args, int argc) {
    if (args[0]->type != VAL_STRING || args[1]->type != VAL_STRING)
        runtime_error(0, "regex.split(): both arguments must be strings");
    RePattern pat;
    if (re_compile(args[0]->as.string, &pat) != 0)
        runtime_error(0, "regex.split(): invalid pattern");
    Value* list = value_new_list();
    const char* text = args[1]->as.string;
    int tlen = (int)strlen(text), offset = 0;
    while (offset <= tlen) {
        int mlen;
        int pos = re_search(&pat, text + offset, &mlen);
        if (pos < 0) { list_append(list, value_new_string(text + offset)); break; }
        list_append(list, value_new_string_take(melody_strndup(text + offset, pos)));
        offset += pos + (mlen > 0 ? mlen : 1);
    }
    return list;
}

Value* create_regex_module(void) {
    static Value* cached = NULL;
    if (cached) return cached;
    Value* mod = value_new_dict();
    module_add_fn(mod, "match",   builtin_regex_match,   2, 2);
    module_add_fn(mod, "search",  builtin_regex_search,  2, 2);
    module_add_fn(mod, "findall", builtin_regex_findall, 2, 2);
    module_add_fn(mod, "replace", builtin_regex_replace, 3, 3);
    module_add_fn(mod, "split",   builtin_regex_split,   2, 2);
    cached = mod;
    return mod;
}
