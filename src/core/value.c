#include "value.h"
#include "env.h"
#include <stdio.h>
#include <math.h>

/* Immortal singletons — ref_count = INT_MAX means never freed */
Value g_val_null  = { VAL_NULL, INT_MAX, { .boolean = 0 } };
Value g_val_true  = { VAL_BOOL, INT_MAX, { .boolean = 1 } };
Value g_val_false = { VAL_BOOL, INT_MAX, { .boolean = 0 } };

/* ---------- Utility ---------- */

char* melody_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char* copy = (char*)malloc(len + 1);
    if (copy) memcpy(copy, s, len + 1);
    return copy;
}

char* melody_strndup(const char* s, size_t n) {
    if (!s) return NULL;
    char* copy = (char*)malloc(n + 1);
    if (copy) { memcpy(copy, s, n); copy[n] = '\0'; }
    return copy;
}

/* Simple dynamic string builder */
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

static char* sb_finish(StringBuilder* sb) {
    return sb->buf;
}

/* ---------- Value Creation ---------- */

static Value* value_alloc(ValueType type) {
    Value* v = (Value*)calloc(1, sizeof(Value));
    v->type = type;
    v->ref_count = 1;
    return v;
}

Value* value_new_null(void) {
    return &g_val_null;
}

Value* value_new_bool(int b) {
    return b ? &g_val_true : &g_val_false;
}

Value* value_new_int(long long n) {
    Value* v = value_alloc(VAL_INT);
    v->as.integer = n;
    return v;
}

Value* value_new_float(double f) {
    Value* v = value_alloc(VAL_FLOAT);
    v->as.floating = f;
    return v;
}

Value* value_new_string(const char* s) {
    Value* v = value_alloc(VAL_STRING);
    v->as.string = melody_strdup(s);
    return v;
}

Value* value_new_string_take(char* s) {
    Value* v = value_alloc(VAL_STRING);
    v->as.string = s;
    return v;
}

Value* value_new_list(void) {
    Value* v = value_alloc(VAL_LIST);
    v->as.list.capacity = 8;
    v->as.list.count = 0;
    v->as.list.items = (Value**)malloc(sizeof(Value*) * 8);
    return v;
}

Value* value_new_dict(void) {
    Value* v = value_alloc(VAL_DICT);
    v->as.dict.capacity = 8;
    v->as.dict.count = 0;
    v->as.dict.keys = (Value**)malloc(sizeof(Value*) * 8);
    v->as.dict.values = (Value**)malloc(sizeof(Value*) * 8);
    return v;
}

Value* value_new_function(const char* name, char** params, int param_count,
                          struct ASTNode* body, struct Env* closure) {
    Value* v = value_alloc(VAL_FUNCTION);
    v->as.func.name = melody_strdup(name);
    v->as.func.params = params;
    v->as.func.param_count = param_count;
    v->as.func.body = body;
    v->as.func.closure = closure;
    if (closure) env_incref(closure);
    return v;
}

Value* value_new_builtin(const char* name, BuiltinFn fn, int min_args, int max_args) {
    Value* v = value_alloc(VAL_BUILTIN);
    v->as.builtin.name = melody_strdup(name);
    v->as.builtin.fn = fn;
    v->as.builtin.min_args = min_args;
    v->as.builtin.max_args = max_args;
    return v;
}

Value* value_new_file(FILE* f) {
    Value* v = value_alloc(VAL_FILE);
    v->as.file = f;
    return v;
}

Value* value_new_class(const char* name, Value* superclass, Value* methods) {
    Value* v = value_alloc(VAL_CLASS);
    v->as.klass.name = melody_strdup(name);
    v->as.klass.superclass = superclass;
    v->as.klass.methods = methods; /* takes ownership */
    return v;
}

Value* value_new_object(Value* klass) {
    Value* v = value_alloc(VAL_OBJECT);
    v->as.object.klass = klass;
    v->as.object.fields = value_new_dict();
    return v;
}

/* ---------- Reference Counting ---------- */

void value_incref(Value* v) {
    if (v && v->ref_count != INT_MAX) v->ref_count++;
}

void value_decref(Value* v) {
    if (!v) return;
    if (v->ref_count == INT_MAX) return; /* immortal singleton */
    v->ref_count--;
    if (v->ref_count > 0) return;

    switch (v->type) {
        case VAL_STRING:
            free(v->as.string);
            break;
        case VAL_LIST:
            for (int i = 0; i < v->as.list.count; i++)
                value_decref(v->as.list.items[i]);
            free(v->as.list.items);
            break;
        case VAL_DICT:
            for (int i = 0; i < v->as.dict.count; i++) {
                value_decref(v->as.dict.keys[i]);
                value_decref(v->as.dict.values[i]);
            }
            free(v->as.dict.keys);
            free(v->as.dict.values);
            break;
        case VAL_FUNCTION:
            free(v->as.func.name);
            /* params and body are owned by the AST, don't free here */
            break;
        case VAL_BUILTIN:
            free(v->as.builtin.name);
            break;
        case VAL_FILE:
            if (v->as.file) fclose(v->as.file);
            break;
        case VAL_CLASS:
            free(v->as.klass.name);
            /* methods dict and superclass are shared, don't deeply free */
            break;
        case VAL_OBJECT:
            /* fields dict is owned by the object */
            break;
        default:
            break;
    }
    free(v);
}

/* ---------- Type Operations ---------- */

const char* value_type_name(Value* v) {
    if (!v) return "null";
    switch (v->type) {
        case VAL_NULL:     return "null";
        case VAL_BOOL:     return "bool";
        case VAL_INT:      return "int";
        case VAL_FLOAT:    return "float";
        case VAL_STRING:   return "string";
        case VAL_LIST:     return "list";
        case VAL_DICT:     return "dict";
        case VAL_FUNCTION: return "function";
        case VAL_BUILTIN:  return "builtin";
        case VAL_FILE:     return "file";
        case VAL_CLASS:    return "class";
        case VAL_OBJECT:   return v->as.object.klass->as.klass.name;
    }
    return "unknown";
}

char* value_to_string(Value* v) {
    if (!v) return melody_strdup("null");

    char buf[64];
    switch (v->type) {
        case VAL_NULL:
            return melody_strdup("null");
        case VAL_BOOL:
            return melody_strdup(v->as.boolean ? "true" : "false");
        case VAL_INT:
            snprintf(buf, sizeof(buf), "%lld", v->as.integer);
            return melody_strdup(buf);
        case VAL_FLOAT:
            snprintf(buf, sizeof(buf), "%g", v->as.floating);
            return melody_strdup(buf);
        case VAL_STRING:
            return melody_strdup(v->as.string);
        case VAL_LIST: {
            StringBuilder sb;
            sb_init(&sb);
            sb_append(&sb, "[");
            for (int i = 0; i < v->as.list.count; i++) {
                if (i > 0) sb_append(&sb, ", ");
                char* s = value_repr(v->as.list.items[i]);
                sb_append(&sb, s);
                free(s);
            }
            sb_append(&sb, "]");
            return sb_finish(&sb);
        }
        case VAL_DICT: {
            StringBuilder sb;
            sb_init(&sb);
            sb_append(&sb, "{");
            for (int i = 0; i < v->as.dict.count; i++) {
                if (i > 0) sb_append(&sb, ", ");
                char* k = value_repr(v->as.dict.keys[i]);
                char* val = value_repr(v->as.dict.values[i]);
                sb_append(&sb, k);
                sb_append(&sb, ": ");
                sb_append(&sb, val);
                free(k);
                free(val);
            }
            sb_append(&sb, "}");
            return sb_finish(&sb);
        }
        case VAL_FUNCTION:
            snprintf(buf, sizeof(buf), "<function %s>", v->as.func.name);
            return melody_strdup(buf);
        case VAL_BUILTIN:
            snprintf(buf, sizeof(buf), "<builtin %s>", v->as.builtin.name);
            return melody_strdup(buf);
        case VAL_FILE:
            return melody_strdup("<file>");
        case VAL_CLASS:
            snprintf(buf, sizeof(buf), "<class %s>", v->as.klass.name);
            return melody_strdup(buf);
        case VAL_OBJECT: {
            StringBuilder sb;
            sb_init(&sb);
            sb_append(&sb, "<");
            sb_append(&sb, v->as.object.klass->as.klass.name);
            sb_append(&sb, " instance>");
            return sb_finish(&sb);
        }
    }
    return melody_strdup("???");
}

char* value_repr(Value* v) {
    if (!v) return melody_strdup("null");
    if (v->type == VAL_STRING) {
        /* Wrap in quotes */
        size_t len = strlen(v->as.string);
        char* r = (char*)malloc(len + 3);
        r[0] = '"';
        memcpy(r + 1, v->as.string, len);
        r[len + 1] = '"';
        r[len + 2] = '\0';
        return r;
    }
    return value_to_string(v);
}

int value_is_truthy(Value* v) {
    if (!v) return 0;
    switch (v->type) {
        case VAL_NULL:     return 0;
        case VAL_BOOL:     return v->as.boolean;
        case VAL_INT:      return v->as.integer != 0;
        case VAL_FLOAT:    return v->as.floating != 0.0;
        case VAL_STRING:   return strlen(v->as.string) > 0;
        case VAL_LIST:     return v->as.list.count > 0;
        case VAL_DICT:     return v->as.dict.count > 0;
        default:           return 1;
    }
}

int value_equals(Value* a, Value* b) {
    if (!a && !b) return 1;
    if (!a || !b) return 0;
    if (a->type == VAL_NULL && b->type == VAL_NULL) return 1;
    if (a->type == VAL_NULL || b->type == VAL_NULL) return 0;

    /* Allow int/float comparison */
    if (a->type == VAL_INT && b->type == VAL_FLOAT)
        return (double)a->as.integer == b->as.floating;
    if (a->type == VAL_FLOAT && b->type == VAL_INT)
        return a->as.floating == (double)b->as.integer;

    if (a->type != b->type) return 0;

    switch (a->type) {
        case VAL_BOOL:   return a->as.boolean == b->as.boolean;
        case VAL_INT:    return a->as.integer == b->as.integer;
        case VAL_FLOAT:  return a->as.floating == b->as.floating;
        case VAL_STRING: return strcmp(a->as.string, b->as.string) == 0;
        case VAL_LIST:
            if (a->as.list.count != b->as.list.count) return 0;
            for (int i = 0; i < a->as.list.count; i++)
                if (!value_equals(a->as.list.items[i], b->as.list.items[i])) return 0;
            return 1;
        default:
            return a == b; /* identity comparison for functions, files, etc. */
    }
}

int value_compare(Value* a, Value* b) {
    /* Returns -1, 0, or 1. Used for <, >, <=, >= */
    double da, db;

    if (a->type == VAL_INT && b->type == VAL_INT) {
        if (a->as.integer < b->as.integer) return -1;
        if (a->as.integer > b->as.integer) return 1;
        return 0;
    }

    /* Promote to double for mixed or float comparisons */
    if (a->type == VAL_INT) da = (double)a->as.integer;
    else if (a->type == VAL_FLOAT) da = a->as.floating;
    else return 0;

    if (b->type == VAL_INT) db = (double)b->as.integer;
    else if (b->type == VAL_FLOAT) db = b->as.floating;
    else return 0;

    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}

/* ---------- List Operations ---------- */

static void list_grow(Value* list) {
    if (list->as.list.count >= list->as.list.capacity) {
        list->as.list.capacity *= 2;
        list->as.list.items = (Value**)realloc(list->as.list.items,
                              sizeof(Value*) * list->as.list.capacity);
    }
}

void list_append(Value* list, Value* item) {
    list_grow(list);
    list->as.list.items[list->as.list.count++] = item;
    value_incref(item);
}

Value* list_pop(Value* list) {
    if (list->as.list.count == 0) return value_new_null();
    list->as.list.count--;
    Value* item = list->as.list.items[list->as.list.count];
    return item; /* transfer ownership to caller */
}

void list_insert(Value* list, int index, Value* item) {
    list_grow(list);
    for (int i = list->as.list.count; i > index; i--)
        list->as.list.items[i] = list->as.list.items[i - 1];
    list->as.list.items[index] = item;
    list->as.list.count++;
    value_incref(item);
}

void list_delete_index(Value* list, int index) {
    if (index < 0 || index >= list->as.list.count) return;
    value_decref(list->as.list.items[index]);
    for (int i = index; i < list->as.list.count - 1; i++)
        list->as.list.items[i] = list->as.list.items[i + 1];
    list->as.list.count--;
}

/* ---------- Dict Operations ---------- */

static int dict_find(Value* dict, Value* key) {
    for (int i = 0; i < dict->as.dict.count; i++) {
        if (value_equals(dict->as.dict.keys[i], key))
            return i;
    }
    return -1;
}

static void dict_grow(Value* dict) {
    if (dict->as.dict.count >= dict->as.dict.capacity) {
        dict->as.dict.capacity *= 2;
        dict->as.dict.keys = (Value**)realloc(dict->as.dict.keys,
                             sizeof(Value*) * dict->as.dict.capacity);
        dict->as.dict.values = (Value**)realloc(dict->as.dict.values,
                               sizeof(Value*) * dict->as.dict.capacity);
    }
}

void dict_set(Value* dict, Value* key, Value* val) {
    int idx = dict_find(dict, key);
    if (idx >= 0) {
        value_decref(dict->as.dict.values[idx]);
        dict->as.dict.values[idx] = val;
        value_incref(val);
        return;
    }
    dict_grow(dict);
    dict->as.dict.keys[dict->as.dict.count] = key;
    dict->as.dict.values[dict->as.dict.count] = val;
    value_incref(key);
    value_incref(val);
    dict->as.dict.count++;
}

Value* dict_get(Value* dict, Value* key) {
    int idx = dict_find(dict, key);
    if (idx >= 0) return dict->as.dict.values[idx];
    return NULL;
}

void dict_delete(Value* dict, Value* key) {
    int idx = dict_find(dict, key);
    if (idx < 0) return;
    value_decref(dict->as.dict.keys[idx]);
    value_decref(dict->as.dict.values[idx]);
    for (int i = idx; i < dict->as.dict.count - 1; i++) {
        dict->as.dict.keys[i] = dict->as.dict.keys[i + 1];
        dict->as.dict.values[i] = dict->as.dict.values[i + 1];
    }
    dict->as.dict.count--;
}

int dict_has(Value* dict, Value* key) {
    return dict_find(dict, key) >= 0;
}

Value* dict_keys_list(Value* dict) {
    Value* list = value_new_list();
    for (int i = 0; i < dict->as.dict.count; i++)
        list_append(list, dict->as.dict.keys[i]);
    return list;
}

Value* dict_values_list(Value* dict) {
    Value* list = value_new_list();
    for (int i = 0; i < dict->as.dict.count; i++)
        list_append(list, dict->as.dict.values[i]);
    return list;
}
