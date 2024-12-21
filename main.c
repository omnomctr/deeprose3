#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include <string.h>
#include "util.h"
#include "arena.h"
#include "object.h"
#include "parser.h"
#include "eval.h"
#include ".build/stdlib.h"

/* return malloc'ed string of line until newline */
char *get_line(FILE *);
char *file_to_str(FILE *f);

int eval_program(const char *program, Env *env /*nullable*/, bool print_eval)
{
    bool free_env = false;
    if (env == NULL) {
        free_env = true;
        env = env_new(NULL);
        env_add_default_variables(env);
    }

    Arena *parser_arena = arena_new(0);
    Lexer *lex = lexer_new(program, parser_arena);
    Parser *parser = parser_new(lex);

    Object *o = parser_parse(parser);

    for (;;) {
        if (parser->error) printf("parser has error \"%s\"", parser_error_string(parser)); 
        else if (o == NULL) break;
        else {
            if (print_eval) {
                object_print(eval(env, o));
                putchar('\n');
            } else eval(env, o);
        }

        if (parser_at_eof(parser)) break;
        o = parser_parse(parser);
        GC_collect_garbage(env, o);
    }
    
    if (free_env) {
        env_free(env);
        GC_collect_garbage(NULL);
    } else {
        GC_collect_garbage(env);
    }

    arena_destroy(parser_arena);
    return 0;
}

int main(int argc, char *argv[])
{
    Env *env = env_new(NULL);
    env_add_default_variables(env);
    eval_program(stdlib, env, false);

    if (argc == 2 && strcmp(argv[1], "-repl") == 0) {
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
                env_free(env);
                return status;
            }
            free(line);
        }

        env_free(env); 
        GC_collect_garbage(NULL);
        arena_destroy(lexer_arena); 
    } else if (argc == 2) {
        FILE *f = fopen(argv[1], "r");
        if (f == NULL) { fprintf(stderr, "error opening file\n"); exit(-1); }
        char *program = file_to_str(f);
        fclose(f);


        int status = eval_program(program, env, false);
        env_free(env);
        GC_collect_garbage(NULL);
        free(program);
        return status;
    }

    env_free(env);
    return 0;
}

char *get_line(FILE *f)
{
    // I LOVE VECTOR PATTERN!!!!
    struct { size_t len, capacity; char *ptr; } str = { .len = 0, .capacity = 1, .ptr = NULL };
    str.ptr = malloc(sizeof(char) * str.capacity);
    CHECK_ALLOC(str.ptr);

    int ch;
    while ((ch = fgetc(f)) != '\n') {
        /* C-D quits if its on a new line (standard unix behaviour) */ 
        if (ch == EOF && str.len == 0) { putchar('\n'); free(str.ptr); exit(0); }
        /* the plus one is for the null character at the end */
        if (str.len + 1 >= str.capacity) {
            str.capacity *= 2;
            str.ptr = realloc(str.ptr, sizeof(char) * str.capacity);
            /* normally you really should have a temporary ptr for realloc, bc realloc might fail and then you have a heap allocation you can't access, but I have made the executive decision that running out of memory crashes the program so it wont matter :) */
            CHECK_ALLOC(str.ptr);
        }

        str.ptr[str.len++] = (char)ch;
    }

    str.ptr[str.len++] = '\0';

    /* shrink vec */
    str.ptr = realloc(str.ptr, sizeof(char) * str.len);
    CHECK_ALLOC(str.ptr);
    
    return str.ptr;
}

char *file_to_str(FILE *f)
{
    fseek(f, 0, SEEK_END);
    size_t filesize = (size_t)ftell(f);
    rewind(f);


    char *ret = malloc(sizeof(char) * (filesize + 1));
    CHECK_ALLOC(ret);

    fread(ret, sizeof(char), filesize, f);
    ret[filesize] = '\0';

    return ret;
}
