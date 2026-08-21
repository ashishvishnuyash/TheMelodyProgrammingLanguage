#include "path_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Forward-declare from interpreter.c */
void runtime_error(int line, const char* fmt, ...);

#ifdef _WIN32
  #include <direct.h>
  #define PATH_SEP '\\'
  #define realpath(p, r) _fullpath((r), (p), _MAX_PATH)
#else
  #include <unistd.h>
  #define PATH_SEP '/'
#endif

/* ========== Helpers ========== */

static void module_add_fn(Value* mod, const char* name, BuiltinFn fn, int min, int max) {
    Value* key = value_new_string(name);
    Value* fn_val = value_new_builtin(name, fn, min, max);
    dict_set(mod, key, fn_val);
}

static int is_sep(char c) {
    return c == '/' || c == '\\';
}

/* ========== Functions ========== */

/* path.join(a, b, ...) → string — variadic */
static Value* builtin_path_join(Value** args, int argc) {
    if (argc == 0) return value_new_string("");

    /* Calculate total length */
    size_t total = 0;
    for (int i = 0; i < argc; i++) {
        if (args[i]->type != VAL_STRING)
            runtime_error(0, "path.join(): all arguments must be strings");
        total += strlen(args[i]->as.string) + 1;
    }

    char* result = (char*)malloc(total + 1);
    result[0] = '\0';

    for (int i = 0; i < argc; i++) {
        const char* part = args[i]->as.string;
        if (i > 0 && strlen(result) > 0) {
            char last = result[strlen(result) - 1];
            if (!is_sep(last) && !is_sep(part[0])) {
                size_t len = strlen(result);
                result[len] = PATH_SEP;
                result[len + 1] = '\0';
            }
        }
        strcat(result, part);
    }

    return value_new_string_take(result);
}

/* path.basename(p) → filename component */
static Value* builtin_path_basename(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "path.basename(): argument must be a string");

    const char* p = args[0]->as.string;
    size_t len = strlen(p);

    /* Skip trailing separators */
    while (len > 0 && is_sep(p[len - 1]))
        len--;

    if (len == 0) return value_new_string("");

    /* Find start of basename */
    size_t start = len;
    while (start > 0 && !is_sep(p[start - 1]))
        start--;

    char* result = melody_strndup(p + start, len - start);
    return value_new_string_take(result);
}

/* path.dirname(p) → directory component */
static Value* builtin_path_dirname(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "path.dirname(): argument must be a string");

    const char* p = args[0]->as.string;
    size_t len = strlen(p);

    /* Skip trailing separators */
    while (len > 0 && is_sep(p[len - 1]))
        len--;

    /* Skip basename */
    while (len > 0 && !is_sep(p[len - 1]))
        len--;

    /* Skip separator between dirname and basename */
    if (len > 1)
        len--;

    if (len == 0) return value_new_string(".");

    char* result = melody_strndup(p, len);
    return value_new_string_take(result);
}

/* path.ext(p) → extension including dot, e.g. ".txt" */
static Value* builtin_path_ext(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "path.ext(): argument must be a string");

    const char* p = args[0]->as.string;
    const char* dot = strrchr(p, '.');
    /* Make sure dot is after the last separator */
    const char* sep1 = strrchr(p, '/');
    const char* sep2 = strrchr(p, '\\');
    const char* sep = sep1;
    if (sep2 && (!sep || sep2 > sep)) sep = sep2;

    if (!dot || (sep && dot < sep))
        return value_new_string("");

    return value_new_string(dot);
}

/* path.exists(p) → bool */
static Value* builtin_path_exists(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "path.exists(): argument must be a string");

    struct stat st;
    return value_new_bool(stat(args[0]->as.string, &st) == 0);
}

/* path.isfile(p) → bool */
static Value* builtin_path_isfile(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "path.isfile(): argument must be a string");

    struct stat st;
    if (stat(args[0]->as.string, &st) != 0) return value_new_bool(0);
    return value_new_bool(S_ISREG(st.st_mode));
}

/* path.isdir(p) → bool */
static Value* builtin_path_isdir(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "path.isdir(): argument must be a string");

    struct stat st;
    if (stat(args[0]->as.string, &st) != 0) return value_new_bool(0);
    return value_new_bool(S_ISDIR(st.st_mode));
}

/* path.abs(p) → absolute path */
static Value* builtin_path_abs(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "path.abs(): argument must be a string");

#ifdef _WIN32
    char buf[_MAX_PATH];
    if (!_fullpath(buf, args[0]->as.string, _MAX_PATH))
        runtime_error(0, "path.abs(): failed for '%s'", args[0]->as.string);
    return value_new_string(buf);
#else
    char* resolved = realpath(args[0]->as.string, NULL);
    if (!resolved)
        runtime_error(0, "path.abs(): failed for '%s'", args[0]->as.string);
    return value_new_string_take(resolved);
#endif
}

/* ========== Module Creation ========== */

Value* create_path_module(void) {
    static Value* cached = NULL;
    if (cached) return cached;

    Value* mod = value_new_dict();
    module_add_fn(mod, "join",     builtin_path_join,     1, -1);  /* variadic */
    module_add_fn(mod, "basename", builtin_path_basename, 1, 1);
    module_add_fn(mod, "dirname",  builtin_path_dirname,  1, 1);
    module_add_fn(mod, "ext",      builtin_path_ext,      1, 1);
    module_add_fn(mod, "exists",   builtin_path_exists,   1, 1);
    module_add_fn(mod, "isfile",   builtin_path_isfile,   1, 1);
    module_add_fn(mod, "isdir",    builtin_path_isdir,    1, 1);
    module_add_fn(mod, "abs",      builtin_path_abs,      1, 1);

    cached = mod;
    return mod;
}
