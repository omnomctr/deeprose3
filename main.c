#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include <string.h>
#include "util.h"
#include "arena.h"

/* return malloc'ed string of line until newline */
char *get_line(FILE *);

int main(int argc, char *argv[])
{
    Arena *lexer_arena = arena_new(sizeof(Lexer) * 2);
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

        Token *t;
        do {
            t = lexer_next_token(lex);
            token_print(t);
            putchar('\n');
        } while (t->type != t_EOF);
        arena_clear(lexer_arena);
        free(line);
    }

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
    while ((ch = fgetc(f)) != EOF && ch != '\n') {
        if (str.len >= str.capacity) {
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
