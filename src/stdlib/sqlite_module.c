#include "sqlite_module.h"
#include "sqlite3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Forward-declare from interpreter.c */
void runtime_error(int line, const char* fmt, ...);

/* ========== Helpers ========== */

static void module_add_fn(Value* mod, const char* name, BuiltinFn fn, int min, int max) {
    Value* key = value_new_string(name);
    Value* fn_val = value_new_builtin(name, fn, min, max);
    dict_set(mod, key, fn_val);
}

/* ========== Functions ========== */

/* sqlite.open(path) -> db handle (int) */
static Value* builtin_sqlite_open(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "sqlite.open(): argument must be a string");

    sqlite3* db = NULL;
    int rc = sqlite3_open(args[0]->as.string, &db);
    if (rc != SQLITE_OK) {
        const char* err = sqlite3_errmsg(db);
        sqlite3_close(db);
        runtime_error(0, "sqlite.open(): %s", err);
    }
    return value_new_int((long long)(uintptr_t)db);
}

/* sqlite.exec(db, sql) -> null */
static Value* builtin_sqlite_exec(Value** args, int argc) {
    if (args[0]->type != VAL_INT)
        runtime_error(0, "sqlite.exec(): first argument must be a db handle (int)");
    if (args[1]->type != VAL_STRING)
        runtime_error(0, "sqlite.exec(): second argument must be a string");

    sqlite3* db = (sqlite3*)(uintptr_t)args[0]->as.integer;
    char* err_msg = NULL;
    int rc = sqlite3_exec(db, args[1]->as.string, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        char* msg = melody_strdup(err_msg);
        sqlite3_free(err_msg);
        runtime_error(0, "sqlite.exec(): %s", msg);
    }
    return value_new_null();
}

/* sqlite.query(db, sql) -> list of dicts */
static Value* builtin_sqlite_query(Value** args, int argc) {
    if (args[0]->type != VAL_INT)
        runtime_error(0, "sqlite.query(): first argument must be a db handle (int)");
    if (args[1]->type != VAL_STRING)
        runtime_error(0, "sqlite.query(): second argument must be a string");

    sqlite3* db = (sqlite3*)(uintptr_t)args[0]->as.integer;
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(db, args[1]->as.string, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
        runtime_error(0, "sqlite.query(): %s", sqlite3_errmsg(db));

    Value* result = value_new_list();
    int col_count = sqlite3_column_count(stmt);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Value* row = value_new_dict();
        for (int i = 0; i < col_count; i++) {
            Value* key = value_new_string(sqlite3_column_name(stmt, i));
            Value* val;
            switch (sqlite3_column_type(stmt, i)) {
                case SQLITE_INTEGER:
                    val = value_new_int(sqlite3_column_int64(stmt, i));
                    break;
                case SQLITE_FLOAT:
                    val = value_new_float(sqlite3_column_double(stmt, i));
                    break;
                case SQLITE_TEXT:
                    val = value_new_string((const char*)sqlite3_column_text(stmt, i));
                    break;
                case SQLITE_NULL:
                default:
                    val = value_new_null();
                    break;
            }
            dict_set(row, key, val);
        }
        list_append(result, row);
    }

    sqlite3_finalize(stmt);
    return result;
}

/* sqlite.close(db) -> null */
static Value* builtin_sqlite_close(Value** args, int argc) {
    if (args[0]->type != VAL_INT)
        runtime_error(0, "sqlite.close(): argument must be a db handle (int)");

    sqlite3* db = (sqlite3*)(uintptr_t)args[0]->as.integer;
    sqlite3_close(db);
    return value_new_null();
}

/* ========== Module Creation ========== */

Value* create_sqlite_module(void) {
    static Value* cached = NULL;
    if (cached) return cached;

    Value* mod = value_new_dict();
    module_add_fn(mod, "open",  builtin_sqlite_open,  1, 1);
    module_add_fn(mod, "exec",  builtin_sqlite_exec,  2, 2);
    module_add_fn(mod, "query", builtin_sqlite_query,  2, 2);
    module_add_fn(mod, "close", builtin_sqlite_close,  1, 1);

    cached = mod;
    return mod;
}
