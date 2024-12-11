#ifndef ARENA_HEADER_
#define ARENA_HEADER_

#include <stddef.h>

typedef char unit;
_Static_assert(sizeof(unit) == 1, "");

#define ARENA_DEFAULT_SIZE 4096

typedef struct Arena Arena;
struct Arena {
    size_t cursor;
    size_t capacity;
    Arena* next; /* nullable */
    unit pool[]; /* flexible array member */
};

Arena *arena_new(size_t capacity);
void arena_destroy(Arena *a);
void *arena_alloc(Arena *a, size_t size);
void arena_clear(Arena *a);

#endif
