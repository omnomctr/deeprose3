#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "util.h"
#include "environment.h"
#include "object.h"

/* env_new() is in object.h because it needs to interact with
 * static garbage collector state */

void env_free(Env *e)
{
    arena_destroy(e->arena);
}

static int _stringslice_cmp(struct StringSlice a, struct StringSlice b)
{
    return (a.len == b.len) ? memcmp(a.ptr, b.ptr, a.len) : a.len - b.len;
}

static EnvValueStore *_evstore_insert(EnvValueStore *cur, Object *ident, Object *val, Arena *a)
{
    if (cur == NULL) {
        EnvValueStore *to_insert = arena_alloc(a, sizeof(EnvValueStore));
        CHECK_ALLOC(to_insert);
        
        *to_insert = (struct EnvValueStore) {
            .ident = ident,
            .value = val
        };

        return to_insert;
    }

    int cmp = _stringslice_cmp(ident->str, cur->ident->str);
    if (cmp == 0) /* equal */ {
        *cur = (struct EnvValueStore) {
            .ident = ident,
            .value = val,
        };
        return cur;
    } else if (cmp < 0) /* to_insert < cur */ {
        cur->left = _evstore_insert(cur->left, ident, val, a);
    } else /* to_insert > cur */ {
        cur->right = _evstore_insert(cur->right, ident, val, a);
    }

    return cur;
}

void env_put(Env *e, Object *ident, Object *value)
{
    assert(ident->kind == O_IDENT);
    e->store = _evstore_insert(e->store, ident, value, e->arena); 
}

Object *env_get(Env *e, Object *ident)
{
    assert(ident->kind == O_IDENT);
    EnvValueStore *cursor = e->store;
    while (cursor != NULL) {
        int cmp = _stringslice_cmp(ident->str, cursor->ident->str);
        if (cmp == 0) return cursor->value;
        else if (cmp < 0) cursor = cursor->left;
        else cursor = cursor->right;
    }

    if (e->parent != NULL) return env_get(e->parent, ident);
    else return object_error_new("identifier \"%s\" not found", ident);
}

