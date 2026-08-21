#include "cli_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward-declare from interpreter.c — do NOT include interpreter.h */
void runtime_error(int line, const char* fmt, ...);
Value* interp_get_argv(void);
extern Value* interp_eval_fn_call(Value* fn, Value** args, int argc);

/* ========== Internal State ========== */

typedef struct {
    char name[64];        /* "--port" */
    char description[256];
    Value* default_val;   /* NULL for flags (default false) */
    int is_flag;          /* 1 = boolean flag, 0 = takes value */
} CliOption;

typedef struct {
    char name[64];        /* "start" */
    char description[256];
    Value* handler;       /* VAL_FUNCTION or VAL_BUILTIN */
} CliCommand;

#define MAX_OPTIONS  64
#define MAX_COMMANDS 32

static CliOption  options[MAX_OPTIONS];
static int        option_count = 0;
static CliCommand commands[MAX_COMMANDS];
static int        command_count = 0;

/* ========== Helpers ========== */

static void module_add_fn(Value* mod, const char* name, BuiltinFn fn, int min, int max) {
    Value* key = value_new_string(name);
    Value* fn_val = value_new_builtin(name, fn, min, max);
    dict_set(mod, key, fn_val);
}

/* Strip leading dashes from an option name for use as a dict key.
   e.g. "--port" -> "port", "-v" -> "v" */
static const char* strip_dashes(const char* name) {
    while (*name == '-') name++;
    return name;
}

/* Find a registered option by its raw name (with dashes).
   Matches against the full name stored in the options array. */
static CliOption* find_option(const char* arg) {
    for (int i = 0; i < option_count; i++) {
        if (strcmp(options[i].name, arg) == 0)
            return &options[i];
    }
    return NULL;
}

/* Skip past the melody binary and script filename in argv.
   Returns the index of the first "real" argument. */
static int skip_argv_prefix(Value* argv_list) {
    int count = argv_list->as.list.count;
    int start = 0;

    /* Skip at least the binary name (index 0) */
    if (count > 0) start = 1;

    /* If the next arg is "run", skip it too */
    if (start < count && argv_list->as.list.items[start]->type == VAL_STRING) {
        if (strcmp(argv_list->as.list.items[start]->as.string, "run") == 0) {
            start++;
        }
    }

    /* If the next arg ends with ".mdy", it's the script filename — skip it */
    if (start < count && argv_list->as.list.items[start]->type == VAL_STRING) {
        const char* s = argv_list->as.list.items[start]->as.string;
        size_t len = strlen(s);
        if (len >= 4 && strcmp(s + len - 4, ".mdy") == 0) {
            start++;
        }
    }

    return start;
}

/* ========== Builtin Functions ========== */

/* cli.args() -> list of all argv strings (raw) */
static Value* builtin_cli_args(Value** args, int argc) {
    return interp_get_argv();
}

/* cli.option(name, description, default?) -> registers a named option */
static Value* builtin_cli_option(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "cli.option(): name must be a string");
    if (args[1]->type != VAL_STRING)
        runtime_error(0, "cli.option(): description must be a string");

    if (option_count >= MAX_OPTIONS)
        runtime_error(0, "cli.option(): too many options registered (max %d)", MAX_OPTIONS);

    CliOption* opt = &options[option_count++];
    strncpy(opt->name, args[0]->as.string, sizeof(opt->name) - 1);
    opt->name[sizeof(opt->name) - 1] = '\0';
    strncpy(opt->description, args[1]->as.string, sizeof(opt->description) - 1);
    opt->description[sizeof(opt->description) - 1] = '\0';
    opt->is_flag = 0;
    opt->default_val = (argc >= 3) ? args[2] : value_new_null();

    return value_new_null();
}

/* cli.flag(name, description) -> registers a boolean flag */
static Value* builtin_cli_flag(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "cli.flag(): name must be a string");
    if (args[1]->type != VAL_STRING)
        runtime_error(0, "cli.flag(): description must be a string");

    if (option_count >= MAX_OPTIONS)
        runtime_error(0, "cli.flag(): too many options registered (max %d)", MAX_OPTIONS);

    CliOption* opt = &options[option_count++];
    strncpy(opt->name, args[0]->as.string, sizeof(opt->name) - 1);
    opt->name[sizeof(opt->name) - 1] = '\0';
    strncpy(opt->description, args[1]->as.string, sizeof(opt->description) - 1);
    opt->description[sizeof(opt->description) - 1] = '\0';
    opt->is_flag = 1;
    opt->default_val = value_new_bool(0);

    return value_new_null();
}

/* cli.parse() -> parses argv against registered options, returns dict */
static Value* builtin_cli_parse(Value** args, int argc) {
    Value* argv_list = interp_get_argv();
    Value* result = value_new_dict();

    /* 1. Set defaults for all registered options */
    for (int i = 0; i < option_count; i++) {
        const char* key = strip_dashes(options[i].name);
        Value* key_val = value_new_string(key);
        dict_set(result, key_val, options[i].default_val);
    }

    /* 2. Create the _positional list */
    Value* positional = value_new_list();

    /* 3. Skip past binary/script in argv */
    int start = skip_argv_prefix(argv_list);

    /* 4. Parse remaining args */
    for (int i = start; i < argv_list->as.list.count; i++) {
        Value* item = argv_list->as.list.items[i];
        if (item->type != VAL_STRING) continue;

        const char* arg = item->as.string;

        if (arg[0] == '-') {
            /* It's an option or flag */
            CliOption* opt = find_option(arg);
            const char* key = strip_dashes(arg);
            Value* key_val = value_new_string(key);

            if (opt && opt->is_flag) {
                /* Registered flag — set to true */
                dict_set(result, key_val, value_new_bool(1));
            } else if (opt && !opt->is_flag) {
                /* Registered option — take next arg as value */
                i++;
                if (i >= argv_list->as.list.count) {
                    runtime_error(0, "cli.parse(): option '%s' requires a value", arg);
                }
                Value* val_item = argv_list->as.list.items[i];
                if (val_item->type == VAL_STRING) {
                    /* Try to parse as integer */
                    char* endptr;
                    long long num = strtoll(val_item->as.string, &endptr, 10);
                    if (*endptr == '\0' && val_item->as.string[0] != '\0') {
                        dict_set(result, key_val, value_new_int(num));
                    } else {
                        dict_set(result, key_val, value_new_string(val_item->as.string));
                    }
                } else {
                    dict_set(result, key_val, val_item);
                }
            } else {
                /* Unregistered option — store anyway */
                /* Check if next arg looks like a value (doesn't start with -) */
                if (i + 1 < argv_list->as.list.count) {
                    Value* next = argv_list->as.list.items[i + 1];
                    if (next->type == VAL_STRING && next->as.string[0] != '-') {
                        i++;
                        char* endptr;
                        long long num = strtoll(next->as.string, &endptr, 10);
                        if (*endptr == '\0' && next->as.string[0] != '\0') {
                            dict_set(result, key_val, value_new_int(num));
                        } else {
                            dict_set(result, key_val, value_new_string(next->as.string));
                        }
                    } else {
                        /* No value follows — treat as flag */
                        dict_set(result, key_val, value_new_bool(1));
                    }
                } else {
                    /* Last arg, no value — treat as flag */
                    dict_set(result, key_val, value_new_bool(1));
                }
            }
        } else {
            /* Positional argument */
            list_append(positional, value_new_string(arg));
        }
    }

    /* 5. Store the _positional list */
    Value* pos_key = value_new_string("_positional");
    dict_set(result, pos_key, positional);

    return result;
}

/* cli.command(name, description, handler_fn) -> registers a subcommand */
static Value* builtin_cli_command(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "cli.command(): name must be a string");
    if (args[1]->type != VAL_STRING)
        runtime_error(0, "cli.command(): description must be a string");
    if (args[2]->type != VAL_FUNCTION && args[2]->type != VAL_BUILTIN)
        runtime_error(0, "cli.command(): handler must be a function");

    if (command_count >= MAX_COMMANDS)
        runtime_error(0, "cli.command(): too many commands registered (max %d)", MAX_COMMANDS);

    CliCommand* cmd = &commands[command_count++];
    strncpy(cmd->name, args[0]->as.string, sizeof(cmd->name) - 1);
    cmd->name[sizeof(cmd->name) - 1] = '\0';
    strncpy(cmd->description, args[1]->as.string, sizeof(cmd->description) - 1);
    cmd->description[sizeof(cmd->description) - 1] = '\0';
    cmd->handler = args[2];

    return value_new_null();
}

/* cli.run() -> matches argv to a registered command, calls its handler */
static Value* builtin_cli_run(Value** args, int argc) {
    Value* argv_list = interp_get_argv();
    int start = skip_argv_prefix(argv_list);

    /* Find first non-flag argument as the subcommand name */
    const char* subcmd = NULL;
    for (int i = start; i < argv_list->as.list.count; i++) {
        Value* item = argv_list->as.list.items[i];
        if (item->type != VAL_STRING) continue;
        if (item->as.string[0] != '-') {
            subcmd = item->as.string;
            break;
        }
        /* If it's a registered non-flag option, skip its value too */
        CliOption* opt = find_option(item->as.string);
        if (opt && !opt->is_flag) {
            i++; /* skip the value */
        }
    }

    if (!subcmd) {
        /* No subcommand found — print help listing */
        printf("Usage:\n");
        if (command_count > 0) {
            printf("\nCommands:\n");
            for (int i = 0; i < command_count; i++) {
                printf("  %-20s %s\n", commands[i].name, commands[i].description);
            }
        }
        if (option_count > 0) {
            printf("\nOptions:\n");
            for (int i = 0; i < option_count; i++) {
                if (options[i].is_flag) {
                    printf("  %-20s %s\n", options[i].name, options[i].description);
                } else {
                    printf("  %-20s %s\n", options[i].name, options[i].description);
                }
            }
        }
        return value_new_null();
    }

    /* Match against registered commands */
    for (int i = 0; i < command_count; i++) {
        if (strcmp(commands[i].name, subcmd) == 0) {
            return interp_eval_fn_call(commands[i].handler, NULL, 0);
        }
    }

    runtime_error(0, "Unknown command: '%s'", subcmd);
    return value_new_null(); /* unreachable, but satisfies compiler */
}

/* cli.reset() -> clears registered options/commands */
static Value* builtin_cli_reset(Value** args, int argc) {
    option_count = 0;
    command_count = 0;
    return value_new_null();
}

/* ========== Module Creation ========== */

Value* create_cli_module(void) {
    static Value* cached = NULL;
    if (cached) {
        /* Reset state on first creation only — subsequent calls return cache */
        return cached;
    }

    /* Reset internal state */
    option_count = 0;
    command_count = 0;

    Value* mod = value_new_dict();
    module_add_fn(mod, "args",    builtin_cli_args,    0, 0);
    module_add_fn(mod, "option",  builtin_cli_option,  2, 3);
    module_add_fn(mod, "flag",    builtin_cli_flag,    2, 2);
    module_add_fn(mod, "parse",   builtin_cli_parse,   0, 0);
    module_add_fn(mod, "command", builtin_cli_command,  3, 3);
    module_add_fn(mod, "run",     builtin_cli_run,     0, 0);
    module_add_fn(mod, "reset",   builtin_cli_reset,   0, 0);

    cached = mod;
    return mod;
}
