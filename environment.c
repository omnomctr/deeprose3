#include <string.h>
#include <assert.h>
#include "environment.h"
#include "object.h"

Env *env_new(Env *parent)
{
    Arena *arena = arena_new(sizeof(Env) + sizeof(EnvValueStore) * 10);
    Env *ret = arena_alloc(arena, sizeof(Env));
    *ret = (Env) {
        .parent = parent,
        .arena = arena,
        .store = NULL,
    };

    return ret;
}


void env_free(Env *e)
{
    arena_destroy(e->arena);
}

void env_put(Env *e, Object *ident, Object *value)
{
    assert(ident->kind == O_IDENT);
    EnvValueStore **cursor = &e->store;
    while (*cursor != NULL) {
       if (ident->str.len == (*cursor)->ident->str.len 
               && memcmp(ident->str.ptr, (*cursor)->ident->str.ptr, ident->str.len) == 0) {
           /* overwrite the value */
           (*cursor)->value = value;
           return;
       }
       cursor = &(*cursor)->next;
    }
    
    assert(*cursor == NULL);
    *cursor = arena_alloc(e->arena, sizeof(EnvValueStore));
    **cursor = (struct EnvValueStore) {
        .ident = ident,
        .value = value,
    };
}


Object *env_get(Env *e, Object *ident)
{
    assert(ident->kind == O_IDENT);
    EnvValueStore *cursor = e->store;
    while (cursor != NULL) {
        if (ident->str.len == cursor->ident->str.len
                && memcmp(ident->str.ptr, cursor->ident->str.ptr, ident->str.len) == 0) {
            return cursor->value;
        }
        cursor = cursor->next;
    }

    if (e->parent != NULL) return env_get(e->parent, ident);
    else return object_error_new("identifier not found");
}

