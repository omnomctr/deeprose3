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

static Object *_eval_sexpr(Env *e, Object *o);

static Object *_builtin_add(Env *e, Object *o);
static Object *_builtin_subtract(Env *e, Object *o);
static Object *_builtin_multiply(Env *e, Object *o);
static Object *_builtin_divide(Env *e, Object *o);
static Object *_builtin_exit(Env *e, Object *o);
static Object *_builtin_cons(Env *e, Object *o);
static Object *_builtin_eval(Env *e, Object *o);
static Object *_builtin_first(Env *e, Object *o);
static Object *_builtin_rest(Env *e, Object *o);
static Object *_builtin_def(Env *e, Object *o);
static Object *_builtin_print_gc_status(Env *e, Object *o);
static Object *_builtin_error(Env *e, Object *o);
static Object *_builtin_lambda(Env *e, Object *o);

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
    { "def", _builtin_def }, 
    { "gc-status", _builtin_print_gc_status }, 
    { "error", _builtin_error },
    { "\\", _builtin_lambda },
};

void env_add_default_variables(Env *e) 
{
    for (size_t i = 0; i < sizeof(builtins) / sizeof(builtin_record); i++) {
        env_put(e, object_ident_new(builtins[i].name, strlen(builtins[i].name)),
                    object_builtin_new(builtins[i].func));
    }
    char nil_ident[] = "nil";
    env_put(e, object_ident_new(nil_ident, strlen(nil_ident)), object_nil_new());
}

Object *eval_expr(Env *e, Object *o)
{
    if (!o->eval) return o;
    switch (o->kind) {
        case O_STR: case O_NUM: case O_NIL: case O_ERROR: case O_BUILTIN: case O_FUNCTION:
            return o;

        case O_IDENT:
            return env_get(e, o);

        case O_LIST:
            return _eval_sexpr(e, o);
    }

    assert(0 && "infallible");
}

Object *eval(Env *e, Object *o)
{
    if (setjmp(on_error_jmp_buf) != 0) {
        return on_error_error;}

    return eval_expr(e, o);

}

_Noreturn void report_error(Object *o)
{
    assert(o->kind == O_ERROR);
    on_error_error = o;
    longjmp(on_error_jmp_buf, 1);
}

static Object *_builtin_add(Env *e, Object *o)
{
    EASSERT(o->kind == O_LIST, "+ requires arguments");
    int32_t num = 0;
    while (o->kind == O_LIST) {
        Object *to_add = eval_expr(e, o->list.car);
        EASSERT_TYPE("+", to_add, O_NUM);
        num += to_add->num;
        o = o->list.cdr;
    }
    return object_num_new(num);
}

static Object *_builtin_subtract(Env *e, Object *o)
{
    EASSERT(o->kind == O_LIST, "- requires arguments");
    Object *lhs_object = eval_expr(e, o->list.car);
    EASSERT_TYPE("-", lhs_object, O_NUM);
    int32_t lhs = lhs_object->num;
    o = o->list.cdr;
    if (o->kind != O_LIST) /* unary minus */
        lhs *= -1;

    while (o->kind == O_LIST) {
        Object *rhs = eval_expr(e, o->list.car);
        EASSERT_TYPE("-", rhs, O_NUM);
        lhs -= rhs->num;
        o = o->list.cdr;
    }

    return object_num_new(lhs);
}

static Object *_builtin_multiply(Env *e, Object *o) 
{
    EASSERT(o->kind == O_LIST, "* requires arguments");
    int32_t num = 1;

    while (o->kind == O_LIST) {
        Object *to_mult = eval_expr(e, o->list.car);
        EASSERT_TYPE("*", to_mult, O_NUM);
        num *= to_mult->num;
        o = o->list.cdr;
    }

    return object_num_new(num);
}

static Object *_builtin_divide(Env *e, Object *o) 
{
    EASSERT(o->kind == O_LIST, "/ requires arguments");
    Object *lhs_object = eval_expr(e, o->list.car);
    EASSERT_TYPE("/", lhs_object, O_NUM);
    int32_t lhs = lhs_object->num;
    o = o->list.cdr;

    while (o->kind == O_LIST) {
        Object *rhs = eval_expr(e, o->list.car);
        EASSERT_TYPE("/", rhs, O_NUM);
        lhs /= rhs->num;
        o = o->list.cdr;
    }

    return object_num_new(lhs);
}

static Object *_builtin_exit(Env *e, Object *o)
{
    EASSERT(o->kind == O_LIST, "exit requires arguments");
    Object *exit_code_object = eval_expr(e, o->list.car);
    EASSERT_TYPE("exit", exit_code_object, O_NUM);
    exit(exit_code_object->num);
}

static Object *_builtin_cons(Env *e, Object *o)
{
    EASSERT(o->kind == O_LIST, "cons needs two arguments");
    Object *car = eval_expr(e, o->list.car);
    Object *next = o->list.cdr; 
    EASSERT(next->kind == O_LIST, "cons needs two arguments");
    Object *cdr = eval_expr(e, next->list.car);
    
    Object *ret = object_list_new(car, cdr);
    ret->eval = false;
    return ret;
}

static Object *_builtin_eval(Env *e, Object *o) 
{
    EASSERT(o->kind == O_LIST, "eval requires an argument");
    Object *quoted_item = eval_expr(e, o->list.car);
    EASSERT(!quoted_item->eval, "eval: already evaluated");
    quoted_item->eval = true;
    return eval_expr(e, quoted_item);
}

static Object *_builtin_first(Env *e, Object *o)
{
    EASSERT(o->kind == O_LIST, "first requires an argument");
    Object *arg1 = eval_expr(e, o->list.car);
    EASSERT_TYPE("first", arg1, O_LIST);

    return arg1->list.car;
}

static Object *_builtin_rest(Env *e, Object *o)
{
    EASSERT(o->kind == O_LIST, "rest requires an argument");
    Object *arg1 = eval_expr(e, o->list.car);
    EASSERT_TYPE("rest", arg1, O_LIST);

    return arg1->list.cdr;
}

static Object *_builtin_def(Env *e, Object *o)
{
    EASSERT(o->kind == O_LIST, "def requires an argument");
    EASSERT_TYPE("def", o->list.car, O_IDENT);

    Object *value = eval_expr(e, o->list.cdr->list.car);
    env_put(e, o->list.car, value);
    return object_nil_new();
}

static Object *_eval_sexpr(Env *e, Object *o)
{
    assert(o->kind == O_LIST);
    EASSERT(o->list.cdr->kind == O_LIST || o->list.cdr->kind == O_NIL, "invalid function call (did you try to run a pair (f . x) ?)");


    o->list.car->eval = true;
    Object *f = o->list.car->kind == O_FUNCTION ? o->list.car : eval_expr(e, o->list.car);

    if (f->kind == O_BUILTIN) return f->builtin(e, o->list.cdr);
    else {
        if (f->kind != O_FUNCTION) {
            printf("got: %s, expected function\n", object_type_as_string(f->kind));
        }

        {
            Object *cursor = f->function.arguments;
            Object *args_cursor = o->list.cdr;
            while (cursor->kind != O_NIL) {
                if (args_cursor->kind == O_NIL) {
                    return object_error_new("function passed too few values");
                }
                EASSERT(args_cursor->kind == O_LIST, "invalid function call form");
                env_put(f->function.env, cursor->list.car, eval_expr(e, args_cursor->list.car));
                
                cursor = cursor->list.cdr;
                args_cursor = args_cursor->list.cdr;
            }

            if (args_cursor->kind != O_NIL) {
                return object_error_new("function passed too many values");
            }
        }

        return eval_expr(f->function.env, f->function.body);
    }
}

static Object *_builtin_print_gc_status(Env *e, Object *o)
{
    GC_debug_print_status();
    return object_nil_new();
}

static Object *_builtin_error(Env *e, Object *o)
{
    EASSERT(o->kind == O_LIST, "error requires an argument");
    Object *error = eval_expr(e, o->list.car);
    EASSERT_TYPE("error", error, O_STR);
    return object_error_new_from_string_slice(error);
}

static Object *_builtin_lambda(Env *e, Object *o)
{
    EASSERT(o->kind == O_LIST, "\\ needs arguments");
    Object *arguments = o->list.car;
    EASSERT_TYPE("\\", arguments, O_LIST);

    {
        /* make sure all the arguments are identifiers */
        Object *cursor = arguments;
        while (cursor->kind != O_NIL) {
            EASSERT_TYPE("\\", cursor, O_LIST);
            EASSERT_TYPE("\\", cursor->list.car, O_IDENT);
            EASSERT(cursor->list.car->eval, "\\: all arguments must be evaluated (did you add a quote somewhere?)");
            cursor = cursor->list.cdr;
        }
    }

    EASSERT(o->list.cdr->kind == O_LIST, "\\ needs two arguments");
    Object *body = o->list.cdr->list.car;
    EASSERT_TYPE("\\", body, O_LIST);

    Env *f_env = env_new(e);

    return object_function_new(f_env, arguments, body);
}
