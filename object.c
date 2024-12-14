#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "object.h"
#include "util.h"
#include <sys/param.h>

static struct {
    size_t live_objects;
    size_t max_objects;
    bool enabled;
    Object *mark_sources;
    Object *obj_list;
} GC = {
    .live_objects = 0,
    .max_objects = INITIAL_OBJECT_GC_MAX,
    .enabled = false,
    .mark_sources = NULL,
    .obj_list = NULL,
};


Object *object_new_generic(void) 
{
    Object *ret = malloc(sizeof(Object));
    CHECK_ALLOC(ret);
    ret->gc_mark = NOT_MARKED;
    ret->obj_next = GC.obj_list;
    GC.obj_list = ret;
    if (++GC.live_objects > GC.max_objects) GC_collect_garbage();
    return ret;
}

Object *object_string_slice_new(const char *s, size_t len)
{
    Object *ret = object_new_generic();
    ret->kind = O_STR;
    ret->str = (struct StringSlice) {
        .len = len,
        .capacity = len,
    };

    ret->str.ptr = malloc(sizeof(char) * len);
    CHECK_ALLOC(ret->str.ptr);
    memcpy(ret->str.ptr, s, len);

    return ret;
}

Object *object_list_new(Object *car, Object *cdr) 
{
    Object *ret = object_new_generic();
    ret->kind = O_LIST;
    ret->list = (struct List) {
        .car = car,
        .cdr = cdr,
    };

    return ret;
}

void object_free(Object *o)
{
    if (o->kind == O_STR || o->kind == O_IDENT) 
        free(o->str.ptr);
    free(o);
}

static void GC_mark(Object *o)
{
    if (o == NULL) {
        DBG("GC_mark tried to mark a null object");
        return;
    }
    if (o->gc_mark == MARKED) return;
    o->gc_mark = MARKED;

    if (o->kind == O_LIST) {
        GC_mark(o->list.car);
        GC_mark(o->list.cdr);
    }
}

static void GC_sweep(void)
{
    Object *o = GC.obj_list;
    while (o) {
        if (o->gc_mark == NOT_MARKED) {
            Object *unreachable = o;
            o = unreachable->obj_next;
            object_free(unreachable);
            GC.live_objects--;
        } else {
            /* reset the object */
            o->gc_mark = NOT_MARKED; 
            o = o->obj_next;
        }
    }
}

void GC_collect_garbage(void)
{
    if (!GC.enabled) return;
    GC_mark(GC.mark_sources);
    GC_sweep();
    GC.max_objects = GC.live_objects == 0 ? INITIAL_OBJECT_GC_MAX : MAX(GC.live_objects * 2, INITIAL_OBJECT_GC_MAX);
}

void GC_enable(bool yes)
{
    GC.enabled = yes;
}

void GC_debug_print_status(void)
{
    fprintf(stderr, "GC status:\n"
            "\tenabled: %s\n"
            "\tlive objects: %zu\n"
            "\tmax objects: %zu\n",
            GC.enabled ? "true" : "false", GC.live_objects, GC.max_objects);
}

int  GC_add_mark_source(Object *mark_source)
{
    if (GC.mark_sources != NULL) return -1;
    else GC.mark_sources = mark_source;
    return 0;
}
