#ifndef UTIL_HEADER__
#define UTIL_HEADER__

#include <assert.h>
#include <stdio.h>

#define CHECK_ALLOC(ptr) \
    assert((ptr) && "ran out of memory");

#ifdef DEBUG
#define DBG(fmt, ...) \
    fprintf(stderr, fmt "\n" __VA_OPT__(,) __VA_ARGS__);
#else
#define DBG(...)
#endif

/* return malloc'ed string of line until newline */
char *get_line(FILE *);
char *file_to_str(FILE *f);

#define da_append(vec, elem) \
    do {\
        if ((vec).len >= (vec).capacity) {\
            (vec).capacity *= 2;\
            (vec).ptr = realloc((vec).ptr, sizeof(*(vec).ptr) * (vec).capacity);\
            CHECK_ALLOC((vec).ptr);\
        }\
        (vec).ptr[(vec).len++] = elem;\
    } while (0)

#endif
