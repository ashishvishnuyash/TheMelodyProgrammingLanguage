#include "time_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Forward-declare from interpreter.c — avoids pulling in lexer.h
   which clashes with Windows SDK's TokenType in winnt.h. */
void runtime_error(int line, const char* fmt, ...);

#ifdef _WIN32
  #include <windows.h>
#else
  #include <unistd.h>
  #include <sys/time.h>
#endif

/* ========== Helpers ========== */

static void module_add_fn(Value* mod, const char* name, BuiltinFn fn, int min, int max) {
    Value* key = value_new_string(name);
    Value* fn_val = value_new_builtin(name, fn, min, max);
    dict_set(mod, key, fn_val);
}

/* ========== Functions ========== */

/* time.now() → float (unix timestamp with sub-second precision) */
static Value* builtin_time_now(Value** args, int argc) {
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    /* FILETIME = 100-nanosecond intervals since 1601-01-01 */
    unsigned long long t = ((unsigned long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    /* Convert to Unix epoch (seconds since 1970-01-01) */
    double secs = (double)(t - 116444736000000000ULL) / 10000000.0;
    return value_new_float(secs);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return value_new_float(tv.tv_sec + tv.tv_usec / 1e6);
#endif
}

/* time.clock() → float (monotonic high-res timer for benchmarking) */
static Value* builtin_time_clock(Value** args, int argc) {
#ifdef _WIN32
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return value_new_float((double)count.QuadPart / (double)freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return value_new_float(ts.tv_sec + ts.tv_nsec / 1e9);
#endif
}

/* time.sleep(seconds) → null */
static Value* builtin_time_sleep(Value** args, int argc) {
    double secs;
    if (args[0]->type == VAL_INT)
        secs = (double)args[0]->as.integer;
    else if (args[0]->type == VAL_FLOAT)
        secs = args[0]->as.floating;
    else {
        runtime_error(0, "time.sleep(): argument must be a number");
        return value_new_null();
    }

    if (secs < 0) secs = 0;

#ifdef _WIN32
    Sleep((DWORD)(secs * 1000));
#else
    struct timespec ts;
    ts.tv_sec  = (time_t)secs;
    ts.tv_nsec = (long)((secs - (double)ts.tv_sec) * 1e9);
    nanosleep(&ts, NULL);
#endif
    return value_new_null();
}

/* time.format(timestamp, fmt) → string
   Uses strftime formatting. timestamp is seconds since epoch. */
static Value* builtin_time_format(Value** args, int argc) {
    double ts;
    if (args[0]->type == VAL_INT)
        ts = (double)args[0]->as.integer;
    else if (args[0]->type == VAL_FLOAT)
        ts = args[0]->as.floating;
    else {
        runtime_error(0, "time.format(): first argument must be a number");
        return value_new_null();
    }

    if (args[1]->type != VAL_STRING)
        runtime_error(0, "time.format(): second argument must be a format string");

    time_t t = (time_t)ts;
    struct tm* tm_info = localtime(&t);
    if (!tm_info)
        runtime_error(0, "time.format(): invalid timestamp");

    char buf[512];
    if (strftime(buf, sizeof(buf), args[1]->as.string, tm_info) == 0)
        runtime_error(0, "time.format(): format produced empty result");

    return value_new_string(buf);
}

/* time.millis() → int (milliseconds since epoch — handy for IDs/timing) */
static Value* builtin_time_millis(Value** args, int argc) {
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    unsigned long long t = ((unsigned long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    long long ms = (long long)((t - 116444736000000000ULL) / 10000);
    return value_new_int(ms);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long ms = (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return value_new_int(ms);
#endif
}

/* ========== Module Creation ========== */

Value* create_time_module(void) {
    static Value* cached = NULL;
    if (cached) return cached;

    Value* mod = value_new_dict();
    module_add_fn(mod, "now",    builtin_time_now,    0, 0);
    module_add_fn(mod, "clock",  builtin_time_clock,  0, 0);
    module_add_fn(mod, "sleep",  builtin_time_sleep,  1, 1);
    module_add_fn(mod, "format", builtin_time_format, 2, 2);
    module_add_fn(mod, "millis", builtin_time_millis, 0, 0);

    cached = mod;
    return mod;
}
