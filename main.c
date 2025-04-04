#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "object.h"
#include "eval.h"
#include ".build/stdlib.h"
#include <readline/readline.h>
#include <readline/history.h>

int main(int argc, char *argv[])
{
    Env *env = env_new(NULL);
    env_add_default_variables(env);
    eval_program(stdlib, env, false);

    if (argc == 1 || argc == 2 && strcmp(argv[1], "-repl") == 0) {
        /* set up readline */
        rl_bind_key('\t', rl_complete); // autocomplete when tab is hit
        using_history();
        rl_variable_bind("blink-matching-paren", "On");

        for (;;) {
            char *line = readline("=> ");
            if (line == NULL) break;
            if (strcmp(line, "") == 0) {
                free(line); continue;
            }

            add_history(line);

            int status; 
            if ((status = eval_program(line, env, true)) != 0) {
                GC_collect_garbage(NULL);
                return status;
            }
            free(line);

        }
        GC_collect_garbage(NULL);

    } else if (argc == 2) {
        FILE *f = fopen(argv[1], "r");
        if (f == NULL) { fprintf(stderr, "error opening file\n"); exit(-1); }
        char *program = file_to_str(f);
        fclose(f);

        int status = eval_program(program, env, false);

        /* run the main function - if it isn't found then theres
         * an error */
        eval_program("(main)", env, false);
        GC_collect_garbage(NULL);
        free(program);
        return status;
    }

    GC_collect_garbage(NULL);
    return 0;
}

