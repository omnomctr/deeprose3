#include <sys/param.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <stdio.h>
#include "eval.h"

jmp_buf on_error_jmp_buf;
Object *on_error_error = NULL;

#define EASSERT(expr, error) \
    do { if (!(expr)) return object_error_new((error)); } while (0);

#define EASSERT_TYPE(f_name, obj, expected_type) \
    do { if ((obj)->kind != (expected_type)) { \
          if ((obj)->kind == O_ERROR) return o; \
          else return object_error_new(f_name ": expected %s, got %s", object_type_as_string(expected_type), object_type_as_string((obj)->kind)); } } while(0);

static Object *_eval_sexpr(Object *o);

static Object *_builtin_add(Object *o);
static Object *_builtin_subtract(Object *o);
static Object *_builtin_multiply(Object *o);
static Object *_builtin_divide(Object *o);
static Object *_builtin_exit(Object *o);
static Object *_builtin_cons(Object *o);
static Object *_builtin_eval(Object *o);
static Object *_builtin_first(Object *o);
static Object *_builtin_rest(Object *o);

typedef struct { const char *name; Builtin func; } builtin_record;
builtin_record builtins[] = {
    { "+", _builtin_add },
    { "-", _builtin_subtract },
    { "*", _builtin_multiply },
    { "/", _builtin_divide },
    { "exit", _builtin_exit },
    { "cons", _builtin_cons },
    { "eval", _builtin_eval },
    { "first", _builtin_first },
    { "rest", _builtin_rest },
};


Object *eval_expr(Object *o)
{
    if (!o->eval) return o;
    switch (o->kind) {
        case O_STR: case O_NUM: case O_NIL: case O_ERROR: case O_BUILTIN:
            return o;

        case O_IDENT:
            for (size_t i = 0; i < sizeof(builtins) / sizeof(builtin_record); i++) {
                if (o->str.len == strlen(builtins[i].name) && memcmp(o->str.ptr, builtins[i].name, o->str.len) == 0) {
                    return object_builtin_new(builtins[i].func);
                }
            }

            return object_error_new("identifier not found");

        case O_LIST:
            return _eval_sexpr(o);
    }

    assert(0 && "infallible");
}

Object *eval(Object *o)
{
    if (setjmp(on_error_jmp_buf) != 0)
        return on_error_error;

    return eval_expr(o);

}

_Noreturn void report_error(Object *o)
{
    assert(o->kind == O_ERROR);
    on_error_error = o;
    longjmp(on_error_jmp_buf, 1);
}

static Object *_builtin_add(Object *o)
{
    EASSERT(o->kind == O_LIST, "+ requires arguments");
    int32_t num = 0;
    while (o->kind == O_LIST) {
        Object *to_add = eval_expr(o->list.car);
        EASSERT_TYPE("+", to_add, O_NUM);
        num += to_add->num;
        o = o->list.cdr;
    }
    return object_num_new(num);
}

static Object *_builtin_subtract(Object *o)
{
    EASSERT(o->kind == O_LIST, "- requires arguments");
    Object *lhs_object = eval_expr(o->list.car);
    EASSERT_TYPE("-", lhs_object, O_NUM);
    int32_t lhs = lhs_object->num;
    o = o->list.cdr;
    if (o->kind != O_LIST) /* unary minus */
        lhs *= -1;

    while (o->kind == O_LIST) {
        Object *rhs = eval_expr(o->list.car);
        EASSERT_TYPE("-", rhs, O_NUM);
        lhs -= rhs->num;
        o = o->list.cdr;
    }

    return object_num_new(lhs);
}

static Object *_builtin_multiply(Object *o) 
{
    EASSERT(o->kind == O_LIST, "* requires arguments");
    int32_t num = 1;

    while (o->kind == O_LIST) {
        Object *to_mult = eval_expr(o->list.car);
        EASSERT_TYPE("*", to_mult, O_NUM);
        num *= to_mult->num;
        o = o->list.cdr;
    }

    return object_num_new(num);
}

static Object *_builtin_divide(Object *o) 
{
    EASSERT(o->kind == O_LIST, "/ requires arguments");
    Object *lhs_object = eval_expr(o->list.car);
    EASSERT_TYPE("/", lhs_object, O_NUM);
    int32_t lhs = lhs_object->num;
    o = o->list.cdr;

    while (o->kind == O_LIST) {
        Object *rhs = eval_expr(o->list.car);
        EASSERT_TYPE("/", rhs, O_NUM);
        lhs /= rhs->num;
        o = o->list.cdr;
    }

    return object_num_new(lhs);
}

static Object *_builtin_exit(Object *o)
{
    EASSERT(o->kind == O_LIST, "exit requires arguments");
    Object *exit_code_object = eval_expr(o->list.car);
    EASSERT_TYPE("exit", exit_code_object, O_NUM);
    exit(exit_code_object->num);
}

static Object *_builtin_cons(Object *o)
{
    EASSERT(o->kind == O_LIST, "cons needs two arguments");
    Object *car = eval_expr(o->list.car);
    Object *next = o->list.cdr; 
    EASSERT(next->kind == O_LIST, "cons needs two arguments");
    Object *cdr = eval_expr(next->list.car);
    
    Object *ret = object_list_new(car, cdr);
    ret->eval = false;
    return ret;
}

static Object *_builtin_eval(Object *o) 
{
    EASSERT(o->kind == O_LIST, "eval requires an argument");
    Object *quoted_item = eval_expr(o->list.car);
    EASSERT(!quoted_item->eval, "eval: already evaluated");
    quoted_item->eval = true;
    return eval_expr(quoted_item);
}

static Object *_builtin_first(Object *o)
{
    EASSERT(o->kind == O_LIST, "first requires an argument");
    Object *arg1 = eval_expr(o->list.car);
    EASSERT_TYPE("first", arg1, O_LIST);

    return arg1->list.car;
}

static Object *_builtin_rest(Object *o)
{
    EASSERT(o->kind == O_LIST, "rest requires an argument");
    Object *arg1 = eval_expr(o->list.car);
    EASSERT_TYPE("rest", arg1, O_LIST);

    return arg1->list.cdr;
}

static Object *_eval_sexpr(Object *o)
{
    assert(o->kind == O_LIST);
    EASSERT(o->list.car->kind == O_IDENT, "first element in list must be an identifier");
    EASSERT(o->list.cdr->kind == O_LIST || o->list.cdr->kind == O_NIL, "invalid function call (did you try to run a pair (f . x) ?");
 
    o->list.car->eval = true;
    Object *f = eval(o->list.car);
    assert(f->kind == O_BUILTIN);
    return f->builtin(o->list.cdr);  
}
