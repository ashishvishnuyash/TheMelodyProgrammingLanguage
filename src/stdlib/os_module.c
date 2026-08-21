#include "os_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

/* Forward-declare from interpreter.c */
void runtime_error(int line, const char* fmt, ...);

#ifdef _WIN32
  #include <direct.h>
  #include <io.h>
  #define getcwd  _getcwd
  #define chdir   _chdir
  #define mkdir(p, m) _mkdir(p)
  #define popen   _popen
  #define pclose  _pclose
#else
  #include <unistd.h>
#endif

/* ========== Helpers ========== */

static void module_add_fn(Value* mod, const char* name, BuiltinFn fn, int min, int max) {
    Value* key = value_new_string(name);
    Value* fn_val = value_new_builtin(name, fn, min, max);
    dict_set(mod, key, fn_val);
}

/* ========== Functions ========== */

/* os.exec(cmd) → exit code */
static Value* builtin_os_exec(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "os.exec(): argument must be a string");
    int ret = system(args[0]->as.string);
    return value_new_int(ret);
}

/* os.capture(cmd) → string (stdout of command) */
static Value* builtin_os_capture(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "os.capture(): argument must be a string");

    FILE* fp = popen(args[0]->as.string, "r");
    if (!fp)
        runtime_error(0, "os.capture(): failed to run '%s'", args[0]->as.string);

    size_t cap = 4096, len = 0;
    char* buf = (char*)malloc(cap);
    size_t n;
    while ((n = fread(buf + len, 1, cap - len - 1, fp)) > 0) {
        len += n;
        if (len + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
    }
    buf[len] = '\0';
    pclose(fp);
    return value_new_string_take(buf);
}

/* os.env(name) → string or null */
static Value* builtin_os_env(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "os.env(): argument must be a string");
    const char* val = getenv(args[0]->as.string);
    if (!val) return value_new_null();
    return value_new_string(val);
}

/* os.setenv(name, value) → null */
static Value* builtin_os_setenv(Value** args, int argc) {
    if (args[0]->type != VAL_STRING || args[1]->type != VAL_STRING)
        runtime_error(0, "os.setenv(): both arguments must be strings");
#ifdef _WIN32
    _putenv_s(args[0]->as.string, args[1]->as.string);
#else
    setenv(args[0]->as.string, args[1]->as.string, 1);
#endif
    return value_new_null();
}

/* os.cwd() → string */
static Value* builtin_os_cwd(Value** args, int argc) {
    char buf[4096];
    if (!getcwd(buf, sizeof(buf)))
        runtime_error(0, "os.cwd(): failed to get working directory");
    return value_new_string(buf);
}

/* os.chdir(path) → null */
static Value* builtin_os_chdir(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "os.chdir(): argument must be a string");
    if (chdir(args[0]->as.string) != 0)
        runtime_error(0, "os.chdir(): cannot change to '%s'", args[0]->as.string);
    return value_new_null();
}

/* os.exit(code?) → never returns */
static Value* builtin_os_exit(Value** args, int argc) {
    int code = 0;
    if (argc > 0 && args[0]->type == VAL_INT)
        code = (int)args[0]->as.integer;
    exit(code);
    return value_new_null();
}

/* os.platform() → "windows" / "linux" / "macos" */
static Value* builtin_os_platform(Value** args, int argc) {
#if defined(_WIN32)
    return value_new_string("windows");
#elif defined(__APPLE__)
    return value_new_string("macos");
#elif defined(__linux__)
    return value_new_string("linux");
#else
    return value_new_string("unknown");
#endif
}

/* os.mkdir(path) → null */
static Value* builtin_os_mkdir(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "os.mkdir(): argument must be a string");
    if (mkdir(args[0]->as.string, 0755) != 0)
        runtime_error(0, "os.mkdir(): cannot create '%s'", args[0]->as.string);
    return value_new_null();
}

/* os.remove(path) → null */
static Value* builtin_os_remove(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "os.remove(): argument must be a string");
    if (remove(args[0]->as.string) != 0)
        runtime_error(0, "os.remove(): cannot remove '%s'", args[0]->as.string);
    return value_new_null();
}

/* os.listdir(path) → list of filenames */
static Value* builtin_os_listdir(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "os.listdir(): argument must be a string");

    DIR* dir = opendir(args[0]->as.string);
    if (!dir)
        runtime_error(0, "os.listdir(): cannot open '%s'", args[0]->as.string);

    Value* result = value_new_list();
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        list_append(result, value_new_string(entry->d_name));
    }
    closedir(dir);
    return result;
}

/* ========== Module Creation ========== */

Value* create_os_module(void) {
    static Value* cached = NULL;
    if (cached) return cached;

    Value* mod = value_new_dict();
    module_add_fn(mod, "exec",     builtin_os_exec,     1, 1);
    module_add_fn(mod, "capture",  builtin_os_capture,  1, 1);
    module_add_fn(mod, "env",      builtin_os_env,      1, 1);
    module_add_fn(mod, "setenv",   builtin_os_setenv,   2, 2);
    module_add_fn(mod, "cwd",      builtin_os_cwd,      0, 0);
    module_add_fn(mod, "chdir",    builtin_os_chdir,    1, 1);
    module_add_fn(mod, "exit",     builtin_os_exit,     0, 1);
    module_add_fn(mod, "platform", builtin_os_platform, 0, 0);
    module_add_fn(mod, "mkdir",    builtin_os_mkdir,    1, 1);
    module_add_fn(mod, "remove",   builtin_os_remove,   1, 1);
    module_add_fn(mod, "listdir",  builtin_os_listdir,  1, 1);

    cached = mod;
    return mod;
}
