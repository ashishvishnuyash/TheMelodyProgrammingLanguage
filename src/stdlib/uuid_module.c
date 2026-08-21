#include "uuid_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

void runtime_error(int line, const char* fmt, ...);

static void module_add_fn(Value* mod, const char* name, BuiltinFn fn, int min, int max) {
    Value* key = value_new_string(name);
    Value* fn_val = value_new_builtin(name, fn, min, max);
    dict_set(mod, key, fn_val);
}

static int seeded = 0;

static Value* builtin_uuid_v4(Value** args, int argc) {
    if (!seeded) { srand((unsigned int)(time(NULL) ^ clock())); seeded = 1; }
    uint8_t bytes[16];
    for (int i = 0; i < 16; i++) bytes[i] = (uint8_t)(rand() & 0xFF);
    bytes[6] = (bytes[6] & 0x0F) | 0x40;
    bytes[8] = (bytes[8] & 0x3F) | 0x80;
    char uuid[37];
    snprintf(uuid, sizeof(uuid),
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        bytes[0],bytes[1],bytes[2],bytes[3],bytes[4],bytes[5],bytes[6],bytes[7],
        bytes[8],bytes[9],bytes[10],bytes[11],bytes[12],bytes[13],bytes[14],bytes[15]);
    return value_new_string(uuid);
}

Value* create_uuid_module(void) {
    static Value* cached = NULL;
    if (cached) return cached;
    Value* mod = value_new_dict();
    module_add_fn(mod, "v4", builtin_uuid_v4, 0, 0);
    cached = mod;
    return mod;
}
