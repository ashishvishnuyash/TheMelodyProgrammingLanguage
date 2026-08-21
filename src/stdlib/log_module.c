#include "log_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void runtime_error(int line, const char* fmt, ...);

static void module_add_fn(Value* mod, const char* name, BuiltinFn fn, int min, int max) {
    Value* key = value_new_string(name);
    Value* fn_val = value_new_builtin(name, fn, min, max);
    dict_set(mod, key, fn_val);
}

static int current_level = 0; /* 0=debug,1=info,2=warn,3=error,4=off */

static int level_from_string(const char* s) {
    if (strcmp(s,"debug")==0) return 0;
    if (strcmp(s,"info")==0)  return 1;
    if (strcmp(s,"warn")==0)  return 2;
    if (strcmp(s,"error")==0) return 3;
    if (strcmp(s,"off")==0)   return 4;
    return -1;
}

static void log_message(int level, const char* label, const char* msg) {
    if (level < current_level) return;
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    fprintf(stderr, "[%s] %02d:%02d:%02d %s\n", label, t->tm_hour, t->tm_min, t->tm_sec, msg);
}

static Value* builtin_log_debug(Value** args, int argc) {
    if (args[0]->type != VAL_STRING) runtime_error(0, "log.debug(): argument must be a string");
    log_message(0, "DEBUG", args[0]->as.string); return value_new_null();
}
static Value* builtin_log_info(Value** args, int argc) {
    if (args[0]->type != VAL_STRING) runtime_error(0, "log.info(): argument must be a string");
    log_message(1, "INFO", args[0]->as.string); return value_new_null();
}
static Value* builtin_log_warn(Value** args, int argc) {
    if (args[0]->type != VAL_STRING) runtime_error(0, "log.warn(): argument must be a string");
    log_message(2, "WARN", args[0]->as.string); return value_new_null();
}
static Value* builtin_log_error(Value** args, int argc) {
    if (args[0]->type != VAL_STRING) runtime_error(0, "log.error(): argument must be a string");
    log_message(3, "ERROR", args[0]->as.string); return value_new_null();
}
static Value* builtin_log_level(Value** args, int argc) {
    if (args[0]->type != VAL_STRING) runtime_error(0, "log.level(): argument must be a string");
    int lvl = level_from_string(args[0]->as.string);
    if (lvl < 0) runtime_error(0, "log.level(): invalid level '%s'", args[0]->as.string);
    current_level = lvl; return value_new_null();
}

Value* create_log_module(void) {
    static Value* cached = NULL;
    if (cached) return cached;
    Value* mod = value_new_dict();
    module_add_fn(mod, "debug", builtin_log_debug, 1, 1);
    module_add_fn(mod, "info",  builtin_log_info,  1, 1);
    module_add_fn(mod, "warn",  builtin_log_warn,  1, 1);
    module_add_fn(mod, "error", builtin_log_error, 1, 1);
    module_add_fn(mod, "level", builtin_log_level, 1, 1);
    cached = mod;
    return mod;
}
