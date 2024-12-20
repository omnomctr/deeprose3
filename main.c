#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include <string.h>
#include "util.h"
#include "arena.h"
#include "object.h"
#include "parser.h"
#include "eval.h"

/* return malloc'ed string of line until newline */
char *get_line(FILE *);

int main(int argc, char *argv[])
{
    Arena *lexer_arena = arena_new(0);
    Env *env = env_new(NULL);
    env_add_default_builtin_functions(env);

    for (;;) {
        printf("=> "); fflush(stdout);
        char* line = get_line(stdin);
        if (strcmp(line, "exit") == 0) {
            free(line); break;
        }
        if (strcmp(line, "") == 0) {
            free(line); continue;
        }

        Lexer *lex = lexer_new(line, lexer_arena);
        Parser *parser = parser_new(lex);
        
        Object *o = parser_parse(parser);
        for (;;) {
            if (parser->error) { printf("parser has error \"%s\"", parser_error_string(parser)); }
            else if (o == NULL) break;
            else object_print(eval(env, o)); 

            putchar('\n');

            if (parser_at_eof(parser)) break;
            o = parser_parse(parser);
        }

        GC_collect_garbage(env);

        arena_clear(lexer_arena);
        free(line);
    }

    env_free(env); 
    GC_collect_garbage(NULL);
    arena_destroy(lexer_arena); 
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
