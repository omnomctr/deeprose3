#include <stdio.h>
#include <stdlib.h>
#include "util.h"

char *get_line(FILE *f)
{
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
