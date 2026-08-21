#include "url_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void runtime_error(int line, const char* fmt, ...);

static void module_add_fn(Value* mod, const char* name, BuiltinFn fn, int min, int max) {
    Value* key = value_new_string(name);
    Value* fn_val = value_new_builtin(name, fn, min, max);
    dict_set(mod, key, fn_val);
}

static Value* builtin_url_parse(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "url.parse(): argument must be a string");
    const char* url = args[0]->as.string;
    Value* result = value_new_dict();

    const char* scheme_end = strstr(url, "://");
    if (scheme_end) {
        dict_set(result, value_new_string("scheme"),
                 value_new_string_take(melody_strndup(url, scheme_end - url)));
        url = scheme_end + 3;
    } else {
        dict_set(result, value_new_string("scheme"), value_new_null());
    }

    const char* frag = strchr(url, '#');
    char* work;
    if (frag) {
        work = melody_strndup(url, frag - url);
        dict_set(result, value_new_string("fragment"), value_new_string(frag + 1));
    } else {
        work = melody_strdup(url);
        dict_set(result, value_new_string("fragment"), value_new_null());
    }

    char* qmark = strchr(work, '?');
    if (qmark) {
        dict_set(result, value_new_string("query"), value_new_string(qmark + 1));
        *qmark = '\0';
    } else {
        dict_set(result, value_new_string("query"), value_new_null());
    }

    char* path_start = strchr(work, '/');
    if (path_start) {
        dict_set(result, value_new_string("path"), value_new_string(path_start));
        *path_start = '\0';
    } else {
        dict_set(result, value_new_string("path"), value_new_string("/"));
    }

    char* colon = strchr(work, ':');
    if (colon) {
        dict_set(result, value_new_string("host"),
                 value_new_string_take(melody_strndup(work, colon - work)));
        dict_set(result, value_new_string("port"), value_new_int(atoi(colon + 1)));
    } else {
        dict_set(result, value_new_string("host"), value_new_string(work));
        dict_set(result, value_new_string("port"), value_new_null());
    }
    free(work);
    return result;
}

static Value* builtin_url_build(Value** args, int argc) {
    if (args[0]->type != VAL_DICT)
        runtime_error(0, "url.build(): argument must be a dict");
    Value* d = args[0];
    char buf[2048];
    int pos = 0;
    Value* scheme = dict_get(d, value_new_string("scheme"));
    Value* host   = dict_get(d, value_new_string("host"));
    Value* port   = dict_get(d, value_new_string("port"));
    Value* path   = dict_get(d, value_new_string("path"));
    Value* query  = dict_get(d, value_new_string("query"));
    Value* frag   = dict_get(d, value_new_string("fragment"));
    if (scheme && scheme->type == VAL_STRING)
        pos += snprintf(buf+pos, sizeof(buf)-pos, "%s://", scheme->as.string);
    if (host && host->type == VAL_STRING)
        pos += snprintf(buf+pos, sizeof(buf)-pos, "%s", host->as.string);
    if (port && port->type == VAL_INT)
        pos += snprintf(buf+pos, sizeof(buf)-pos, ":%lld", port->as.integer);
    if (path && path->type == VAL_STRING)
        pos += snprintf(buf+pos, sizeof(buf)-pos, "%s", path->as.string);
    if (query && query->type == VAL_STRING)
        pos += snprintf(buf+pos, sizeof(buf)-pos, "?%s", query->as.string);
    if (frag && frag->type == VAL_STRING)
        pos += snprintf(buf+pos, sizeof(buf)-pos, "#%s", frag->as.string);
    return value_new_string(buf);
}

static Value* builtin_url_encode(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "url.encode(): argument must be a string");
    const char* s = args[0]->as.string;
    int len = (int)strlen(s);
    char* out = (char*)malloc(len * 3 + 1);
    int j = 0;
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (isalnum(c) || c=='-' || c=='_' || c=='.' || c=='~')
            out[j++] = c;
        else
            j += sprintf(out + j, "%%%02X", c);
    }
    out[j] = '\0';
    return value_new_string_take(out);
}

static Value* builtin_url_decode(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "url.decode(): argument must be a string");
    const char* s = args[0]->as.string;
    int len = (int)strlen(s);
    char* out = (char*)malloc(len + 1);
    int j = 0;
    for (int i = 0; i < len; i++) {
        if (s[i] == '%' && i+2 < len) {
            char hex[3] = {s[i+1], s[i+2], '\0'};
            out[j++] = (char)strtol(hex, NULL, 16);
            i += 2;
        } else if (s[i] == '+') { out[j++] = ' '; }
        else { out[j++] = s[i]; }
    }
    out[j] = '\0';
    return value_new_string_take(out);
}

Value* create_url_module(void) {
    static Value* cached = NULL;
    if (cached) return cached;
    Value* mod = value_new_dict();
    module_add_fn(mod, "parse",  builtin_url_parse,  1, 1);
    module_add_fn(mod, "build",  builtin_url_build,  1, 1);
    module_add_fn(mod, "encode", builtin_url_encode, 1, 1);
    module_add_fn(mod, "decode", builtin_url_decode, 1, 1);
    cached = mod;
    return mod;
}
