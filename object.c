#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "object.h"
#include "util.h"
#include <sys/param.h>

static struct {
    size_t live_objects;
    size_t max_objects;
    Object *mark_sources;
    Object *obj_list;
} GC = {
    .live_objects = 0,
    .max_objects = INITIAL_OBJECT_GC_MAX,
    .mark_sources = NULL,
    .obj_list = NULL,
};


Object *object_new_generic(void) 
{
    Object *ret = malloc(sizeof(Object));
    CHECK_ALLOC(ret);
    DBG("creating object at %p", ret);
    ret->gc_mark = NOT_MARKED;
    ret->obj_next = GC.obj_list;
    GC.obj_list = ret;
    GC.live_objects++;
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

Object *object_num_new(int32_t num)
{
    Object *ret = object_new_generic();
    ret->kind = O_NUM;
    ret->num = num;
    return ret;
}

Object *object_nil_new(void)
{
    Object *ret = object_new_generic();
    ret->kind = O_NIL;
    return ret;
}

void object_free(Object *o)
{
    DBG("freeing object at %p", o);
    if (o->kind == O_STR || o->kind == O_IDENT) 
        free(o->str.ptr);
    free(o);
}

void object_print(Object *o)
{
    if (o == NULL) 
        printf("[null object]");
    else { 
        switch (o->kind) {
            case O_STR:
                printf("[string object: \"");
                for (size_t i = 0; i < o->str.len; i++)
                    putchar(o->str.ptr[i]);
                printf("\"]");
                break;
            case O_NUM:
                printf("[number object: %d]", o->num);
                break;
            case O_IDENT:
                printf("[identifier object: \"");
                for (size_t i = 0; i < o->str.len; i++)
                    putchar(o->str.ptr[i]);
                printf("\"]");
                break;
            case O_LIST:
                printf("("); 
                Object *cursor = o;
                for (;;) {
                    object_print(cursor->list.car);
                    cursor = cursor->list.cdr;
                    if (cursor->kind != O_LIST) break;
                    else putchar(' ');
                }
                printf(")");
                break;
            case O_NIL:
                printf("[nil object]");
        }
    }
}

static void _GC_mark(Object *o)
{
    if (o == NULL) {
        DBG("_GC_mark tried to mark a null object");
        return;
    }
    if (o->gc_mark == MARKED) { return; }
    o->gc_mark = MARKED;

    if (o->kind == O_LIST) {
        _GC_mark(o->list.car);
        _GC_mark(o->list.cdr);
    }
}

static void _GC_sweep(void)
{
    /* thank you baby's first garbage collector for showing me the proper way to do this
     * https://journal.stuffwithstuff.com/2013/12/08/babys-first-garbage-collector/
     * I originally did this wrong without the double ptr and paid the price in debuging time */
    Object **o = &GC.obj_list;
    while (*o) {
        DBG("sweeping %p", o);
        if ((*o)->gc_mark == NOT_MARKED) {
            Object *unreachable = *o;
            *o = unreachable->obj_next;
            object_free(unreachable);
            GC.live_objects--;
        } else {
            /* reset the object */
            (*o)->gc_mark = NOT_MARKED; 
            o = &(*o)->obj_next;
        }
    }
}

void GC_collect_garbage(void)
{
    _GC_mark(GC.mark_sources);
    _GC_sweep();
    GC.max_objects = GC.live_objects == 0 ? INITIAL_OBJECT_GC_MAX : MAX(GC.live_objects * 2, INITIAL_OBJECT_GC_MAX);
}

void GC_debug_print_status(void)
{
    fprintf(stderr, "GC status:\n"
            "\tlive objects: %zu\n"
            "\tmax objects: %zu\n",
           GC.live_objects, GC.max_objects);
}

int GC_add_mark_source(Object *mark_source)
{
    if (GC.mark_sources != NULL) return -1;
    else GC.mark_sources = mark_source;
    return 0;
}