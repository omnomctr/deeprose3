#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "object.h"
#include "util.h"
#include <sys/param.h>
#include <stdarg.h>
#include "eval.h"

static struct {
    size_t live_objects;
    size_t live_environments;
    Object *obj_list;
    Env *env_list;
} GC = {
    .live_objects = 0,
    .live_environments = 0,
    .obj_list= NULL,
    .env_list = NULL,
};

const char * const object_type_string[] = {
   [O_STR] = "string", 
   [O_NUM] = "number", 
   [O_LIST] = "list", 
   [O_IDENT] = "identifier", 
   [O_NIL] = "nil", 
   [O_ERROR] = "error",
   [O_BUILTIN] = "builtin",
   [O_FUNCTION] = "function",
   [O_CHAR] = "character",
};

const char *object_type_as_string(enum ObjectKind k)
{
    return object_type_string[k];
}

Object *object_new_generic(void) 
{
    Object *ret = malloc(sizeof(Object));
    CHECK_ALLOC(ret);
    DBG("creating object at %p", ret);
    ret->eval = true;
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

Object *object_ident_new(const char *s, size_t len)
{
    Object *ret = object_string_slice_new(s, len);
    ret->kind = O_IDENT;
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

Object *object_builtin_new(Builtin f)
{
    Object *ret = object_new_generic();
    ret->kind = O_BUILTIN;
    ret->builtin = f;
    return ret;
}

Object *object_num_new(int64_t num)
{
    Object *ret = object_new_generic();
    ret->kind = O_NUM;
    ret->list.car = NULL;
    ret->list.cdr = NULL;
    ret->num = num;
    return ret;
}

Object *object_nil_new(void)
{
    Object *ret = object_new_generic();
    ret->kind = O_NIL;
    return ret;
}

Object *object_error_new(const char *fmt, ...)
{
    Object *ret = object_new_generic();
    ret->kind = O_ERROR;
    
    ret->str.capacity = 10;
    ret->str.len = 0;
    ret->str.ptr = malloc(sizeof(char) * ret->str.capacity);
    CHECK_ALLOC(ret->str.ptr);

    va_list ap;
    va_start(ap, fmt);
    while (*fmt) 
        if (*fmt == '%') {
            fmt++;
            switch (*fmt++) { 
                case '%': {
                    if (ret->str.len >= ret->str.capacity) {
                        ret->str.capacity *= 2;
                        ret->str.ptr = realloc(ret->str.ptr, sizeof(char) * ret->str.capacity);
                        CHECK_ALLOC(ret->str.ptr);                       
                    }
                    ret->str.ptr[ret->str.len++] = '%';
                    fmt++;
                } break;
                case 's': {
                    if (*fmt == 'c') /* null-terminated c string */ {
                        fmt++;
                        const char *str = va_arg(ap, char *);
                        size_t len = strlen(str);
                        if (ret->str.len + len >= ret->str.capacity) {
                            ret->str.capacity += len;
                            ret->str.ptr = realloc(ret->str.ptr, sizeof(char) * ret->str.capacity);
                            CHECK_ALLOC(ret->str.ptr);
                        }
                        memcpy(ret->str.ptr + ret->str.len, str, len);
                        ret->str.len += len;
                    } else {
                        Object *string = va_arg(ap, Object *);
                        assert(string->kind == O_STR || string->kind == O_IDENT);
                        if (ret->str.len + string->str.len >= ret->str.capacity) {
                            ret->str.capacity += string->str.len;
                            ret->str.ptr = realloc(ret->str.ptr, sizeof(char) * ret->str.capacity);
                            CHECK_ALLOC(ret->str.ptr);
                        }
                        memcpy(ret->str.ptr + ret->str.len, string->str.ptr, string->str.len);
                        ret->str.len += string->str.len;
                    }
                } break;
                case 'd': {
                    Object *num = va_arg(ap, Object *);
                    assert(num->kind == O_NUM);
                    char *num_cstr = malloc(sizeof(char) * ERROR_NUM_MAX_STR_SIZE);
                    CHECK_ALLOC(num_cstr);
                    snprintf(num_cstr, ERROR_NUM_MAX_STR_SIZE, "%ld", num->num);
                    size_t len = strlen(num_cstr);

                    if (ret->str.len + len >= ret->str.capacity) {
                        ret->str.capacity += len;
                        ret->str.ptr = realloc(ret->str.ptr, sizeof(char) * ret->str.capacity);
                        CHECK_ALLOC(ret->str.ptr);
                    }
                    memcpy(ret->str.ptr + ret->str.len, num_cstr, len);
                    ret->str.len += len;
                    free(num_cstr);
                } break;
                case 'c': {
                    if (ret->str.len >= ret->str.capacity) {
                        ret->str.capacity *= 2;
                        ret->str.ptr = realloc(ret->str.ptr, sizeof(char) * ret->str.capacity);
                        CHECK_ALLOC(ret->str.ptr);
                    }
                    int c = va_arg(ap, int);
                    ret->str.ptr[ret->str.len++] = (char)c;
                } break;
                default: assert(0);
            } 
        } else {
            if (ret->str.len >= ret->str.capacity) {
                ret->str.capacity *= 2;
                ret->str.ptr = realloc(ret->str.ptr, sizeof(char) * ret->str.capacity);
                CHECK_ALLOC(ret->str.ptr);
            }
            ret->str.ptr[ret->str.len++] = *fmt++;
        }

    ret->str.capacity = ret->str.len;
    ret->str.ptr = realloc(ret->str.ptr, sizeof(char) * ret->str.capacity);
    CHECK_ALLOC(ret->str.ptr);

    va_end(ap);

    report_error(ret);
    return ret;
}

Object *object_error_new_from_string_slice(Object *o)
{
    assert(o->kind == O_STR);
    Object *ret = object_new_generic();
    ret->kind = O_ERROR;
    ret->str.capacity = ret->str.len = o->str.len;
    ret->str.ptr = malloc(sizeof(char) * ret->str.capacity);
    CHECK_ALLOC(ret->str.ptr);
    memcpy(ret->str.ptr, o->str.ptr, ret->str.len);

    report_error(ret);
    return ret;
}

Object *object_function_new(Env *e, Object *args, Object *body)
{
    Object *ret = object_new_generic();
    ret->kind = O_FUNCTION;
    ret->function.arguments = args;
    ret->function.body = body;
    ret->function.env = e;
    return ret;
}

Object *object_char_new(char c)
{
    Object *ret = object_new_generic();
    ret->kind = O_CHAR;
    ret->character = c;
    return ret;
}

void object_free(Object *o)
{
    DBG("freeing object at %p", o);
    if (o->kind == O_STR || o->kind == O_IDENT || o->kind == O_ERROR) 
        free(o->str.ptr);

    free(o);
}

inline static void _print_slice(struct StringSlice str)
{
    for (size_t i = 0; i < str.len; i++)
        putchar(str.ptr[i]);
}

void object_print(Object *o)
{
    assert(o);
    switch (o->kind) {
        case O_STR:
            putchar('"');
            _print_slice(o->str);
            putchar('"');
            break;
        case O_NUM:
            printf("%ld", o->num);
            break;
        case O_IDENT:
            _print_slice(o->str);
            break;
        case O_LIST:
            printf("("); 
            Object *cursor = o;
            for (;;) {
                object_print(cursor->list.car);
                cursor = cursor->list.cdr;
                if (cursor->kind == O_NIL) break;
                else if (cursor->kind == O_LIST) putchar(' ');
                /* this is for pairs similar to scheme (a . b) */
                else { printf(" . "); object_print(cursor); break; }
            }
            printf(")");
            break;
        case O_NIL:
            printf("nil");
            break;
        case O_ERROR:
            printf("ERROR: ");
            _print_slice(o->str);
            break;
        case O_BUILTIN:
            printf("builtin <%p>", o->builtin);
            break;
        case O_FUNCTION:
            object_print(o->function.arguments);
            printf(" -> ");
            object_print(o->function.body);
            break;
        case O_CHAR:
            printf("~%c", o->character);
            break;
    }
}

Env *env_new(Env *parent)
{
    Arena *arena = arena_new(sizeof(Env) + sizeof(EnvValueStore) * 10);
    Env *ret = arena_alloc(arena, sizeof(Env));
    *ret = (Env) {
        .parent = parent,
        .arena = arena,
        .store = NULL,
        .env_next = GC.env_list,
        .gc_mark = NOT_MARKED,
    };
    GC.env_list = ret;
    GC.live_environments++;

    return ret;
}

static void _GC_mark_env(Env *e);

static void _GC_mark_object(Object *o)
{
    if (o == NULL) {
        DBG("_GC_mark tried to mark a null object");
        return;
    }
    if (o->gc_mark == MARKED) { return; }
    o->gc_mark = MARKED;

    if (o->kind == O_LIST) {
        _GC_mark_object(o->list.car);
        _GC_mark_object(o->list.cdr);
    }

    if (o->kind == O_FUNCTION) {
        _GC_mark_object(o->function.arguments);
        _GC_mark_object(o->function.body);
        _GC_mark_env(o->function.env);
    }
}

static void _GC_mark_env(Env *e) 
{
    if (e->gc_mark == MARKED) return;
    e->gc_mark = MARKED;
    EnvValueStore *cursor = e->store;
    while (cursor != NULL) {
        _GC_mark_object(cursor->ident);
        _GC_mark_object(cursor->value);
        cursor = cursor->next;
    }

    if (e->parent) _GC_mark_env(e->parent);
}

static void _GC_sweep(void)
{
    /* thank you baby's first garbage collector for showing me the proper way to do this
     * https://journal.stuffwithstuff.com/2013/12/08/babys-first-garbage-collector/
     * I originally did this wrong without the double ptr and paid the price in debuging time */
    {
        Object **o = &GC.obj_list;
        while (*o) {
            DBG("sweeping object %p", o);
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
    {
        Env **e = &GC.env_list;
        while (*e) {
            DBG("sweeping environment %p", e);
            if ((*e)->gc_mark == NOT_MARKED) {
                Env *unreachable = *e;
                *e = unreachable->env_next;
                env_free(unreachable);
                GC.live_environments--;
            } else {
                (*e)->gc_mark = NOT_MARKED;
                e = &(*e)->env_next;
            }
        }
    }

}

void _GC_collect_garbage(Env *e, ...)
{
    if (e) _GC_mark_env(e);

    va_list ap;
    va_start(ap, e);
    Object *to_mark;
    while ((to_mark = va_arg(ap, Object*)) != NULL)
        _GC_mark_object(to_mark);

    va_end(ap);

    _GC_sweep();
}

void GC_debug_print_status(void)
{
    fprintf(stderr, "GC status:\n"
            "\tlive objects: %zu\n"
            "\tlive environments: %zu\n"
            "\tobj_list: %p\n"
            "\tenv_list: %p\n",
            GC.live_objects, GC.live_environments, GC.obj_list, GC.env_list);
}

