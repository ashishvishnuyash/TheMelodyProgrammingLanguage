#include "repl.h"
#include "interpreter.h"
#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MELODY_VERSION "0.1.0"

/* Count net open braces to support multi-line input */
static int count_open_braces(const char* s) {
    int depth = 0;
    int in_string = 0;
    for (int i = 0; s[i]; i++) {
        if (s[i] == '"' && (i == 0 || s[i-1] != '\\')) in_string = !in_string;
        if (!in_string) {
            if (s[i] == '{') depth++;
            else if (s[i] == '}') depth--;
        }
    }
    return depth;
}

void repl_run(void) {
    printf("Melody v%s - Interactive REPL\n", MELODY_VERSION);
    printf("Type 'exit' to quit.\n\n");

    Interpreter* interp = interp_new();
    char line[4096];

    while (1) {
        printf(">>> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\r\n")] = '\0';

        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) break;
        if (strlen(line) == 0) continue;

        /* Build input, handling multi-line blocks */
        int cap = (int)strlen(line) + 64;
        char* input = (char*)malloc(cap);
        strcpy(input, line);

        while (count_open_braces(input) > 0) {
            printf("... ");
            fflush(stdout);
            if (!fgets(line, sizeof(line), stdin)) break;
            line[strcspn(line, "\r\n")] = '\0';

            int new_len = (int)strlen(input) + (int)strlen(line) + 2;
            if (new_len >= cap) { cap = new_len + 64; input = (char*)realloc(input, cap); }
            strcat(input, "\n");
            strcat(input, line);
        }

        /* Execute and show result for expressions */
        interp_exec(interp, input);
        free(input);
    }

    interp_free(interp);
    printf("Goodbye.\n");
}
