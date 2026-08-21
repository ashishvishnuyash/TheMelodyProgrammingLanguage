#ifndef MELODY_ENV_H
#define MELODY_ENV_H

#include "value.h"

#define ENV_INIT_CAP 32

typedef struct {
    char* key;
    Value* value;
    int occupied;
} EnvEntry;

typedef struct Env {
    EnvEntry* entries;
    int capacity;
    int count;
    int ref_count;
    struct Env* parent;
} Env;

Env* env_new(Env* parent);
void env_free(Env* env);
void env_incref(Env* env);
void env_decref(Env* env);

/* Get variable, searching up the scope chain. Returns NULL if not found. */
Value* env_get(Env* env, const char* name);

/* Set variable: updates existing binding in any scope, or creates in current. */
void env_set(Env* env, const char* name, Value* val);

/* Set variable only in the current scope (for function params, for-in vars). */
void env_set_local(Env* env, const char* name, Value* val);

/* Check if variable exists in any scope. */
int env_has(Env* env, const char* name);

#endif
