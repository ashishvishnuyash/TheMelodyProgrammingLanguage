#include "env_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void runtime_error(int line, const char* fmt, ...);

#ifdef _WIN32
  #define setenv(name, value, overwrite) _putenv_s(name, value)
  extern char** _environ;
  #define environ _environ
#else
  extern char** environ;
#endif

static void module_add_fn(Value* mod, const char* name, BuiltinFn fn, int min, int max) {
    Value* key = value_new_string(name);
    Value* fn_val = value_new_builtin(name, fn, min, max);
    dict_set(mod, key, fn_val);
}

static Value* builtin_env_load(Value** args, int argc) {
    const char* path = ".env";
    if (argc >= 1) {
        if (args[0]->type != VAL_STRING) runtime_error(0, "env.load(): argument must be a string");
        path = args[0]->as.string;
    }
    FILE* fp = fopen(path, "r");
    if (!fp) { runtime_error(0, "env.load(): cannot open '%s'", path); return value_new_null(); }
    char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        char* nl = strchr(line, '\n'); if (nl) *nl = '\0';
        nl = strchr(line, '\r'); if (nl) *nl = '\0';
        char* p = line;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '\0' || *p == '#') continue;
        char* eq = strchr(p, '=');
        if (!eq) continue;
        char* key_end = eq - 1;
        while (key_end > p && isspace((unsigned char)*key_end)) key_end--;
        int key_len = (int)(key_end - p + 1);
        char key[512];
        if (key_len >= (int)sizeof(key)) key_len = (int)sizeof(key) - 1;
        strncpy(key, p, key_len); key[key_len] = '\0';
        char* val = eq + 1;
        while (*val && isspace((unsigned char)*val)) val++;
        int vlen = (int)strlen(val);
        if (vlen >= 2 && ((val[0]=='"' && val[vlen-1]=='"') || (val[0]=='\'' && val[vlen-1]=='\''))) {
            val[vlen-1] = '\0'; val++;
        }
        setenv(key, val, 1);
    }
    fclose(fp);
    return value_new_null();
}

static Value* builtin_env_get(Value** args, int argc) {
    if (args[0]->type != VAL_STRING) runtime_error(0, "env.get(): argument must be a string");
    const char* val = getenv(args[0]->as.string);
    if (!val) return value_new_null();
    return value_new_string(val);
}

static Value* builtin_env_set(Value** args, int argc) {
    if (args[0]->type != VAL_STRING || args[1]->type != VAL_STRING)
        runtime_error(0, "env.set(): both arguments must be strings");
    setenv(args[0]->as.string, args[1]->as.string, 1);
    return value_new_null();
}

static Value* builtin_env_all(Value** args, int argc) {
    Value* result = value_new_dict();
    for (char** e = environ; *e; e++) {
        char* eq = strchr(*e, '=');
        if (!eq) continue;
        Value* key = value_new_string_take(melody_strndup(*e, eq - *e));
        Value* val = value_new_string(eq + 1);
        dict_set(result, key, val);
    }
    return result;
}

Value* create_env_module(void) {
    static Value* cached = NULL;
    if (cached) return cached;
    Value* mod = value_new_dict();
    module_add_fn(mod, "load", builtin_env_load, 0, 1);
    module_add_fn(mod, "get",  builtin_env_get,  1, 1);
    module_add_fn(mod, "set",  builtin_env_set,  2, 2);
    module_add_fn(mod, "all",  builtin_env_all,  0, 0);
    cached = mod;
    return mod;
}
