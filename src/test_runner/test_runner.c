#include "test_runner.h"
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include "value.h"
#include "env.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#ifdef _WIN32
  #define PATH_SEP '\\'
#else
  #define PATH_SEP '/'
#endif

/* ========== File discovery ========== */

#define MAX_TEST_FILES 256

static int is_test_filename(const char* name) {
    int len = (int)strlen(name);
    /* Must end in .mdy */
    if (len < 5 || strcmp(name + len - 4, ".mdy") != 0) return 0;
    /* test_*.mdy */
    if (strncmp(name, "test_", 5) == 0) return 1;
    /* *_test.mdy */
    if (len >= 9 && strncmp(name + len - 9, "_test.mdy", 9) == 0) return 1;
    return 0;
}

static int collect_test_files(const char* dir_path, char files[][512], int count) {
    DIR* dir = opendir(dir_path);
    if (!dir) return count;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL && count < MAX_TEST_FILES) {
        if (entry->d_name[0] == '.') continue;

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s%c%s", dir_path, PATH_SEP, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) continue;

        if (st.st_mode & S_IFDIR) {
            count = collect_test_files(full_path, files, count);
        } else if (is_test_filename(entry->d_name)) {
            strncpy(files[count], full_path, 511);
            files[count][511] = '\0';
            count++;
        }
    }
    closedir(dir);
    return count;
}

/* ========== Read file ========== */

static char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* buf = (char*)malloc(size + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

/* ========== Run tests in a single file ========== */

static int run_test_file(const char* path, int* total, int* passed) {
    char* source = read_file(path);
    if (!source) {
        fprintf(stderr, "  Cannot read %s\n", path);
        return -1;
    }

    /* Lex and parse — keep AST alive so fn bodies remain valid */
    Lexer* lex = lexer_new(source);
    Token* tokens = lexer_tokenize(lex);
    int token_count = lexer_token_count(lex);
    ASTNode* program = parse(tokens, token_count);

    Interpreter* interp = interp_new();
    interp_eval(interp, program);

    /* Find all test_ functions in global env */
    Env* env = interp->global_env;
    for (int i = 0; i < env->capacity; i++) {
        if (!env->entries[i].occupied) continue;
        if (strncmp(env->entries[i].key, "test_", 5) != 0) continue;

        Value* fn = env->entries[i].value;
        if (fn->type != VAL_FUNCTION) continue;

        const char* test_name = env->entries[i].key;
        (*total)++;

        /* Run with try/catch via setjmp */
        TryFrame frame;
        frame.prev = interp->try_stack;
        interp->try_stack = &frame;

        if (setjmp(frame.buf) == 0) {
            interp_eval_fn_call(fn, NULL, 0);
            interp->try_stack = frame.prev;
            printf("  %s ", test_name);
            int dots = 30 - (int)strlen(test_name);
            for (int d = 0; d < dots; d++) putchar('.');
            printf(" PASS\n");
            (*passed)++;
        } else {
            interp->try_stack = frame.prev;
            printf("  %s ", test_name);
            int dots = 30 - (int)strlen(test_name);
            for (int d = 0; d < dots; d++) putchar('.');
            if (interp->thrown_value && interp->thrown_value->type == VAL_STRING)
                printf(" FAIL: %s\n", interp->thrown_value->as.string);
            else
                printf(" FAIL\n");
            interp->thrown_value = NULL;
            interp->signal = SIG_NONE;
        }
    }

    /* Now safe to free everything */
    ast_free(program);
    lexer_free(lex);
    free(source);
    interp_free(interp);
    return 0;
}

/* ========== Public API ========== */

int test_runner_run(const char* path) {
    struct stat st;
    int total = 0, passed_count = 0;

    printf("Running tests...\n\n");

    if (stat(path, &st) != 0) {
        fprintf(stderr, "Error: path '%s' not found\n", path);
        return 1;
    }

    if (st.st_mode & S_IFDIR) {
        /* Collect all test files */
        char (*files)[512] = (char(*)[512])malloc(MAX_TEST_FILES * 512);
        int file_count = collect_test_files(path, files, 0);

        for (int i = 0; i < file_count; i++) {
            /* Print relative path */
            printf("%s\n", files[i]);
            run_test_file(files[i], &total, &passed_count);
            printf("\n");
        }
        free(files);
    } else {
        printf("%s\n", path);
        run_test_file(path, &total, &passed_count);
        printf("\n");
    }

    int failed = total - passed_count;
    printf("%d tests, %d passed, %d failed\n", total, passed_count, failed);
    return (failed > 0) ? 1 : 0;
}
