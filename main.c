#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "arena.h"
#include "object.h"
#include "eval.h"
#include ".build/stdlib.h"

int main(int argc, char *argv[])
{
    Env *env = env_new(NULL);
    env_add_default_variables(env);
    eval_program(stdlib, env, false);

    if (argc == 1) {
        Arena *lexer_arena = arena_new(0);

        for (;;) {
            printf("=> "); fflush(stdout);
            char* line = get_line(stdin);
            if (strcmp(line, "exit") == 0) {
                free(line); break;
            }
            if (strcmp(line, "") == 0) {
                free(line); continue;
            }

            int status; 
            if ((status = eval_program(line, env, true)) != 0) {
                //env_free(env);
                return status;
            }
            free(line);
            
        }

        GC_collect_garbage(NULL);
        arena_destroy(lexer_arena); 
    } else if (argc == 2) {
        FILE *f = fopen(argv[1], "r");
        if (f == NULL) { fprintf(stderr, "error opening file\n"); exit(-1); }
        char *program = file_to_str(f);
        fclose(f);


        int status = eval_program(program, env, false);
        GC_collect_garbage(NULL);
        free(program);
        return status;
    }

    GC_collect_garbage(NULL);
    return 0;
}

