#include <sys/param.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "eval.h"

#define EASSERT(expr, error) \
    do { if (!(expr)) return object_error_new((error)); } while (0);
static Object *_eval_sexpr(Object *o);

Object *eval(Object *o)
{
    if (!o->eval) return o;
    switch (o->kind) {
        case O_STR: case O_NUM: case O_NIL: case O_ERROR:
            return o;

        case O_IDENT:
            /* TODO */
            return object_error_new("identifier lookup feature not implemented :)");

        case O_LIST:
            return _eval_sexpr(o);
    }

    assert(0 && "infallible");
}

static Object *_builtin_add(Object *o)
{
    assert(o->kind == O_LIST);
    int32_t num = 0;
    while (o->kind == O_LIST) {
        Object *to_add = eval(o->list.car);
        EASSERT(to_add->kind == O_NUM, "+ needs numbers");
        num += to_add->num;
        o = o->list.cdr;
    }
    return object_num_new(num);
}

static Object *_builtin_subtract(Object *o)
{
    assert(o->kind == O_LIST);
    Object *lhs_object = eval(o->list.car);
    EASSERT(lhs_object->kind == O_NUM, "- needs numbers");
    int32_t lhs = lhs_object->num;
    o = o->list.cdr;
    if (o->kind != O_LIST) /* unary minus */
        lhs *= -1;

    while (o->kind == O_LIST) {
        Object *rhs = eval(o->list.car);
        EASSERT(rhs->kind == O_NUM, "- needs numbers");
        lhs -= rhs->num;
        o = o->list.cdr;
    }

    return object_num_new(lhs);
}

static Object *_builtin_multiply(Object *o) 
{
    assert(o->kind == O_LIST);
    int32_t num = 1;

    while (o->kind == O_LIST) {
        Object *to_mult = eval(o->list.car);
        EASSERT(to_mult->kind == O_NUM, "* needs numbers");
        num *= to_mult->num;
        o = o->list.cdr;
    }

    return object_num_new(num);
}

static Object *_builtin_divide(Object *o) 
{
    assert(o->kind == O_LIST);
    Object *lhs_object = eval(o->list.car);
    EASSERT(lhs_object->kind == O_NUM, "/ needs numbers");
    int32_t lhs = lhs_object->num;
    o = o->list.cdr;

    while (o->kind == O_LIST) {
        Object *rhs = eval(o->list.car);
        EASSERT(rhs->kind == O_NUM, "/ needs numbers");
        lhs /= rhs->num;
        o = o->list.cdr;
    }

    return object_num_new(lhs);
}

static Object *_builtin_exit(Object *o)
{
    assert(o->kind == O_LIST);
    Object *exit_code_object = eval(o->list.car);
    EASSERT(exit_code_object->kind == O_NUM, "exit needs numbers");
    exit(exit_code_object->num);
}

static Object *_builtin_cons(Object *o)
{
    assert(o->kind == O_LIST);
    Object *car = eval(o->list.car);
    Object *next = o->list.cdr; 
    EASSERT(next->kind == O_LIST, "cons needs two arguments");
    Object *cdr = eval(next->list.car);
    
    Object *ret = object_list_new(car, cdr);
    ret->eval = false;
    return ret;
}

static Object *_builtin_eval(Object *o) 
{
    assert(o->kind == O_LIST);
    Object *quoted_item = eval(o->list.car);
    EASSERT(!quoted_item->eval, "already evaluated");
    quoted_item->eval = true;
    return eval(quoted_item);
}

typedef Object *(*builtin)(Object*);
typedef struct { const char *name; builtin func; } builtin_record;
builtin_record builtins[] = {
    { "+", _builtin_add },
    { "-", _builtin_subtract },
    { "*", _builtin_multiply },
    { "/", _builtin_divide },
    { "exit", _builtin_exit },
    { "cons", _builtin_cons },
    { "eval", _builtin_eval },
};

static Object *_eval_sexpr(Object *o)
{
    /* TODO: so currently evaluating sexpr is completely backwards, it just checks the identifier
     * from a builtins list rather than getting the eval for identifiers part give it the function. 
     * thats because I havent set up the environment / scoping stuff yet */
    assert(o->kind == O_LIST);
    EASSERT(o->list.car->kind == O_IDENT, "first element in list must be an identifier");

    for (size_t i = 0; i < sizeof(builtins) / sizeof(builtin_record); i++) {
        if (memcmp(o->list.car->str.ptr, builtins[i].name, MIN(o->list.car->str.len, strlen(builtins[i].name))) == 0) {
            return builtins[i].func(o->list.cdr);
        }
    }

    return object_error_new("function not found");
}
