#ifndef MELODY_INTERPRETER_H
#define MELODY_INTERPRETER_H

#include "parser.h"
#include "env.h"
#include <setjmp.h>

typedef enum {
    SIG_NONE,
    SIG_RETURN,
    SIG_BREAK,
    SIG_CONTINUE,
} Signal;

typedef struct TryFrame {
    jmp_buf buf;
    struct TryFrame* prev;
} TryFrame;

typedef struct {
    Env* global_env;
    Env* env;
    Signal signal;
    TryFrame* try_stack;
    Value* thrown_value;
} Interpreter;

/* Error reporting — accessible to stdlib modules */
void runtime_error(int line, const char* fmt, ...);

Interpreter* interp_new(void);
void interp_free(Interpreter* interp);

/* Execute source code string. Returns 0 on success, 1 on error. */
int interp_exec(Interpreter* interp, const char* source);

/* Execute an AST in the interpreter's current environment. */
Value* interp_eval(Interpreter* interp, ASTNode* node);

/* Set program arguments — call before interp_exec(). */
void interp_set_argv(int argc, char** argv);

/* Get argv as a Melody list (used by stdlib modules) */
Value* interp_get_argv(void);

/* Call a Melody function (VAL_FUNCTION or VAL_BUILTIN) from C code */
Value* interp_eval_fn_call(Value* fn, Value** args, int argc);

#endif
