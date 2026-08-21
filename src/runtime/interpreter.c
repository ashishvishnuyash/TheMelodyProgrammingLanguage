#include "interpreter.h"
#include "net.h"
#include "os_module.h"
#include "time_module.h"
#include "path_module.h"
#include "http_module.h"
#include "sqlite_module.h"
#include "cli_module.h"
#include "regex_module.h"
#include "crypto_module.h"
#include "uuid_module.h"
#include "url_module.h"
#include "log_module.h"
#include "env_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

/* ========== Error Reporting ========== */

static Interpreter* g_interp = NULL;

static Value* g_argv_list = NULL;

void interp_set_argv(int argc, char** argv) {
    g_argv_list = value_new_list();
    for (int i = 0; i < argc; i++)
        list_append(g_argv_list, value_new_string(argv[i]));
}

Value* interp_get_argv(void) {
    if (!g_argv_list) {
        g_argv_list = value_new_list();
    }
    return g_argv_list;
}

void runtime_error(int line, const char* fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (g_interp && g_interp->try_stack) {
        char msg[640];
        if (line > 0)
            snprintf(msg, sizeof(msg), "RuntimeError at line %d: %s", line, buf);
        else
            snprintf(msg, sizeof(msg), "RuntimeError: %s", buf);
        g_interp->thrown_value = value_new_string(msg);
        longjmp(g_interp->try_stack->buf, 1);
    }
    if (line > 0)
        fprintf(stderr, "Runtime error at line %d: %s\n", line, buf);
    else
        fprintf(stderr, "Runtime error: %s\n", buf);
    exit(EXIT_FAILURE);
}

/* ========== Built-in Functions ========== */

static Value* builtin_print(Value** args, int argc) {
    for (int i = 0; i < argc; i++) {
        if (i > 0) printf(" ");
        char* s = value_to_string(args[i]);
        printf("%s", s);
        free(s);
    }
    printf("\n");
    return value_new_null();
}

static Value* builtin_input(Value** args, int argc) {
    if (argc >= 1) {
        char* prompt = value_to_string(args[0]);
        printf("%s", prompt);
        free(prompt);
    }
    char buf[4096];
    if (!fgets(buf, sizeof(buf), stdin)) return value_new_string("");
    buf[strcspn(buf, "\r\n")] = '\0';
    return value_new_string(buf);
}

static Value* builtin_len(Value** args, int argc) {
    if (argc != 1) runtime_error(0, "len() takes 1 argument");
    Value* v = args[0];
    switch (v->type) {
        case VAL_STRING: return value_new_int((long long)strlen(v->as.string));
        case VAL_LIST:   return value_new_int(v->as.list.count);
        case VAL_DICT:   return value_new_int(v->as.dict.count);
        default:
            runtime_error(0, "len() not supported for type '%s'", value_type_name(v));
            return NULL;
    }
}

static Value* builtin_type_fn(Value** args, int argc) {
    if (argc != 1) runtime_error(0, "type() takes 1 argument");
    return value_new_string(value_type_name(args[0]));
}

static Value* builtin_int(Value** args, int argc) {
    if (argc != 1) runtime_error(0, "int() takes 1 argument");
    Value* v = args[0];
    switch (v->type) {
        case VAL_INT:    return value_new_int(v->as.integer);
        case VAL_FLOAT:  return value_new_int((long long)v->as.floating);
        case VAL_BOOL:   return value_new_int(v->as.boolean);
        case VAL_STRING: {
            char* endptr;
            errno = 0;
            long long n = strtoll(v->as.string, &endptr, 10);
            if (errno != 0 || endptr == v->as.string || *endptr != '\0')
                runtime_error(0, "Cannot convert '%s' to int", v->as.string);
            return value_new_int(n);
        }
        default:
            runtime_error(0, "Cannot convert '%s' to int", value_type_name(v));
            return NULL;
    }
}

static Value* builtin_float(Value** args, int argc) {
    if (argc != 1) runtime_error(0, "float() takes 1 argument");
    Value* v = args[0];
    switch (v->type) {
        case VAL_INT:    return value_new_float((double)v->as.integer);
        case VAL_FLOAT:  return value_new_float(v->as.floating);
        case VAL_BOOL:   return value_new_float(v->as.boolean ? 1.0 : 0.0);
        case VAL_STRING: {
            char* endptr;
            errno = 0;
            double d = strtod(v->as.string, &endptr);
            if (errno != 0 || endptr == v->as.string || *endptr != '\0')
                runtime_error(0, "Cannot convert '%s' to float", v->as.string);
            return value_new_float(d);
        }
        default:
            runtime_error(0, "Cannot convert '%s' to float", value_type_name(v));
            return NULL;
    }
}

static Value* builtin_str(Value** args, int argc) {
    if (argc != 1) runtime_error(0, "str() takes 1 argument");
    char* s = value_to_string(args[0]);
    return value_new_string_take(s);
}

static Value* builtin_bool(Value** args, int argc) {
    if (argc != 1) runtime_error(0, "bool() takes 1 argument");
    return value_new_bool(value_is_truthy(args[0]));
}

static Value* builtin_abs(Value** args, int argc) {
    if (argc != 1) runtime_error(0, "abs() takes 1 argument");
    Value* v = args[0];
    if (v->type == VAL_INT) return value_new_int(v->as.integer < 0 ? -v->as.integer : v->as.integer);
    if (v->type == VAL_FLOAT) return value_new_float(fabs(v->as.floating));
    runtime_error(0, "abs() requires numeric argument");
    return NULL;
}

static Value* builtin_min(Value** args, int argc) {
    if (argc != 2) runtime_error(0, "min() takes 2 arguments");
    return value_compare(args[0], args[1]) <= 0 ? args[0] : args[1];
}

static Value* builtin_max(Value** args, int argc) {
    if (argc != 2) runtime_error(0, "max() takes 2 arguments");
    return value_compare(args[0], args[1]) >= 0 ? args[0] : args[1];
}

static Value* builtin_round(Value** args, int argc) {
    if (argc != 1) runtime_error(0, "round() takes 1 argument");
    Value* v = args[0];
    if (v->type == VAL_INT) return value_new_int(v->as.integer);
    if (v->type == VAL_FLOAT) return value_new_int((long long)round(v->as.floating));
    runtime_error(0, "round() requires numeric argument");
    return NULL;
}

static Value* builtin_range(Value** args, int argc) {
    long long start = 0, end = 0, step = 1;
    if (argc == 1) {
        end = args[0]->as.integer;
    } else if (argc == 2) {
        start = args[0]->as.integer;
        end = args[1]->as.integer;
    } else if (argc == 3) {
        start = args[0]->as.integer;
        end = args[1]->as.integer;
        step = args[2]->as.integer;
    } else {
        runtime_error(0, "range() takes 1-3 arguments");
    }
    if (step == 0) runtime_error(0, "range() step cannot be 0");

    Value* list = value_new_list();
    if (step > 0) {
        for (long long i = start; i < end; i += step)
            list_append(list, value_new_int(i));
    } else {
        for (long long i = start; i > end; i += step)
            list_append(list, value_new_int(i));
    }
    return list;
}

static Value* builtin_open(Value** args, int argc) {
    if (argc != 2) runtime_error(0, "open() takes 2 arguments");
    char* path = args[0]->as.string;
    char* mode = args[1]->as.string;
    FILE* f = fopen(path, mode);
    if (!f) runtime_error(0, "Cannot open file '%s'", path);
    return value_new_file(f);
}

static Value* builtin_close(Value** args, int argc) {
    if (argc != 1) runtime_error(0, "close() takes 1 argument");
    if (args[0]->type != VAL_FILE) runtime_error(0, "close() requires file argument");
    if (args[0]->as.file) { fclose(args[0]->as.file); args[0]->as.file = NULL; }
    return value_new_null();
}

static Value* builtin_read(Value** args, int argc) {
    if (argc != 1) runtime_error(0, "read() takes 1 argument");
    if (args[0]->type != VAL_FILE) runtime_error(0, "read() requires file argument");
    FILE* f = args[0]->as.file;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* buf = (char*)malloc(size + 1);
    fread(buf, 1, size, f);
    buf[size] = '\0';
    return value_new_string_take(buf);
}

static Value* builtin_write(Value** args, int argc) {
    if (argc != 2) runtime_error(0, "write() takes 2 arguments");
    if (args[0]->type != VAL_FILE) runtime_error(0, "write() requires file argument");
    char* content = value_to_string(args[1]);
    fprintf(args[0]->as.file, "%s", content);
    free(content);
    return value_new_null();
}

static Value* builtin_append(Value** args, int argc) {
    if (argc != 2) runtime_error(0, "append() takes 2 arguments");
    if (args[0]->type != VAL_LIST) runtime_error(0, "append() requires list argument");
    list_append(args[0], args[1]);
    return value_new_null();
}

static Value* builtin_pop(Value** args, int argc) {
    if (argc != 1) runtime_error(0, "pop() takes 1 argument");
    if (args[0]->type != VAL_LIST) runtime_error(0, "pop() requires list argument");
    return list_pop(args[0]);
}

static Value* builtin_keys(Value** args, int argc) {
    if (argc != 1) runtime_error(0, "keys() takes 1 argument");
    if (args[0]->type != VAL_DICT) runtime_error(0, "keys() requires dict argument");
    return dict_keys_list(args[0]);
}

static Value* builtin_values(Value** args, int argc) {
    if (argc != 1) runtime_error(0, "values() takes 1 argument");
    if (args[0]->type != VAL_DICT) runtime_error(0, "values() requires dict argument");
    return dict_values_list(args[0]);
}

static Value* builtin_sort(Value** args, int argc) {
    if (argc != 1 || args[0]->type != VAL_LIST)
        runtime_error(0, "sort() takes 1 list argument");
    Value* list = args[0];
    /* Simple bubble sort */
    for (int i = 0; i < list->as.list.count - 1; i++) {
        for (int j = 0; j < list->as.list.count - 1 - i; j++) {
            if (value_compare(list->as.list.items[j], list->as.list.items[j+1]) > 0) {
                Value* tmp = list->as.list.items[j];
                list->as.list.items[j] = list->as.list.items[j+1];
                list->as.list.items[j+1] = tmp;
            }
        }
    }
    return value_new_null();
}

static Value* builtin_reverse(Value** args, int argc) {
    if (argc != 1 || args[0]->type != VAL_LIST)
        runtime_error(0, "reverse() takes 1 list argument");
    Value* list = args[0];
    int n = list->as.list.count;
    for (int i = 0; i < n / 2; i++) {
        Value* tmp = list->as.list.items[i];
        list->as.list.items[i] = list->as.list.items[n - 1 - i];
        list->as.list.items[n - 1 - i] = tmp;
    }
    return value_new_null();
}

static Value* builtin_list(Value** args, int argc) {
    if (argc != 1) runtime_error(0, "list() takes 1 argument");
    Value* v = args[0];
    if (v->type == VAL_LIST) {
        /* Copy the list */
        Value* copy = value_new_list();
        for (int i = 0; i < v->as.list.count; i++)
            list_append(copy, v->as.list.items[i]);
        return copy;
    }
    if (v->type == VAL_STRING) {
        /* Convert string to list of characters */
        Value* list = value_new_list();
        int len = (int)strlen(v->as.string);
        for (int i = 0; i < len; i++) {
            char s[2] = { v->as.string[i], '\0' };
            list_append(list, value_new_string(s));
        }
        return list;
    }
    if (v->type == VAL_DICT) {
        /* Convert dict to list of keys */
        return dict_keys_list(v);
    }
    runtime_error(0, "Cannot convert '%s' to list", value_type_name(v));
    return NULL;
}

static Value* builtin_dict(Value** args, int argc) {
    if (argc != 1) runtime_error(0, "dict() takes 1 argument");
    Value* v = args[0];
    if (v->type == VAL_DICT) {
        /* Copy the dict */
        Value* copy = value_new_dict();
        for (int i = 0; i < v->as.dict.count; i++)
            dict_set(copy, v->as.dict.keys[i], v->as.dict.values[i]);
        return copy;
    }
    if (v->type == VAL_LIST) {
        /* Convert list of [key, value] pairs to dict */
        Value* dict = value_new_dict();
        for (int i = 0; i < v->as.list.count; i++) {
            Value* pair = v->as.list.items[i];
            if (pair->type != VAL_LIST || pair->as.list.count != 2)
                runtime_error(0, "dict() requires list of [key, value] pairs");
            dict_set(dict, pair->as.list.items[0], pair->as.list.items[1]);
        }
        return dict;
    }
    runtime_error(0, "Cannot convert '%s' to dict", value_type_name(v));
    return NULL;
}

/* Forward declarations for helpers used by modules */
static double to_double(Value* v);
static int is_numeric(Value* v);

/* ========== Module Infrastructure ========== */

/* String-key lookup on a dict without allocating a temp Value */
static Value* dict_get_str(Value* dict, const char* key_str) {
    for (int i = 0; i < dict->as.dict.count; i++) {
        Value* k = dict->as.dict.keys[i];
        if (k->type == VAL_STRING && strcmp(k->as.string, key_str) == 0)
            return dict->as.dict.values[i];
    }
    return NULL;
}

static void module_add_fn(Value* mod, const char* name, BuiltinFn fn, int min, int max) {
    Value* key = value_new_string(name);
    Value* fn_val = value_new_builtin(name, fn, min, max);
    dict_set(mod, key, fn_val);
}

static void module_add_float(Value* mod, const char* name, double d) {
    Value* key = value_new_string(name);
    Value* val = value_new_float(d);
    dict_set(mod, key, val);
}

/* ========== StringBuilder (local copy for JSON) ========== */

typedef struct {
    char* buf;
    int len;
    int cap;
} StringBuilder;

static void sb_init(StringBuilder* sb) {
    sb->cap = 64;
    sb->buf = (char*)malloc(sb->cap);
    sb->buf[0] = '\0';
    sb->len = 0;
}

static void sb_append(StringBuilder* sb, const char* s) {
    int slen = (int)strlen(s);
    while (sb->len + slen + 1 > sb->cap) {
        sb->cap *= 2;
        sb->buf = (char*)realloc(sb->buf, sb->cap);
    }
    memcpy(sb->buf + sb->len, s, slen + 1);
    sb->len += slen;
}

static void sb_append_char(StringBuilder* sb, char c) {
    if (sb->len + 2 > sb->cap) {
        sb->cap *= 2;
        sb->buf = (char*)realloc(sb->buf, sb->cap);
    }
    sb->buf[sb->len++] = c;
    sb->buf[sb->len] = '\0';
}

static char* sb_finish(StringBuilder* sb) {
    return sb->buf;
}

/* ========== Math Module ========== */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E 2.71828182845904523536
#endif

static Value* builtin_math_sin(Value** args, int argc)  { return value_new_float(sin(to_double(args[0]))); }
static Value* builtin_math_cos(Value** args, int argc)  { return value_new_float(cos(to_double(args[0]))); }
static Value* builtin_math_tan(Value** args, int argc)  { return value_new_float(tan(to_double(args[0]))); }
static Value* builtin_math_asin(Value** args, int argc) { return value_new_float(asin(to_double(args[0]))); }
static Value* builtin_math_acos(Value** args, int argc) { return value_new_float(acos(to_double(args[0]))); }
static Value* builtin_math_atan(Value** args, int argc) { return value_new_float(atan(to_double(args[0]))); }
static Value* builtin_math_exp(Value** args, int argc)  { return value_new_float(exp(to_double(args[0]))); }

static Value* builtin_math_atan2(Value** args, int argc) {
    return value_new_float(atan2(to_double(args[0]), to_double(args[1])));
}

static Value* builtin_math_sqrt(Value** args, int argc) {
    double d = to_double(args[0]);
    if (d < 0) runtime_error(0, "math.sqrt() of negative number");
    return value_new_float(sqrt(d));
}

static Value* builtin_math_log(Value** args, int argc) {
    double d = to_double(args[0]);
    if (d <= 0) runtime_error(0, "math.log() of non-positive number");
    return value_new_float(log(d));
}

static Value* builtin_math_log10(Value** args, int argc) {
    double d = to_double(args[0]);
    if (d <= 0) runtime_error(0, "math.log10() of non-positive number");
    return value_new_float(log10(d));
}

static Value* builtin_math_floor(Value** args, int argc) {
    return value_new_int((long long)floor(to_double(args[0])));
}

static Value* builtin_math_ceil(Value** args, int argc) {
    return value_new_int((long long)ceil(to_double(args[0])));
}

static Value* builtin_math_pow(Value** args, int argc) {
    return value_new_float(pow(to_double(args[0]), to_double(args[1])));
}

static Value* builtin_math_fabs(Value** args, int argc) {
    return value_new_float(fabs(to_double(args[0])));
}

static Value* create_math_module(void) {
    static Value* cached = NULL;
    if (cached) return cached;
    Value* mod = value_new_dict();
    module_add_fn(mod, "sin",   builtin_math_sin,   1, 1);
    module_add_fn(mod, "cos",   builtin_math_cos,   1, 1);
    module_add_fn(mod, "tan",   builtin_math_tan,   1, 1);
    module_add_fn(mod, "asin",  builtin_math_asin,  1, 1);
    module_add_fn(mod, "acos",  builtin_math_acos,  1, 1);
    module_add_fn(mod, "atan",  builtin_math_atan,  1, 1);
    module_add_fn(mod, "atan2", builtin_math_atan2, 2, 2);
    module_add_fn(mod, "sqrt",  builtin_math_sqrt,  1, 1);
    module_add_fn(mod, "log",   builtin_math_log,   1, 1);
    module_add_fn(mod, "log10", builtin_math_log10, 1, 1);
    module_add_fn(mod, "floor", builtin_math_floor, 1, 1);
    module_add_fn(mod, "ceil",  builtin_math_ceil,  1, 1);
    module_add_fn(mod, "pow",   builtin_math_pow,   2, 2);
    module_add_fn(mod, "exp",   builtin_math_exp,   1, 1);
    module_add_fn(mod, "abs",   builtin_math_fabs,  1, 1);
    module_add_float(mod, "pi", M_PI);
    module_add_float(mod, "e",  M_E);
    module_add_float(mod, "inf", INFINITY);
    cached = mod;
    return mod;
}

/* ========== Random Module ========== */

static Value* builtin_random_random(Value** args, int argc) {
    return value_new_float((double)rand() / ((double)RAND_MAX + 1.0));
}

static Value* builtin_random_randint(Value** args, int argc) {
    if (args[0]->type != VAL_INT || args[1]->type != VAL_INT)
        runtime_error(0, "random.randint() requires 2 integer arguments");
    long long lo = args[0]->as.integer;
    long long hi = args[1]->as.integer;
    if (lo > hi) runtime_error(0, "random.randint(): min > max");
    long long range = hi - lo + 1;
    return value_new_int(lo + (long long)(rand() % range));
}

static Value* builtin_random_choice(Value** args, int argc) {
    if (args[0]->type != VAL_LIST)
        runtime_error(0, "random.choice() requires a list argument");
    Value* list = args[0];
    if (list->as.list.count == 0)
        runtime_error(0, "random.choice() called on empty list");
    return list->as.list.items[rand() % list->as.list.count];
}

static Value* builtin_random_shuffle(Value** args, int argc) {
    if (args[0]->type != VAL_LIST)
        runtime_error(0, "random.shuffle() requires a list argument");
    Value* list = args[0];
    int n = list->as.list.count;
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Value* tmp = list->as.list.items[i];
        list->as.list.items[i] = list->as.list.items[j];
        list->as.list.items[j] = tmp;
    }
    return value_new_null();
}

static Value* builtin_random_seed(Value** args, int argc) {
    if (args[0]->type != VAL_INT)
        runtime_error(0, "random.seed() requires 1 integer argument");
    srand((unsigned int)args[0]->as.integer);
    return value_new_null();
}

static Value* create_random_module(void) {
    static Value* cached = NULL;
    if (cached) return cached;
    static int seeded = 0;
    if (!seeded) { srand((unsigned int)time(NULL)); seeded = 1; }
    Value* mod = value_new_dict();
    module_add_fn(mod, "random",  builtin_random_random,  0, 0);
    module_add_fn(mod, "randint", builtin_random_randint, 2, 2);
    module_add_fn(mod, "choice",  builtin_random_choice,  1, 1);
    module_add_fn(mod, "shuffle", builtin_random_shuffle, 1, 1);
    module_add_fn(mod, "seed",    builtin_random_seed,    1, 1);
    cached = mod;
    return mod;
}

/* ========== File Module ========== */

static Value* builtin_file_read(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "file.read() requires a string path");
    FILE* f = fopen(args[0]->as.string, "rb");
    if (!f) runtime_error(0, "file.read(): cannot open '%s'", args[0]->as.string);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* buf = (char*)malloc(size + 1);
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return value_new_string_take(buf);
}

static Value* builtin_file_write(Value** args, int argc) {
    if (args[0]->type != VAL_STRING || args[1]->type != VAL_STRING)
        runtime_error(0, "file.write() requires path and content strings");
    FILE* f = fopen(args[0]->as.string, "wb");
    if (!f) runtime_error(0, "file.write(): cannot open '%s'", args[0]->as.string);
    fputs(args[1]->as.string, f);
    fclose(f);
    return value_new_null();
}

static Value* builtin_file_exists(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "file.exists() requires a string path");
    FILE* f = fopen(args[0]->as.string, "rb");
    if (f) { fclose(f); return value_new_bool(1); }
    return value_new_bool(0);
}

static Value* builtin_file_readlines(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "file.readlines() requires a string path");
    FILE* f = fopen(args[0]->as.string, "r");
    if (!f) runtime_error(0, "file.readlines(): cannot open '%s'", args[0]->as.string);
    Value* list = value_new_list();
    char buf[4096];
    while (fgets(buf, sizeof(buf), f)) {
        int len = (int)strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[--len] = '\0';
        if (len > 0 && buf[len - 1] == '\r') buf[--len] = '\0';
        list_append(list, value_new_string(buf));
    }
    fclose(f);
    return list;
}

static Value* builtin_file_append(Value** args, int argc) {
    if (args[0]->type != VAL_STRING || args[1]->type != VAL_STRING)
        runtime_error(0, "file.append() requires path and content strings");
    FILE* f = fopen(args[0]->as.string, "ab");
    if (!f) runtime_error(0, "file.append(): cannot open '%s'", args[0]->as.string);
    fputs(args[1]->as.string, f);
    fclose(f);
    return value_new_null();
}

static Value* create_file_module(void) {
    static Value* cached = NULL;
    if (cached) return cached;
    Value* mod = value_new_dict();
    module_add_fn(mod, "read",      builtin_file_read,      1, 1);
    module_add_fn(mod, "write",     builtin_file_write,     2, 2);
    module_add_fn(mod, "append",    builtin_file_append,    2, 2);
    module_add_fn(mod, "exists",    builtin_file_exists,    1, 1);
    module_add_fn(mod, "readlines", builtin_file_readlines, 1, 1);
    cached = mod;
    return mod;
}

/* ========== JSON Module ========== */

typedef struct {
    const char* src;
    int pos;
    int len;
} JsonParser;

static void json_skip_ws(JsonParser* p) {
    while (p->pos < p->len && isspace((unsigned char)p->src[p->pos]))
        p->pos++;
}

static Value* json_parse_value(JsonParser* p); /* forward decl */

static Value* json_parse_string(JsonParser* p) {
    p->pos++; /* skip opening " */
    StringBuilder sb;
    sb_init(&sb);
    while (p->pos < p->len && p->src[p->pos] != '"') {
        if (p->src[p->pos] == '\\' && p->pos + 1 < p->len) {
            p->pos++;
            switch (p->src[p->pos]) {
                case '"':  sb_append_char(&sb, '"');  break;
                case '\\': sb_append_char(&sb, '\\'); break;
                case '/':  sb_append_char(&sb, '/');  break;
                case 'n':  sb_append_char(&sb, '\n'); break;
                case 't':  sb_append_char(&sb, '\t'); break;
                case 'r':  sb_append_char(&sb, '\r'); break;
                case 'b':  sb_append_char(&sb, '\b'); break;
                case 'f':  sb_append_char(&sb, '\f'); break;
                case 'u': {
                    /* \uXXXX — decode to ASCII range only */
                    if (p->pos + 4 < p->len) {
                        char hex[5] = { p->src[p->pos+1], p->src[p->pos+2],
                                        p->src[p->pos+3], p->src[p->pos+4], '\0' };
                        unsigned int cp = (unsigned int)strtoul(hex, NULL, 16);
                        p->pos += 4;
                        if (cp < 128) sb_append_char(&sb, (char)cp);
                        else sb_append_char(&sb, '?');
                    }
                    break;
                }
                default: sb_append_char(&sb, p->src[p->pos]); break;
            }
        } else {
            sb_append_char(&sb, p->src[p->pos]);
        }
        p->pos++;
    }
    if (p->pos < p->len) p->pos++; /* skip closing " */
    return value_new_string_take(sb_finish(&sb));
}

static Value* json_parse_number(JsonParser* p) {
    int start = p->pos;
    int is_float = 0;
    if (p->src[p->pos] == '-') p->pos++;
    while (p->pos < p->len && isdigit((unsigned char)p->src[p->pos])) p->pos++;
    if (p->pos < p->len && p->src[p->pos] == '.') {
        is_float = 1;
        p->pos++;
        while (p->pos < p->len && isdigit((unsigned char)p->src[p->pos])) p->pos++;
    }
    if (p->pos < p->len && (p->src[p->pos] == 'e' || p->src[p->pos] == 'E')) {
        is_float = 1;
        p->pos++;
        if (p->pos < p->len && (p->src[p->pos] == '+' || p->src[p->pos] == '-')) p->pos++;
        while (p->pos < p->len && isdigit((unsigned char)p->src[p->pos])) p->pos++;
    }
    int len = p->pos - start;
    char* tmp = melody_strndup(p->src + start, len);
    Value* result;
    if (is_float) {
        result = value_new_float(strtod(tmp, NULL));
    } else {
        result = value_new_int(strtoll(tmp, NULL, 10));
    }
    free(tmp);
    return result;
}

static Value* json_parse_array(JsonParser* p) {
    p->pos++; /* skip [ */
    Value* list = value_new_list();
    json_skip_ws(p);
    if (p->pos < p->len && p->src[p->pos] == ']') { p->pos++; return list; }
    while (1) {
        list_append(list, json_parse_value(p));
        json_skip_ws(p);
        if (p->pos < p->len && p->src[p->pos] == ',') { p->pos++; continue; }
        break;
    }
    json_skip_ws(p);
    if (p->pos < p->len && p->src[p->pos] == ']') p->pos++;
    return list;
}

static Value* json_parse_object(JsonParser* p) {
    p->pos++; /* skip { */
    Value* dict = value_new_dict();
    json_skip_ws(p);
    if (p->pos < p->len && p->src[p->pos] == '}') { p->pos++; return dict; }
    while (1) {
        json_skip_ws(p);
        if (p->pos >= p->len || p->src[p->pos] != '"')
            runtime_error(0, "json.parse(): expected string key");
        Value* key = json_parse_string(p);
        json_skip_ws(p);
        if (p->pos < p->len && p->src[p->pos] == ':') p->pos++;
        Value* val = json_parse_value(p);
        dict_set(dict, key, val);
        json_skip_ws(p);
        if (p->pos < p->len && p->src[p->pos] == ',') { p->pos++; continue; }
        break;
    }
    json_skip_ws(p);
    if (p->pos < p->len && p->src[p->pos] == '}') p->pos++;
    return dict;
}

static Value* json_parse_value(JsonParser* p) {
    json_skip_ws(p);
    if (p->pos >= p->len)
        runtime_error(0, "json.parse(): unexpected end of input");
    char c = p->src[p->pos];
    if (c == '"') return json_parse_string(p);
    if (c == '[') return json_parse_array(p);
    if (c == '{') return json_parse_object(p);
    if (c == 't' && strncmp(p->src + p->pos, "true", 4) == 0)
        { p->pos += 4; return value_new_bool(1); }
    if (c == 'f' && strncmp(p->src + p->pos, "false", 5) == 0)
        { p->pos += 5; return value_new_bool(0); }
    if (c == 'n' && strncmp(p->src + p->pos, "null", 4) == 0)
        { p->pos += 4; return value_new_null(); }
    if (c == '-' || isdigit((unsigned char)c)) return json_parse_number(p);
    runtime_error(0, "json.parse(): unexpected character '%c'", c);
    return NULL;
}

/* JSON stringify */
static void json_stringify_value(StringBuilder* sb, Value* v) {
    char buf[64];
    switch (v->type) {
        case VAL_NULL: sb_append(sb, "null"); break;
        case VAL_BOOL: sb_append(sb, v->as.boolean ? "true" : "false"); break;
        case VAL_INT:
            snprintf(buf, sizeof(buf), "%lld", v->as.integer);
            sb_append(sb, buf);
            break;
        case VAL_FLOAT:
            snprintf(buf, sizeof(buf), "%g", v->as.floating);
            sb_append(sb, buf);
            break;
        case VAL_STRING: {
            sb_append_char(sb, '"');
            for (const char* s = v->as.string; *s; s++) {
                switch (*s) {
                    case '"':  sb_append(sb, "\\\""); break;
                    case '\\': sb_append(sb, "\\\\"); break;
                    case '\n': sb_append(sb, "\\n");  break;
                    case '\t': sb_append(sb, "\\t");  break;
                    case '\r': sb_append(sb, "\\r");  break;
                    case '\b': sb_append(sb, "\\b");  break;
                    case '\f': sb_append(sb, "\\f");  break;
                    default:   sb_append_char(sb, *s); break;
                }
            }
            sb_append_char(sb, '"');
            break;
        }
        case VAL_LIST:
            sb_append_char(sb, '[');
            for (int i = 0; i < v->as.list.count; i++) {
                if (i > 0) sb_append_char(sb, ',');
                json_stringify_value(sb, v->as.list.items[i]);
            }
            sb_append_char(sb, ']');
            break;
        case VAL_DICT:
            sb_append_char(sb, '{');
            for (int i = 0; i < v->as.dict.count; i++) {
                if (i > 0) sb_append_char(sb, ',');
                /* Keys: convert to string if needed */
                if (v->as.dict.keys[i]->type == VAL_STRING) {
                    json_stringify_value(sb, v->as.dict.keys[i]);
                } else {
                    char* ks = value_to_string(v->as.dict.keys[i]);
                    sb_append_char(sb, '"');
                    sb_append(sb, ks);
                    sb_append_char(sb, '"');
                    free(ks);
                }
                sb_append_char(sb, ':');
                json_stringify_value(sb, v->as.dict.values[i]);
            }
            sb_append_char(sb, '}');
            break;
        default:
            runtime_error(0, "json.stringify(): cannot serialize type '%s'",
                          value_type_name(v));
    }
}

static Value* builtin_json_parse(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "json.parse() requires a string argument");
    JsonParser p = { args[0]->as.string, 0, (int)strlen(args[0]->as.string) };
    Value* result = json_parse_value(&p);
    json_skip_ws(&p);
    if (p.pos != p.len)
        runtime_error(0, "json.parse(): trailing data after value");
    return result;
}

static Value* builtin_json_stringify(Value** args, int argc) {
    StringBuilder sb;
    sb_init(&sb);
    json_stringify_value(&sb, args[0]);
    return value_new_string_take(sb_finish(&sb));
}

static Value* create_json_module(void) {
    static Value* cached = NULL;
    if (cached) return cached;
    Value* mod = value_new_dict();
    module_add_fn(mod, "parse",     builtin_json_parse,     1, 1);
    module_add_fn(mod, "stringify", builtin_json_stringify, 1, 1);
    cached = mod;
    return mod;
}

/* ========== Method Dispatch ========== */

static Value* call_string_method(Value* obj, const char* method, Value** args, int argc) {
    if (strcmp(method, "upper") == 0) {
        char* s = melody_strdup(obj->as.string);
        for (int i = 0; s[i]; i++) s[i] = toupper((unsigned char)s[i]);
        return value_new_string_take(s);
    }
    if (strcmp(method, "lower") == 0) {
        char* s = melody_strdup(obj->as.string);
        for (int i = 0; s[i]; i++) s[i] = tolower((unsigned char)s[i]);
        return value_new_string_take(s);
    }
    if (strcmp(method, "strip") == 0) {
        const char* start = obj->as.string;
        const char* end = start + strlen(start) - 1;
        while (*start && isspace((unsigned char)*start)) start++;
        while (end > start && isspace((unsigned char)*end)) end--;
        return value_new_string_take(melody_strndup(start, end - start + 1));
    }
    if (strcmp(method, "split") == 0) {
        const char* sep = " ";
        if (argc >= 1 && args[0]->type == VAL_STRING) sep = args[0]->as.string;
        Value* list = value_new_list();
        char* str = melody_strdup(obj->as.string);
        int sep_len = (int)strlen(sep);
        char* p = str;
        while (1) {
            char* found = strstr(p, sep);
            if (!found) {
                list_append(list, value_new_string(p));
                break;
            }
            *found = '\0';
            list_append(list, value_new_string(p));
            p = found + sep_len;
        }
        free(str);
        return list;
    }
    if (strcmp(method, "find") == 0) {
        if (argc != 1 || args[0]->type != VAL_STRING)
            runtime_error(0, "find() takes 1 string argument");
        char* pos = strstr(obj->as.string, args[0]->as.string);
        if (!pos) return value_new_int(-1);
        return value_new_int((long long)(pos - obj->as.string));
    }
    if (strcmp(method, "replace") == 0) {
        if (argc != 2 || args[0]->type != VAL_STRING || args[1]->type != VAL_STRING)
            runtime_error(0, "replace() takes 2 string arguments");
        const char* src = obj->as.string;
        const char* old = args[0]->as.string;
        const char* new_s = args[1]->as.string;
        int old_len = (int)strlen(old);
        int new_len = (int)strlen(new_s);
        /* Count occurrences */
        int count = 0;
        const char* tmp = src;
        while ((tmp = strstr(tmp, old)) != NULL) { count++; tmp += old_len; }
        /* Build result */
        int result_len = (int)strlen(src) + count * (new_len - old_len);
        char* result = (char*)malloc(result_len + 1);
        char* dst = result;
        while (*src) {
            if (strncmp(src, old, old_len) == 0) {
                memcpy(dst, new_s, new_len);
                dst += new_len;
                src += old_len;
            } else {
                *dst++ = *src++;
            }
        }
        *dst = '\0';
        return value_new_string_take(result);
    }
    if (strcmp(method, "len") == 0) {
        return value_new_int((long long)strlen(obj->as.string));
    }
    if (strcmp(method, "startswith") == 0) {
        if (argc != 1 || args[0]->type != VAL_STRING)
            runtime_error(0, "startswith() takes 1 string argument");
        int slen = (int)strlen(obj->as.string);
        int plen = (int)strlen(args[0]->as.string);
        if (plen > slen) return value_new_bool(0);
        return value_new_bool(strncmp(obj->as.string, args[0]->as.string, plen) == 0);
    }
    if (strcmp(method, "endswith") == 0) {
        if (argc != 1 || args[0]->type != VAL_STRING)
            runtime_error(0, "endswith() takes 1 string argument");
        int slen = (int)strlen(obj->as.string);
        int plen = (int)strlen(args[0]->as.string);
        if (plen > slen) return value_new_bool(0);
        return value_new_bool(strcmp(obj->as.string + slen - plen, args[0]->as.string) == 0);
    }
    if (strcmp(method, "contains") == 0) {
        if (argc != 1 || args[0]->type != VAL_STRING)
            runtime_error(0, "contains() takes 1 string argument");
        return value_new_bool(strstr(obj->as.string, args[0]->as.string) != NULL);
    }
    if (strcmp(method, "count") == 0) {
        if (argc != 1 || args[0]->type != VAL_STRING)
            runtime_error(0, "count() takes 1 string argument");
        const char* hay = obj->as.string;
        const char* needle = args[0]->as.string;
        int nlen = (int)strlen(needle);
        if (nlen == 0) return value_new_int(0);
        int count = 0;
        while ((hay = strstr(hay, needle)) != NULL) { count++; hay += nlen; }
        return value_new_int(count);
    }
    if (strcmp(method, "substring") == 0) {
        if (argc < 1 || argc > 2 || args[0]->type != VAL_INT)
            runtime_error(0, "substring() takes 1-2 integer arguments");
        int slen = (int)strlen(obj->as.string);
        int start = (int)args[0]->as.integer;
        int end = slen;
        if (argc == 2) {
            if (args[1]->type != VAL_INT)
                runtime_error(0, "substring() end must be an integer");
            end = (int)args[1]->as.integer;
        }
        if (start < 0) start = 0;
        if (end > slen) end = slen;
        if (start >= end) return value_new_string("");
        return value_new_string_take(melody_strndup(obj->as.string + start, end - start));
    }
    if (strcmp(method, "trim_left") == 0) {
        const char* s = obj->as.string;
        while (*s && isspace((unsigned char)*s)) s++;
        return value_new_string(s);
    }
    if (strcmp(method, "trim_right") == 0) {
        int len = (int)strlen(obj->as.string);
        if (len == 0) return value_new_string("");
        int end = len - 1;
        while (end >= 0 && isspace((unsigned char)obj->as.string[end])) end--;
        return value_new_string_take(melody_strndup(obj->as.string, end + 1));
    }
    if (strcmp(method, "pad_left") == 0) {
        if (argc < 1 || args[0]->type != VAL_INT)
            runtime_error(0, "pad_left() requires a width (int)");
        int width = (int)args[0]->as.integer;
        char pad_char = ' ';
        if (argc >= 2 && args[1]->type == VAL_STRING && args[1]->as.string[0])
            pad_char = args[1]->as.string[0];
        int slen = (int)strlen(obj->as.string);
        if (slen >= width) return value_new_string(obj->as.string);
        int pad = width - slen;
        char* result = (char*)malloc(width + 1);
        memset(result, pad_char, pad);
        memcpy(result + pad, obj->as.string, slen);
        result[width] = '\0';
        return value_new_string_take(result);
    }
    if (strcmp(method, "pad_right") == 0) {
        if (argc < 1 || args[0]->type != VAL_INT)
            runtime_error(0, "pad_right() requires a width (int)");
        int width = (int)args[0]->as.integer;
        char pad_char = ' ';
        if (argc >= 2 && args[1]->type == VAL_STRING && args[1]->as.string[0])
            pad_char = args[1]->as.string[0];
        int slen = (int)strlen(obj->as.string);
        if (slen >= width) return value_new_string(obj->as.string);
        char* result = (char*)malloc(width + 1);
        memcpy(result, obj->as.string, slen);
        memset(result + slen, pad_char, width - slen);
        result[width] = '\0';
        return value_new_string_take(result);
    }
    if (strcmp(method, "join") == 0) {
        if (argc != 1 || args[0]->type != VAL_LIST)
            runtime_error(0, "join() takes 1 list argument");
        Value* list = args[0];
        const char* sep = obj->as.string;
        int sep_len = (int)strlen(sep);
        int total = 0;
        for (int i = 0; i < list->as.list.count; i++) {
            char* s = value_to_string(list->as.list.items[i]);
            total += (int)strlen(s);
            free(s);
            if (i > 0) total += sep_len;
        }
        char* result = (char*)malloc(total + 1);
        result[0] = '\0';
        int pos = 0;
        for (int i = 0; i < list->as.list.count; i++) {
            if (i > 0) { memcpy(result + pos, sep, sep_len); pos += sep_len; }
            char* s = value_to_string(list->as.list.items[i]);
            int slen = (int)strlen(s);
            memcpy(result + pos, s, slen);
            pos += slen;
            free(s);
        }
        result[pos] = '\0';
        return value_new_string_take(result);
    }
    if (strcmp(method, "format") == 0) {
        const char* fmt = obj->as.string;
        int fmt_len = (int)strlen(fmt);
        int out_size = fmt_len;
        int arg_idx = 0;
        for (int i = 0; i < fmt_len - 1; i++) {
            if (fmt[i] == '{' && fmt[i+1] == '}') {
                if (arg_idx < argc) {
                    char* s = value_to_string(args[arg_idx]);
                    out_size += (int)strlen(s) - 2;
                    free(s);
                    arg_idx++;
                }
            }
        }
        char* result = (char*)malloc(out_size + 256);
        int pos = 0;
        arg_idx = 0;
        for (int i = 0; i < fmt_len; i++) {
            if (i < fmt_len - 1 && fmt[i] == '{' && fmt[i+1] == '}') {
                if (arg_idx < argc) {
                    char* s = value_to_string(args[arg_idx++]);
                    int slen = (int)strlen(s);
                    memcpy(result + pos, s, slen);
                    pos += slen;
                    free(s);
                } else {
                    result[pos++] = '{';
                    result[pos++] = '}';
                }
                i++;
            } else {
                result[pos++] = fmt[i];
            }
        }
        result[pos] = '\0';
        return value_new_string_take(result);
    }
    runtime_error(0, "String has no method '%s'", method);
    return NULL;
}

static Value* call_list_method(Value* obj, const char* method, Value** args, int argc) {
    if (strcmp(method, "append") == 0) {
        if (argc != 1) runtime_error(0, "list.append() takes 1 argument");
        list_append(obj, args[0]);
        return value_new_null();
    }
    if (strcmp(method, "pop") == 0) {
        return list_pop(obj);
    }
    if (strcmp(method, "insert") == 0) {
        if (argc != 2 || args[0]->type != VAL_INT)
            runtime_error(0, "list.insert() takes an integer index and a value");
        list_insert(obj, (int)args[0]->as.integer, args[1]);
        return value_new_null();
    }
    if (strcmp(method, "sort") == 0) {
        Value* a[1] = { obj };
        return builtin_sort(a, 1);
    }
    if (strcmp(method, "reverse") == 0) {
        Value* a[1] = { obj };
        return builtin_reverse(a, 1);
    }
    if (strcmp(method, "len") == 0) {
        return value_new_int(obj->as.list.count);
    }
    if (strcmp(method, "join") == 0) {
        const char* sep = "";
        if (argc >= 1 && args[0]->type == VAL_STRING) sep = args[0]->as.string;
        /* Build joined string */
        int total = 0;
        int sep_len = (int)strlen(sep);
        char** parts = (char**)malloc(sizeof(char*) * obj->as.list.count);
        for (int i = 0; i < obj->as.list.count; i++) {
            parts[i] = value_to_string(obj->as.list.items[i]);
            total += (int)strlen(parts[i]);
            if (i > 0) total += sep_len;
        }
        char* result = (char*)malloc(total + 1);
        result[0] = '\0';
        for (int i = 0; i < obj->as.list.count; i++) {
            if (i > 0) strcat(result, sep);
            strcat(result, parts[i]);
            free(parts[i]);
        }
        free(parts);
        return value_new_string_take(result);
    }
    runtime_error(0, "List has no method '%s'", method);
    return NULL;
}

static Value* call_dict_method(Value* obj, const char* method, Value** args, int argc) {
    if (strcmp(method, "keys") == 0)   return dict_keys_list(obj);
    if (strcmp(method, "values") == 0) return dict_values_list(obj);
    if (strcmp(method, "get") == 0) {
        if (argc < 1) runtime_error(0, "dict.get() takes 1-2 arguments");
        Value* result = dict_get(obj, args[0]);
        if (result) return result;
        return argc >= 2 ? args[1] : value_new_null();
    }
    if (strcmp(method, "has") == 0) {
        if (argc != 1) runtime_error(0, "dict.has() takes 1 argument");
        return value_new_bool(dict_has(obj, args[0]));
    }
    if (strcmp(method, "len") == 0) {
        return value_new_int(obj->as.dict.count);
    }
    runtime_error(0, "Dict has no method '%s'", method);
    return NULL;
}

/* ========== Interpreter Core ========== */

/* Forward declarations */
static Value* eval(Interpreter* interp, ASTNode* node);
static Value* class_find_method(Value* klass, const char* name);

/* Evaluate a helper to get numeric values with proper type promotion */
static double to_double(Value* v) {
    if (v->type == VAL_INT) return (double)v->as.integer;
    if (v->type == VAL_FLOAT) return v->as.floating;
    return 0.0;
}

static int is_numeric(Value* v) {
    return v->type == VAL_INT || v->type == VAL_FLOAT;
}

/* FIX: Proper type promotion for mixed int/float arithmetic */
static Value* numeric_binary(TokenType op, Value* left, Value* right, int line) {
    int use_float = (left->type == VAL_FLOAT || right->type == VAL_FLOAT);

    if (use_float) {
        double a = to_double(left), b = to_double(right);
        switch (op) {
            case TOK_PLUS:      return value_new_float(a + b);
            case TOK_MINUS:     return value_new_float(a - b);
            case TOK_STAR:      return value_new_float(a * b);
            case TOK_SLASH:
                if (b == 0.0) runtime_error(line, "Division by zero");
                return value_new_float(a / b);
            case TOK_PERCENT:
                if (b == 0.0) runtime_error(line, "Modulo by zero");
                return value_new_float(fmod(a, b));
            case TOK_POWER:     return value_new_float(pow(a, b));
            case TOK_FLOOR_DIV:
                if (b == 0.0) runtime_error(line, "Division by zero");
                return value_new_float(floor(a / b));
            default: break;
        }
    } else {
        long long a = left->as.integer, b = right->as.integer;
        switch (op) {
            case TOK_PLUS:      return value_new_int(a + b);
            case TOK_MINUS:     return value_new_int(a - b);
            case TOK_STAR:      return value_new_int(a * b);
            case TOK_SLASH:
                if (b == 0) runtime_error(line, "Division by zero");
                return value_new_int(a / b);
            case TOK_PERCENT:
                if (b == 0) runtime_error(line, "Modulo by zero");
                return value_new_int(a % b);
            case TOK_POWER:     return value_new_int((long long)pow((double)a, (double)b));
            case TOK_FLOOR_DIV:
                if (b == 0) runtime_error(line, "Division by zero");
                return value_new_int(a / b);
            default: break;
        }
    }
    runtime_error(line, "Invalid numeric operation");
    return NULL;
}

static Value* eval_binary(Interpreter* interp, ASTNode* node) {
    /* Short-circuit for logical operators */
    if (node->data.binary.op == TOK_DOUBLE_AMP) {
        Value* left = eval(interp, node->data.binary.left);
        if (!value_is_truthy(left)) return value_new_bool(0);
        Value* right = eval(interp, node->data.binary.right);
        return value_new_bool(value_is_truthy(right));
    }
    if (node->data.binary.op == TOK_DOUBLE_PIPE) {
        Value* left = eval(interp, node->data.binary.left);
        if (value_is_truthy(left)) return value_new_bool(1);
        Value* right = eval(interp, node->data.binary.right);
        return value_new_bool(value_is_truthy(right));
    }

    Value* left = eval(interp, node->data.binary.left);
    Value* right = eval(interp, node->data.binary.right);
    int line = node->line;
    TokenType op = node->data.binary.op;

    /* Arithmetic on numbers */
    if (is_numeric(left) && is_numeric(right)) {
        if (op == TOK_PLUS || op == TOK_MINUS || op == TOK_STAR || op == TOK_SLASH ||
            op == TOK_PERCENT || op == TOK_POWER || op == TOK_FLOOR_DIV) {
            return numeric_binary(op, left, right, line);
        }
    }

    /* String concatenation */
    if (op == TOK_PLUS && left->type == VAL_STRING && right->type == VAL_STRING) {
        size_t l1 = strlen(left->as.string), l2 = strlen(right->as.string);
        char* s = (char*)malloc(l1 + l2 + 1);
        memcpy(s, left->as.string, l1);
        memcpy(s + l1, right->as.string, l2 + 1);
        return value_new_string_take(s);
    }

    /* String + non-string: auto-convert RHS to string */
    if (op == TOK_PLUS && left->type == VAL_STRING) {
        char* rs = value_to_string(right);
        size_t l1 = strlen(left->as.string), l2 = strlen(rs);
        char* s = (char*)malloc(l1 + l2 + 1);
        memcpy(s, left->as.string, l1);
        memcpy(s + l1, rs, l2 + 1);
        free(rs);
        return value_new_string_take(s);
    }

    /* String repetition: "abc" * 3 */
    if (op == TOK_STAR && left->type == VAL_STRING && right->type == VAL_INT) {
        int n = (int)right->as.integer;
        if (n <= 0) return value_new_string("");
        size_t len = strlen(left->as.string);
        char* s = (char*)malloc(len * n + 1);
        s[0] = '\0';
        for (int i = 0; i < n; i++) memcpy(s + i * len, left->as.string, len);
        s[len * n] = '\0';
        return value_new_string_take(s);
    }

    /* List concatenation */
    if (op == TOK_PLUS && left->type == VAL_LIST && right->type == VAL_LIST) {
        Value* result = value_new_list();
        for (int i = 0; i < left->as.list.count; i++)
            list_append(result, left->as.list.items[i]);
        for (int i = 0; i < right->as.list.count; i++)
            list_append(result, right->as.list.items[i]);
        return result;
    }

    /* Comparison operators */
    if (op == TOK_EQ)  return value_new_bool(value_equals(left, right));
    if (op == TOK_NEQ) return value_new_bool(!value_equals(left, right));

    if (is_numeric(left) && is_numeric(right)) {
        int cmp = value_compare(left, right);
        switch (op) {
            case TOK_LT:  return value_new_bool(cmp < 0);
            case TOK_GT:  return value_new_bool(cmp > 0);
            case TOK_LTE: return value_new_bool(cmp <= 0);
            case TOK_GTE: return value_new_bool(cmp >= 0);
            default: break;
        }
    }

    /* Bitwise operators (integers only) */
    if (left->type == VAL_INT && right->type == VAL_INT) {
        long long a = left->as.integer, b = right->as.integer;
        switch (op) {
            case TOK_AMPERSAND: return value_new_int(a & b);
            case TOK_PIPE:      return value_new_int(a | b);
            case TOK_CARET:     return value_new_int(a ^ b);
            case TOK_SHL:       return value_new_int(a << b);
            case TOK_SHR:       return value_new_int(a >> b);
            default: break;
        }
    }

    runtime_error(line, "Unsupported operation '%s' between '%s' and '%s'",
                  token_type_name(op), value_type_name(left), value_type_name(right));
    return NULL;
}

/* FIX: Unary ops now correctly dereference ->value, not ->type */
static Value* eval_unary(Interpreter* interp, ASTNode* node) {
    Value* operand = eval(interp, node->data.unary.operand);
    int line = node->line;

    switch (node->data.unary.op) {
        case TOK_MINUS:
            if (operand->type == VAL_INT)   return value_new_int(-operand->as.integer);
            if (operand->type == VAL_FLOAT) return value_new_float(-operand->as.floating);
            runtime_error(line, "Cannot negate '%s'", value_type_name(operand));
            break;
        case TOK_PLUS:
            if (is_numeric(operand)) return operand;
            runtime_error(line, "Cannot apply unary + to '%s'", value_type_name(operand));
            break;
        case TOK_BANG:
            return value_new_bool(!value_is_truthy(operand));
        case TOK_TILDE:
            if (operand->type == VAL_INT) return value_new_int(~operand->as.integer);
            runtime_error(line, "Bitwise NOT requires integer");
            break;
        default:
            runtime_error(line, "Unknown unary operator");
    }
    return value_new_null();
}

static Value* eval_assign(Interpreter* interp, ASTNode* node) {
    Value* val = eval(interp, node->data.assign.value);
    env_set(interp->env, node->data.assign.target->data.identifier, val);
    return val;
}

static Value* eval_compound_assign(Interpreter* interp, ASTNode* node) {
    const char* name = node->data.compound.target->data.identifier;
    Value* current_val = env_get(interp->env, name);
    if (!current_val) runtime_error(node->line, "Undefined variable '%s'", name);

    Value* rhs = eval(interp, node->data.compound.value);
    int line = node->line;

    /* Map compound operator to arithmetic operator */
    TokenType arith_op;
    switch (node->data.compound.op) {
        case TOK_PLUS_EQ:    arith_op = TOK_PLUS;    break;
        case TOK_MINUS_EQ:   arith_op = TOK_MINUS;   break;
        case TOK_STAR_EQ:    arith_op = TOK_STAR;    break;
        case TOK_SLASH_EQ:   arith_op = TOK_SLASH;   break;
        case TOK_PERCENT_EQ: arith_op = TOK_PERCENT; break;
        default: runtime_error(line, "Unknown compound operator"); return NULL;
    }

    Value* result;
    if (is_numeric(current_val) && is_numeric(rhs)) {
        result = numeric_binary(arith_op, current_val, rhs, line);
    } else if (arith_op == TOK_PLUS && current_val->type == VAL_STRING) {
        char* rs = value_to_string(rhs);
        size_t l1 = strlen(current_val->as.string), l2 = strlen(rs);
        char* s = (char*)malloc(l1 + l2 + 1);
        memcpy(s, current_val->as.string, l1);
        memcpy(s + l1, rs, l2 + 1);
        free(rs);
        result = value_new_string_take(s);
    } else {
        runtime_error(line, "Unsupported compound assignment");
        return NULL;
    }

    env_set(interp->env, name, result);
    return result;
}

static Value* eval_index(Interpreter* interp, ASTNode* node) {
    Value* obj = eval(interp, node->data.index.object);
    Value* idx = eval(interp, node->data.index.index);
    int line = node->line;

    if (obj->type == VAL_LIST) {
        if (idx->type != VAL_INT) runtime_error(line, "List index must be integer");
        int i = (int)idx->as.integer;
        if (i < 0) i += obj->as.list.count; /* negative indexing */
        if (i < 0 || i >= obj->as.list.count) runtime_error(line, "List index out of bounds");
        return obj->as.list.items[i];
    }
    if (obj->type == VAL_DICT) {
        Value* val = dict_get(obj, idx);
        if (!val) runtime_error(line, "Key not found in dict");
        return val;
    }
    if (obj->type == VAL_STRING) {
        if (idx->type != VAL_INT) runtime_error(line, "String index must be integer");
        int i = (int)idx->as.integer;
        int len = (int)strlen(obj->as.string);
        if (i < 0) i += len;
        if (i < 0 || i >= len) runtime_error(line, "String index out of bounds");
        char s[2] = { obj->as.string[i], '\0' };
        return value_new_string(s);
    }
    if (obj->type == VAL_OBJECT) {
        /* Field lookup: check instance fields first, then class methods */
        Value* val = dict_get(obj->as.object.fields, idx);
        if (val) return val;
        /* Fall through to method lookup if field name is a string */
        if (idx->type == VAL_STRING) {
            Value* method = class_find_method(obj->as.object.klass, idx->as.string);
            if (method) return method;
        }
        runtime_error(line, "'%s' object has no attribute '%s'",
                      obj->as.object.klass->as.klass.name,
                      idx->type == VAL_STRING ? idx->as.string : "?");
        return NULL;
    }

    runtime_error(line, "Cannot index type '%s'", value_type_name(obj));
    return NULL;
}

static Value* eval_index_assign(Interpreter* interp, ASTNode* node) {
    Value* obj = eval(interp, node->data.index_assign.object);
    Value* idx = eval(interp, node->data.index_assign.idx);
    Value* val = eval(interp, node->data.index_assign.value);
    int line = node->line;

    if (obj->type == VAL_LIST) {
        if (idx->type != VAL_INT) runtime_error(line, "List index must be integer");
        int i = (int)idx->as.integer;
        if (i < 0) i += obj->as.list.count;
        if (i < 0 || i >= obj->as.list.count) runtime_error(line, "List index out of bounds");
        obj->as.list.items[i] = val;
        return val;
    }
    if (obj->type == VAL_DICT) {
        dict_set(obj, idx, val);
        return val;
    }
    if (obj->type == VAL_OBJECT) {
        dict_set(obj->as.object.fields, idx, val);
        return val;
    }
    runtime_error(line, "Cannot assign to index of '%s'", value_type_name(obj));
    return NULL;
}

static Value* eval_func_def(Interpreter* interp, ASTNode* node) {
    Value* func = value_new_function(
        node->data.func_def.name,
        node->data.func_def.params,
        node->data.func_def.param_count,
        node->data.func_def.body,
        interp->env /* capture closure */
    );
    env_set(interp->env, node->data.func_def.name, func);
    return func;
}

static Value* eval_func_call(Interpreter* interp, ASTNode* node) {
    Value* callee = eval(interp, node->data.call.callee);
    int line = node->line;
    int argc = node->data.call.arg_count;

    /* Evaluate arguments */
    Value** args = NULL;
    if (argc > 0) {
        args = (Value**)malloc(sizeof(Value*) * argc);
        for (int i = 0; i < argc; i++)
            args[i] = eval(interp, node->data.call.args[i]);
    }

    Value* result = NULL;

    if (callee->type == VAL_BUILTIN) {
        /* Arity check */
        if (callee->as.builtin.min_args >= 0 && argc < callee->as.builtin.min_args)
            runtime_error(line, "'%s' expects at least %d arguments, got %d",
                          callee->as.builtin.name, callee->as.builtin.min_args, argc);
        if (callee->as.builtin.max_args >= 0 && argc > callee->as.builtin.max_args)
            runtime_error(line, "'%s' expects at most %d arguments, got %d",
                          callee->as.builtin.name, callee->as.builtin.max_args, argc);
        result = callee->as.builtin.fn(args, argc);
    }
    else if (callee->type == VAL_FUNCTION) {
        if (argc != callee->as.func.param_count)
            runtime_error(line, "'%s' expects %d arguments, got %d",
                          callee->as.func.name, callee->as.func.param_count, argc);

        /* Create new scope with closure as parent */
        Env* func_env = env_new(callee->as.func.closure);
        for (int i = 0; i < argc; i++)
            env_set_local(func_env, callee->as.func.params[i], args[i]);

        /* Execute body */
        Env* saved_env = interp->env;
        interp->env = func_env;
        result = eval(interp, callee->as.func.body);
        interp->env = saved_env;

        /* Handle return signal */
        if (interp->signal == SIG_RETURN)
            interp->signal = SIG_NONE;

        env_decref(func_env);
    }
    else if (callee->type == VAL_CLASS) {
        /* Class instantiation: ClassName(args...) */
        Value* instance = value_new_object(callee);

        /* Look for init method */
        Value* init_method = class_find_method(callee, "init");
        if (init_method) {
            /* init expects (self, ...user_args), so param_count = 1 + argc */
            if (argc != init_method->as.func.param_count - 1)
                runtime_error(line, "'%s.init' expects %d arguments, got %d",
                              callee->as.klass.name,
                              init_method->as.func.param_count - 1, argc);

            Env* init_env = env_new(init_method->as.func.closure);
            /* Bind self */
            env_set_local(init_env, init_method->as.func.params[0], instance);
            /* Bind remaining params */
            for (int i = 0; i < argc; i++)
                env_set_local(init_env, init_method->as.func.params[i + 1], args[i]);

            /* Set "super" so init can call super.init() */
            if (callee->as.klass.superclass)
                env_set_local(init_env, "super", callee->as.klass.superclass);

            Env* saved = interp->env;
            interp->env = init_env;
            eval(interp, init_method->as.func.body);
            interp->env = saved;
            if (interp->signal == SIG_RETURN) interp->signal = SIG_NONE;
            env_decref(init_env);
        }
        result = instance;
    }
    else {
        runtime_error(line, "Cannot call value of type '%s'", value_type_name(callee));
    }

    free(args);
    return result ? result : value_new_null();
}

static Value* eval_method_call(Interpreter* interp, ASTNode* node) {
    Value* obj = eval(interp, node->data.method.object);
    const char* method = node->data.method.method;
    int argc = node->data.method.arg_count;

    Value** args = NULL;
    if (argc > 0) {
        args = (Value**)malloc(sizeof(Value*) * argc);
        for (int i = 0; i < argc; i++)
            args[i] = eval(interp, node->data.method.args[i]);
    }

    Value* result = NULL;
    switch (obj->type) {
        case VAL_STRING: result = call_string_method(obj, method, args, argc); break;
        case VAL_LIST:   result = call_list_method(obj, method, args, argc); break;
        case VAL_DICT: {
            /* Check for callable entry in the dict (module function dispatch) */
            Value* entry = dict_get_str(obj, method);
            if (entry && entry->type == VAL_BUILTIN) {
                if (entry->as.builtin.min_args >= 0 && argc < entry->as.builtin.min_args)
                    runtime_error(node->line, "'%s' expects at least %d arguments, got %d",
                                  entry->as.builtin.name, entry->as.builtin.min_args, argc);
                if (entry->as.builtin.max_args >= 0 && argc > entry->as.builtin.max_args)
                    runtime_error(node->line, "'%s' expects at most %d arguments, got %d",
                                  entry->as.builtin.name, entry->as.builtin.max_args, argc);
                result = entry->as.builtin.fn(args, argc);
            } else if (entry && entry->type == VAL_FUNCTION) {
                if (argc != entry->as.func.param_count)
                    runtime_error(node->line, "'%s' expects %d arguments, got %d",
                                  entry->as.func.name, entry->as.func.param_count, argc);
                Env* fn_env = env_new(entry->as.func.closure);
                for (int i = 0; i < argc; i++)
                    env_set_local(fn_env, entry->as.func.params[i], args[i]);
                Env* saved = interp->env;
                interp->env = fn_env;
                result = eval(interp, entry->as.func.body);
                interp->env = saved;
                if (interp->signal == SIG_RETURN) interp->signal = SIG_NONE;
                env_decref(fn_env);
            } else {
                result = call_dict_method(obj, method, args, argc);
            }
            break;
        }
        case VAL_OBJECT: {
            /* Instance method call: obj.method(args) */
            Value* func = class_find_method(obj->as.object.klass, method);
            if (!func)
                runtime_error(node->line, "'%s' has no method '%s'",
                              obj->as.object.klass->as.klass.name, method);

            /* Method expects (self, ...user_args) */
            if (argc != func->as.func.param_count - 1)
                runtime_error(node->line, "'%s.%s' expects %d arguments, got %d",
                              obj->as.object.klass->as.klass.name, method,
                              func->as.func.param_count - 1, argc);

            Env* method_env = env_new(func->as.func.closure);
            env_set_local(method_env, func->as.func.params[0], obj); /* self */
            for (int i = 0; i < argc; i++)
                env_set_local(method_env, func->as.func.params[i + 1], args[i]);

            /* Set super */
            if (obj->as.object.klass->as.klass.superclass)
                env_set_local(method_env, "super", obj->as.object.klass->as.klass.superclass);

            Env* saved = interp->env;
            interp->env = method_env;
            result = eval(interp, func->as.func.body);
            interp->env = saved;
            if (interp->signal == SIG_RETURN) interp->signal = SIG_NONE;
            env_decref(method_env);
            break;
        }
        case VAL_CLASS: {
            /* super.method(self, args...) — called from within a method */
            Value* func = class_find_method(obj, method);
            if (!func)
                runtime_error(node->line, "Class '%s' has no method '%s'",
                              obj->as.klass.name, method);

            /* For super calls, all args are passed directly (including self) */
            if (argc != func->as.func.param_count)
                runtime_error(node->line, "'%s.%s' expects %d arguments, got %d",
                              obj->as.klass.name, method,
                              func->as.func.param_count, argc);

            Env* method_env = env_new(func->as.func.closure);
            for (int i = 0; i < argc; i++)
                env_set_local(method_env, func->as.func.params[i], args[i]);

            /* Set super to the parent's parent for chained super calls */
            if (obj->as.klass.superclass)
                env_set_local(method_env, "super", obj->as.klass.superclass);

            Env* saved = interp->env;
            interp->env = method_env;
            result = eval(interp, func->as.func.body);
            interp->env = saved;
            if (interp->signal == SIG_RETURN) interp->signal = SIG_NONE;
            env_decref(method_env);
            break;
        }
        default:
            runtime_error(node->line, "Type '%s' has no methods", value_type_name(obj));
    }

    free(args);
    return result ? result : value_new_null();
}

static Value* eval_if(Interpreter* interp, ASTNode* node) {
    Value* cond = eval(interp, node->data.if_stmt.condition);
    if (value_is_truthy(cond)) {
        return eval(interp, node->data.if_stmt.then_block);
    } else if (node->data.if_stmt.else_block) {
        return eval(interp, node->data.if_stmt.else_block);
    }
    return value_new_null();
}

static Value* eval_while(Interpreter* interp, ASTNode* node) {
    while (1) {
        Value* cond = eval(interp, node->data.while_loop.condition);
        if (!value_is_truthy(cond)) break;

        eval(interp, node->data.while_loop.body);

        if (interp->signal == SIG_BREAK) {
            interp->signal = SIG_NONE;
            break;
        }
        if (interp->signal == SIG_CONTINUE) {
            interp->signal = SIG_NONE;
            continue;
        }
        if (interp->signal == SIG_RETURN) break;
    }
    return value_new_null();
}

static Value* eval_for(Interpreter* interp, ASTNode* node) {
    eval(interp, node->data.for_loop.init);

    while (1) {
        Value* cond = eval(interp, node->data.for_loop.condition);
        if (!value_is_truthy(cond)) break;

        eval(interp, node->data.for_loop.body);

        if (interp->signal == SIG_BREAK) {
            interp->signal = SIG_NONE;
            break;
        }
        if (interp->signal == SIG_CONTINUE) {
            interp->signal = SIG_NONE;
        }
        if (interp->signal == SIG_RETURN) break;

        eval(interp, node->data.for_loop.increment);
    }
    return value_new_null();
}

static Value* eval_for_in(Interpreter* interp, ASTNode* node) {
    Value* iterable = eval(interp, node->data.for_in.iterable);
    const char* var = node->data.for_in.var_name;

    if (iterable->type == VAL_LIST) {
        for (int i = 0; i < iterable->as.list.count; i++) {
            env_set(interp->env, var, iterable->as.list.items[i]);
            eval(interp, node->data.for_in.body);

            if (interp->signal == SIG_BREAK) { interp->signal = SIG_NONE; break; }
            if (interp->signal == SIG_CONTINUE) { interp->signal = SIG_NONE; continue; }
            if (interp->signal == SIG_RETURN) break;
        }
    }
    else if (iterable->type == VAL_STRING) {
        int len = (int)strlen(iterable->as.string);
        for (int i = 0; i < len; i++) {
            char s[2] = { iterable->as.string[i], '\0' };
            env_set(interp->env, var, value_new_string(s));
            eval(interp, node->data.for_in.body);

            if (interp->signal == SIG_BREAK) { interp->signal = SIG_NONE; break; }
            if (interp->signal == SIG_CONTINUE) { interp->signal = SIG_NONE; continue; }
            if (interp->signal == SIG_RETURN) break;
        }
    }
    else if (iterable->type == VAL_DICT) {
        for (int i = 0; i < iterable->as.dict.count; i++) {
            env_set(interp->env, var, iterable->as.dict.keys[i]);
            eval(interp, node->data.for_in.body);

            if (interp->signal == SIG_BREAK) { interp->signal = SIG_NONE; break; }
            if (interp->signal == SIG_CONTINUE) { interp->signal = SIG_NONE; continue; }
            if (interp->signal == SIG_RETURN) break;
        }
    }
    else {
        runtime_error(node->line, "Cannot iterate over '%s'", value_type_name(iterable));
    }

    return value_new_null();
}

static Value* eval_block(Interpreter* interp, ASTNode* node) {
    Value* result = value_new_null();
    for (int i = 0; i < node->data.block.count; i++) {
        result = eval(interp, node->data.block.stmts[i]);
        if (interp->signal != SIG_NONE) break;
    }
    return result;
}

static Value* eval_del(Interpreter* interp, ASTNode* node) {
    ASTNode* target = node->data.del_target;
    if (target->type != NODE_INDEX)
        runtime_error(node->line, "del requires an indexed expression");

    Value* obj = eval(interp, target->data.index.object);
    Value* idx = eval(interp, target->data.index.index);

    if (obj->type == VAL_LIST) {
        if (idx->type != VAL_INT) runtime_error(node->line, "List index must be integer");
        list_delete_index(obj, (int)idx->as.integer);
    } else if (obj->type == VAL_DICT) {
        dict_delete(obj, idx);
    } else {
        runtime_error(node->line, "del not supported for '%s'", value_type_name(obj));
    }
    return value_new_null();
}

static Value* eval_import(Interpreter* interp, ASTNode* node) {
    Value* path_val = eval(interp, node->data.import_path);
    if (path_val->type != VAL_STRING)
        runtime_error(node->line, "import path must be a string");

    const char* path = path_val->as.string;

    /* Built-in module interception */
    if (strcmp(path, "math")   == 0) return create_math_module();
    if (strcmp(path, "random") == 0) return create_random_module();
    if (strcmp(path, "json")   == 0) return create_json_module();
    if (strcmp(path, "file")   == 0) return create_file_module();
    if (strcmp(path, "net")    == 0) return create_net_module();
    if (strcmp(path, "os")     == 0) return create_os_module();
    if (strcmp(path, "time")   == 0) return create_time_module();
    if (strcmp(path, "path")   == 0) return create_path_module();
    if (strcmp(path, "http")   == 0) return create_http_module();
    if (strcmp(path, "sqlite") == 0) return create_sqlite_module();
    if (strcmp(path, "cli")    == 0) return create_cli_module();
    if (strcmp(path, "regex")  == 0) return create_regex_module();
    if (strcmp(path, "crypto") == 0) return create_crypto_module();
    if (strcmp(path, "uuid")   == 0) return create_uuid_module();
    if (strcmp(path, "url")    == 0) return create_url_module();
    if (strcmp(path, "log")    == 0) return create_log_module();
    if (strcmp(path, "env")    == 0) return create_env_module();

    /* Package import: check melody_modules/<name>/main.mdy */
    {
        char pkg_path[512];
        snprintf(pkg_path, sizeof(pkg_path), "melody_modules/%s/main.mdy", path);
        FILE* pkg_f = fopen(pkg_path, "rb");
        if (pkg_f) {
            fseek(pkg_f, 0, SEEK_END);
            long pkg_size = ftell(pkg_f);
            rewind(pkg_f);
            char* pkg_source = (char*)malloc(pkg_size + 1);
            fread(pkg_source, 1, pkg_size, pkg_f);
            pkg_source[pkg_size] = '\0';
            fclose(pkg_f);

            interp_exec(interp, pkg_source);
            free(pkg_source);
            return value_new_null();
        }
    }

    /* File import */
    FILE* f = fopen(path, "rb");
    if (!f) runtime_error(node->line, "Cannot open file '%s'", path);

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* source = (char*)malloc(size + 1);
    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);

    interp_exec(interp, source);
    free(source);
    return value_new_null();
}

/* ========== OOP: Class / Object helpers ========== */

/* Walk the class hierarchy to find a method by name */
static Value* class_find_method(Value* klass, const char* name) {
    while (klass && klass->type == VAL_CLASS) {
        Value* key = value_new_string(name);
        Value* method = dict_get(klass->as.klass.methods, key);
        /* key is temporary — no need to free since dict_get just searches */
        if (method) return method;
        klass = klass->as.klass.superclass;
    }
    return NULL;
}

static Value* eval_class_def(Interpreter* interp, ASTNode* node) {
    /* Resolve superclass if present */
    Value* superclass = NULL;
    if (node->data.class_def.superclass) {
        superclass = env_get(interp->env, node->data.class_def.superclass);
        if (!superclass || superclass->type != VAL_CLASS)
            runtime_error(node->line, "'%s' is not a class", node->data.class_def.superclass);
    }

    /* Build methods dict */
    Value* methods = value_new_dict();
    for (int i = 0; i < node->data.class_def.method_count; i++) {
        ASTNode* mbody = node->data.class_def.method_bodies[i];
        Value* func = value_new_function(
            mbody->data.func_def.name,
            mbody->data.func_def.params,
            mbody->data.func_def.param_count,
            mbody->data.func_def.body,
            interp->env
        );
        Value* key = value_new_string(node->data.class_def.method_names[i]);
        dict_set(methods, key, func);
    }

    Value* klass = value_new_class(node->data.class_def.name, superclass, methods);
    env_set(interp->env, node->data.class_def.name, klass);
    return klass;
}

/* ========== Try / Catch / Throw ========== */

static Value* eval_try_catch(Interpreter* interp, ASTNode* node) {
    TryFrame frame;
    frame.prev = interp->try_stack;
    interp->try_stack = &frame;

    if (setjmp(frame.buf) == 0) {
        /* Normal path: execute try block */
        Value* result = eval(interp, node->data.try_catch.try_block);
        interp->try_stack = frame.prev; /* pop frame */
        return result;
    } else {
        /* Exception path: longjmp landed here */
        interp->try_stack = frame.prev; /* pop frame */
        Value* err = interp->thrown_value;
        interp->thrown_value = NULL;

        /* Bind the error to the catch variable */
        env_set(interp->env, node->data.try_catch.catch_var, err ? err : value_new_null());

        /* Execute catch block */
        return eval(interp, node->data.try_catch.catch_block);
    }
}

static Value* eval_throw(Interpreter* interp, ASTNode* node) {
    Value* val = eval(interp, node->data.throw_val);

    if (interp->try_stack) {
        /* Convert value to string for the error message */
        if (val->type == VAL_STRING) {
            interp->thrown_value = val;
        } else {
            char* s = value_to_string(val);
            interp->thrown_value = value_new_string_take(s);
        }
        longjmp(interp->try_stack->buf, 1);
    }

    /* No try frame — fatal error */
    char* s = value_to_string(val);
    fprintf(stderr, "Unhandled exception at line %d: %s\n", node->line, s);
    free(s);
    exit(EXIT_FAILURE);
    return NULL; /* unreachable */
}

/* Main eval dispatcher */
static Value* eval(Interpreter* interp, ASTNode* node) {
    if (!node) return value_new_null();

    switch (node->type) {
        case NODE_INT_LIT:    return value_new_int(node->data.int_val);
        case NODE_FLOAT_LIT:  return value_new_float(node->data.float_val);
        case NODE_STRING_LIT: return value_new_string(node->data.string_val);
        case NODE_BOOL_LIT:   return value_new_bool(node->data.bool_val);
        case NODE_NULL_LIT:   return value_new_null();

        case NODE_IDENTIFIER: {
            Value* val = env_get(interp->env, node->data.identifier);
            if (!val) runtime_error(node->line, "Undefined variable '%s'", node->data.identifier);
            return val;
        }

        case NODE_BINARY_OP:       return eval_binary(interp, node);
        case NODE_UNARY_OP:        return eval_unary(interp, node);
        case NODE_ASSIGN:          return eval_assign(interp, node);
        case NODE_COMPOUND_ASSIGN: return eval_compound_assign(interp, node);

        case NODE_LIST_LIT: {
            Value* list = value_new_list();
            for (int i = 0; i < node->data.list.count; i++)
                list_append(list, eval(interp, node->data.list.items[i]));
            return list;
        }
        case NODE_DICT_LIT: {
            Value* dict = value_new_dict();
            for (int i = 0; i < node->data.dict.count; i++) {
                Value* key = eval(interp, node->data.dict.keys[i]);
                Value* val = eval(interp, node->data.dict.values[i]);
                dict_set(dict, key, val);
            }
            return dict;
        }

        case NODE_INDEX:        return eval_index(interp, node);
        case NODE_INDEX_ASSIGN: return eval_index_assign(interp, node);
        case NODE_FUNC_DEF:     return eval_func_def(interp, node);
        case NODE_FUNC_CALL:    return eval_func_call(interp, node);
        case NODE_METHOD_CALL:  return eval_method_call(interp, node);

        case NODE_RETURN: {
            Value* val = eval(interp, node->data.return_val);
            interp->signal = SIG_RETURN;
            return val;
        }

        case NODE_IF:       return eval_if(interp, node);
        case NODE_WHILE:    return eval_while(interp, node);
        case NODE_FOR:      return eval_for(interp, node);
        case NODE_FOR_IN:   return eval_for_in(interp, node);

        case NODE_BREAK:
            interp->signal = SIG_BREAK;
            return value_new_null();
        case NODE_CONTINUE:
            interp->signal = SIG_CONTINUE;
            return value_new_null();

        case NODE_BLOCK:
        case NODE_PROGRAM:
            return eval_block(interp, node);

        case NODE_IMPORT:    return eval_import(interp, node);
        case NODE_DEL:       return eval_del(interp, node);
        case NODE_TRY_CATCH: return eval_try_catch(interp, node);
        case NODE_THROW:     return eval_throw(interp, node);
        case NODE_CLASS_DEF: return eval_class_def(interp, node);

        default:
            runtime_error(node->line, "Unknown AST node type %d", node->type);
            return NULL;
    }
}

/* ========== Assert ========== */

static Value* builtin_assert(Value** args, int argc) {
    if (!value_is_truthy(args[0])) {
        if (argc >= 2) {
            char* msg = value_to_string(args[1]);
            runtime_error(0, "Assertion failed: %s", msg);
            free(msg);
        } else {
            runtime_error(0, "Assertion failed");
        }
    }
    return value_new_null();
}

/* ========== Register Built-ins ========== */

static void register_builtin(Env* env, const char* name, BuiltinFn fn, int min, int max) {
    env_set_local(env, name, value_new_builtin(name, fn, min, max));
}

static void register_builtins(Env* env) {
    register_builtin(env, "print",   builtin_print,   0, -1);
    register_builtin(env, "input",   builtin_input,    0, 1);
    register_builtin(env, "len",     builtin_len,      1, 1);
    register_builtin(env, "type",    builtin_type_fn,  1, 1);
    register_builtin(env, "int",     builtin_int,      1, 1);
    register_builtin(env, "float",   builtin_float,    1, 1);
    register_builtin(env, "str",     builtin_str,      1, 1);
    register_builtin(env, "bool",    builtin_bool,     1, 1);
    register_builtin(env, "abs",     builtin_abs,      1, 1);
    register_builtin(env, "min",     builtin_min,      2, 2);
    register_builtin(env, "max",     builtin_max,      2, 2);
    register_builtin(env, "round",   builtin_round,    1, 1);
    register_builtin(env, "range",   builtin_range,    1, 3);
    register_builtin(env, "open",    builtin_open,     2, 2);
    register_builtin(env, "close",   builtin_close,    1, 1);
    register_builtin(env, "read",    builtin_read,     1, 1);
    register_builtin(env, "write",   builtin_write,    2, 2);
    register_builtin(env, "append",  builtin_append,   2, 2);
    register_builtin(env, "pop",     builtin_pop,      1, 1);
    register_builtin(env, "keys",    builtin_keys,     1, 1);
    register_builtin(env, "values",  builtin_values,   1, 1);
    register_builtin(env, "sort",    builtin_sort,     1, 1);
    register_builtin(env, "reverse", builtin_reverse,  1, 1);
    register_builtin(env, "list",    builtin_list,     1, 1);
    register_builtin(env, "dict",    builtin_dict,     1, 1);

    register_builtin(env, "assert",  builtin_assert,  1, 2);

    /* Backward compat aliases */
    register_builtin(env, "scan",    builtin_input,    0, 1);
}

/* ========== Public API ========== */

Interpreter* interp_new(void) {
    Interpreter* interp = (Interpreter*)calloc(1, sizeof(Interpreter));
    interp->global_env = env_new(NULL);
    interp->env = interp->global_env;
    interp->signal = SIG_NONE;
    interp->try_stack = NULL;
    interp->thrown_value = NULL;
    g_interp = interp;
    register_builtins(interp->global_env);
    return interp;
}

void interp_free(Interpreter* interp) {
    if (!interp) return;
    env_free(interp->global_env);
    free(interp);
}

Value* interp_eval(Interpreter* interp, ASTNode* node) {
    return eval(interp, node);
}

Value* interp_eval_fn_call(Value* fn, Value** args, int argc) {
    if (!g_interp) runtime_error(0, "No active interpreter");

    if (fn->type == VAL_BUILTIN) {
        return fn->as.builtin.fn(args, argc);
    }

    if (fn->type == VAL_FUNCTION) {
        if (argc != fn->as.func.param_count)
            runtime_error(0, "'%s' expects %d arguments, got %d",
                          fn->as.func.name, fn->as.func.param_count, argc);
        Env* func_env = env_new(fn->as.func.closure);
        for (int i = 0; i < argc; i++)
            env_set_local(func_env, fn->as.func.params[i], args[i]);

        Env* saved = g_interp->env;
        g_interp->env = func_env;
        Value* result = eval(g_interp, fn->as.func.body);
        g_interp->env = saved;
        if (g_interp->signal == SIG_RETURN)
            g_interp->signal = SIG_NONE;
        env_decref(func_env);
        return result;
    }

    runtime_error(0, "Value is not callable");
    return value_new_null();
}

int interp_exec(Interpreter* interp, const char* source) {
    Lexer* lex = lexer_new(source);
    Token* tokens = lexer_tokenize(lex);
    int count = lexer_token_count(lex);

    ASTNode* program = parse(tokens, count);
    Value* result = eval(interp, program);
    (void)result;

    ast_free(program);
    lexer_free(lex);
    return 0;
}
