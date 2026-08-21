#ifndef MELODY_VALUE_H
#define MELODY_VALUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

/* Forward declarations */
struct ASTNode;
struct Env;

typedef enum {
    VAL_NULL,
    VAL_BOOL,
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING,
    VAL_LIST,
    VAL_DICT,
    VAL_FUNCTION,
    VAL_BUILTIN,
    VAL_FILE,
    VAL_CLASS,
    VAL_OBJECT,
} ValueType;

typedef struct Value Value;
typedef Value* (*BuiltinFn)(Value** args, int argc);

struct Value {
    ValueType type;
    int ref_count;
    union {
        int boolean;
        long long integer;
        double floating;
        char* string;
        struct {
            Value** items;
            int count;
            int capacity;
        } list;
        struct {
            Value** keys;
            Value** values;
            int count;
            int capacity;
        } dict;
        struct {
            char* name;
            char** params;
            int param_count;
            struct ASTNode* body;
            struct Env* closure;
        } func;
        struct {
            char* name;
            BuiltinFn fn;
            int min_args;
            int max_args; /* -1 for variadic */
        } builtin;
        FILE* file;
        struct {                        /* VAL_CLASS */
            char* name;
            Value* superclass;          /* NULL or VAL_CLASS */
            Value* methods;             /* VAL_DICT: method_name → VAL_FUNCTION */
        } klass;
        struct {                        /* VAL_OBJECT */
            Value* klass;               /* VAL_CLASS this instance belongs to */
            Value* fields;              /* VAL_DICT: field_name → value */
        } object;
    } as;
};

/* Utility */
char* melody_strdup(const char* s);
char* melody_strndup(const char* s, size_t n);

/* Value creation */
Value* value_new_null(void);
Value* value_new_bool(int b);
Value* value_new_int(long long n);
Value* value_new_float(double f);
Value* value_new_string(const char* s);
Value* value_new_string_take(char* s);
Value* value_new_list(void);
Value* value_new_dict(void);
Value* value_new_function(const char* name, char** params, int param_count,
                          struct ASTNode* body, struct Env* closure);
Value* value_new_builtin(const char* name, BuiltinFn fn, int min_args, int max_args);
Value* value_new_file(FILE* f);
Value* value_new_class(const char* name, Value* superclass, Value* methods);
Value* value_new_object(Value* klass);

/* Immortal singletons — never freed */
extern Value g_val_null;
extern Value g_val_true;
extern Value g_val_false;

/* Reference counting */
void value_incref(Value* v);
void value_decref(Value* v);

/* Operations */
const char* value_type_name(Value* v);
char* value_to_string(Value* v);
char* value_repr(Value* v);
int value_is_truthy(Value* v);
int value_equals(Value* a, Value* b);
int value_compare(Value* a, Value* b);

/* List operations */
void list_append(Value* list, Value* item);
Value* list_pop(Value* list);
void list_insert(Value* list, int index, Value* item);
void list_delete_index(Value* list, int index);

/* Dict operations */
void dict_set(Value* dict, Value* key, Value* val);
Value* dict_get(Value* dict, Value* key);
void dict_delete(Value* dict, Value* key);
int dict_has(Value* dict, Value* key);
Value* dict_keys_list(Value* dict);
Value* dict_values_list(Value* dict);

#endif
