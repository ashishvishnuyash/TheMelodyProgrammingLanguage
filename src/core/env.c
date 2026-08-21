#include "env.h"
#include <stdio.h>
#include <string.h>

/* FNV-1a hash */
static unsigned int hash_string(const char* key) {
    unsigned int hash = 2166136261u;
    while (*key) {
        hash ^= (unsigned char)*key++;
        hash *= 16777619u;
    }
    return hash;
}

Env* env_new(Env* parent) {
    Env* env = (Env*)calloc(1, sizeof(Env));
    env->capacity = ENV_INIT_CAP;
    env->count = 0;
    env->ref_count = 1;
    env->parent = parent;
    if (parent) env_incref(parent);
    env->entries = (EnvEntry*)calloc(env->capacity, sizeof(EnvEntry));
    return env;
}

static void env_free_internal(Env* env) {
    if (!env) return;
    for (int i = 0; i < env->capacity; i++) {
        if (env->entries[i].occupied) {
            free(env->entries[i].key);
        }
    }
    free(env->entries);
    if (env->parent) env_decref(env->parent);
    free(env);
}

void env_free(Env* env) {
    env_free_internal(env);
}

void env_incref(Env* env) {
    if (env) env->ref_count++;
}

void env_decref(Env* env) {
    if (!env) return;
    env->ref_count--;
    if (env->ref_count <= 0) {
        env_free_internal(env);
    }
}

/* Find entry in a single scope (not parent). Returns slot index or -1. */
static int env_find_local(Env* env, const char* name) {
    unsigned int idx = hash_string(name) % env->capacity;
    for (int i = 0; i < env->capacity; i++) {
        int slot = (idx + i) % env->capacity;
        if (!env->entries[slot].occupied) return -1;
        if (strcmp(env->entries[slot].key, name) == 0) return slot;
    }
    return -1;
}

/* Resize when load factor > 0.7 */
static void env_resize(Env* env) {
    int old_cap = env->capacity;
    EnvEntry* old = env->entries;

    env->capacity *= 2;
    env->entries = (EnvEntry*)calloc(env->capacity, sizeof(EnvEntry));
    env->count = 0;

    for (int i = 0; i < old_cap; i++) {
        if (old[i].occupied) {
            env_set_local(env, old[i].key, old[i].value);
            free(old[i].key); /* env_set_local makes its own copy */
        }
    }
    free(old);
}

/* Insert or update in current scope only. */
void env_set_local(Env* env, const char* name, Value* val) {
    if ((double)env->count / env->capacity > 0.7) env_resize(env);

    unsigned int idx = hash_string(name) % env->capacity;
    for (int i = 0; i < env->capacity; i++) {
        int slot = (idx + i) % env->capacity;
        if (!env->entries[slot].occupied) {
            env->entries[slot].key = melody_strdup(name);
            env->entries[slot].value = val;
            env->entries[slot].occupied = 1;
            env->count++;
            return;
        }
        if (strcmp(env->entries[slot].key, name) == 0) {
            env->entries[slot].value = val;
            return;
        }
    }
}

/* Set variable: update existing in any scope, or create in current. */
void env_set(Env* env, const char* name, Value* val) {
    /* Walk up the scope chain looking for an existing binding */
    Env* e = env;
    while (e) {
        int slot = env_find_local(e, name);
        if (slot >= 0) {
            e->entries[slot].value = val;
            return;
        }
        e = e->parent;
    }
    /* Not found anywhere -- create in current scope */
    env_set_local(env, name, val);
}

/* Get variable, searching up the scope chain. */
Value* env_get(Env* env, const char* name) {
    Env* e = env;
    while (e) {
        int slot = env_find_local(e, name);
        if (slot >= 0) return e->entries[slot].value;
        e = e->parent;
    }
    return NULL;
}

int env_has(Env* env, const char* name) {
    return env_get(env, name) != NULL;
}
