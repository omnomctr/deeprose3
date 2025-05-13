#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "util.h"
#include "environment.h"
#include "object.h"

/* env_new() is in object.h because it needs to interact with
 * static garbage collector state */

static void _evstore_free(EnvValueStore *evs)
{
    if (evs->left) _evstore_free(evs->left);
    if (evs->right) _evstore_free(evs->right);

    free(evs);
}

void env_free(Env *e)
{
    if (e->store) _evstore_free(e->store);
    free(e);
}

static int _stringslice_cmp(struct StringSlice a, struct StringSlice b)
{
    return (a.len == b.len) ? memcmp(a.ptr, b.ptr, a.len) : a.len - b.len;
}

static EnvValueStore *_evstore_insert(EnvValueStore *cur, EnvValueStore *to_insert)
{
    if (cur == NULL) return to_insert;

    int cmp = _stringslice_cmp(to_insert->ident->str, cur->ident->str);
    if (cmp == 0) /* equal */ {
        // ensure the lower parts of the btree are intact
        to_insert->left = cur->left;
        to_insert->right = cur->right;
        free(cur); // were done with cur since were replacing it
        return to_insert;
    } else if (cmp < 0) /* to_insert < cur */ {
        cur->left = _evstore_insert(cur->left, to_insert);
    } else /* to_insert > cur */ {
        cur->right = _evstore_insert(cur->right, to_insert);
    }

    return cur;
}

void env_put(Env *e, Object *ident, Object *value)
{
    assert(ident->kind == O_IDENT);
    EnvValueStore *to_insert = malloc(sizeof(EnvValueStore));
    CHECK_ALLOC(to_insert);

    *to_insert = (struct EnvValueStore) {
        .ident = ident,
        .value = value,
    };

    e->store = _evstore_insert(e->store, to_insert); 
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

