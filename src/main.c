#include "interpreter.h"
#include "pm.h"
#include "repl.h"
#include "test_runner.h"
#include "formatter.h"
#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
  /* Avoid including <windows.h> to prevent TokenType clash with winnt.h */
  __declspec(dllimport) void __stdcall Sleep(unsigned long dwMilliseconds);
#else
  #include <unistd.h>
#endif

#define MELODY_VERSION "0.1.0"

static char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char* buf = (char*)malloc(size + 1);
    if (!buf) {
        fclose(f);
        fprintf(stderr, "Error: Out of memory\n");
        return NULL;
    }
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

static void print_usage(const char* prog) {
    printf("Melody Programming Language v%s\n\n", MELODY_VERSION);
    printf("Usage:\n");
    printf("  %s run <file>              Run a .mdy source file\n", prog);
    printf("  %s run --watch <file>      Run with auto-reload on change\n", prog);
    printf("  %s <file>                  Run a file (shorthand)\n", prog);
    printf("  %s repl                    Start interactive REPL\n", prog);
    printf("  %s init                    Initialize a new project\n", prog);
    printf("  %s install <source>        Install a package (github:user/repo)\n", prog);
    printf("  %s install                 Install all from melody.json\n", prog);
    printf("  %s uninstall <name>        Remove a package\n", prog);
    printf("  %s test [path]             Run tests\n", prog);
    printf("  %s fmt <file|dir>          Format .mdy files\n", prog);
    printf("  %s version                 Show version info\n", prog);
    printf("  %s                         Start REPL (no arguments)\n", prog);
}

int main(int argc, char* argv[]) {
    /* No arguments: start REPL */
    if (argc == 1) {
        repl_run();
        return 0;
    }

    const char* cmd = argv[1];

    /* melody version */
    if (strcmp(cmd, "version") == 0 || strcmp(cmd, "--version") == 0 || strcmp(cmd, "-v") == 0) {
        printf("Melody v%s\n", MELODY_VERSION);
        return 0;
    }

    /* melody help */
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    /* melody repl */
    if (strcmp(cmd, "repl") == 0) {
        repl_run();
        return 0;
    }

    /* melody init */
    if (strcmp(cmd, "init") == 0) {
        return pm_init();
    }

    /* melody install [source] */
    if (strcmp(cmd, "install") == 0) {
        if (argc < 3)
            return pm_install_all();
        return pm_install(argv[2]);
    }

    /* melody uninstall <name> */
    if (strcmp(cmd, "uninstall") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: 'uninstall' requires a package name\n");
            return 1;
        }
        return pm_uninstall(argv[2]);
    }

    /* melody test [path] */
    if (strcmp(cmd, "test") == 0) {
        const char* test_path = (argc >= 3) ? argv[2] : ".";
        return test_runner_run(test_path);
    }

    /* melody fmt <file|dir> [more...] */
    if (strcmp(cmd, "fmt") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: melody fmt <file.mdy|directory> [...]\n");
            return 1;
        }
        int errors = 0;
        for (int i = 2; i < argc; i++) {
            struct stat st;
            if (stat(argv[i], &st) != 0) {
                fprintf(stderr, "Error: '%s' not found\n", argv[i]);
                errors++;
                continue;
            }
            if (st.st_mode & S_IFDIR)
                errors += formatter_format_dir(argv[i]);
            else
                errors += (formatter_format_file(argv[i]) != 0) ? 1 : 0;
        }
        return errors > 0 ? 1 : 0;
    }

    /* melody run [--watch] <file> */
    if (strcmp(cmd, "run") == 0) {
        int watch = 0;
        const char* filename = NULL;

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--watch") == 0) watch = 1;
            else filename = argv[i];
        }
        if (!filename) {
            fprintf(stderr, "Error: 'run' requires a filename\n");
            return 1;
        }

        if (!watch) {
            char* source = read_file(filename);
            if (!source) return 1;
            Interpreter* interp = interp_new();
            interp_set_argv(argc, argv);
            int result = interp_exec(interp, source);
            interp_free(interp);
            free(source);
            return result;
        }

        /* Watch mode */
        struct stat st_watch;
        time_t last_mtime = 0;

        printf("[watching %s — Ctrl+C to stop]\n\n", filename);
        while (1) {
            if (stat(filename, &st_watch) == 0 && st_watch.st_mtime != last_mtime) {
                if (last_mtime != 0) printf("\n[re-running...]\n\n");
                last_mtime = st_watch.st_mtime;

                char* source = read_file(filename);
                if (source) {
                    Interpreter* interp = interp_new();
                    interp_set_argv(argc, argv);
                    interp_exec(interp, source);
                    interp_free(interp);
                    free(source);
                }
            }
#ifdef _WIN32
            Sleep(1000);
#else
            sleep(1);
#endif
        }
        return 0;
    }

    /* melody <file> (shorthand) */
    char* source = read_file(cmd);
    if (!source) {
        fprintf(stderr, "Unknown command: '%s'\n\n", cmd);
        print_usage(argv[0]);
        return 1;
    }

    Interpreter* interp = interp_new();
    interp_set_argv(argc, argv);
    int result = interp_exec(interp, source);
    interp_free(interp);
    free(source);
    return result;
}
