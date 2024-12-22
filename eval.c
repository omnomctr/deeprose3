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
static Object *_builtin_if(Env *e, Object *o);
static Object *_builtin_equals(Env *e, Object *o);
static Object *_builtin_print(Env *e, Object *o);
static Object *_builtin_println(Env *e, Object *o);
static Object *_builtin_list(Env *e, Object *o);
static Object *_builtin_mod(Env *e, Object *o);
static Object *_builtin_not(Env *e, Object *o);
static Object *_builtin_and(Env *e, Object *o);
static Object *_builtin_or(Env *e, Object *o);
static Object *_builtin_lt(Env *e, Object *o);
static Object *_builtin_gt(Env *e, Object *o);

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
    { "if", _builtin_if },
    { "=", _builtin_equals },
    { "print", _builtin_print },
    { "println", _builtin_println },
    { "list", _builtin_list },
    { "mod", _builtin_mod },
    { "not", _builtin_not },
    { "and", _builtin_and },
    { "or", _builtin_or },
    { "<", _builtin_lt },
    { ">", _builtin_gt },
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
        assert(f->kind == O_FUNCTION);
        Env *env = env_new(f->function.env);

        {
            Object *cursor = f->function.arguments;
            Object *args_cursor = o->list.cdr;
            while (cursor->kind != O_NIL) {
                if (args_cursor->kind == O_NIL) {
                    return object_error_new("function passed too few values");
                }
                EASSERT(args_cursor->kind == O_LIST, "invalid function call form");
                env_put(env, cursor->list.car, eval_expr(e, args_cursor->list.car));
                
                cursor = cursor->list.cdr;
                args_cursor = args_cursor->list.cdr;
            }
#if 0
            /* TODO function expressions dont work with this but 
             * then we cant catch too many arguments being passed
             * to a function */
            if (args_cursor->kind != O_NIL) {
                return object_error_new("function passed too many values");
            }
#endif
        }

        Object *res = eval_expr(env, f->function.body);
        env_free(env);
#if 0
        /* TODO: figure out why enabling the gc leads to a segfault when running this program:
         * (def fib (\ (prev current n)
         *      (if (= n 1)
         *          (cons current nil)
         *          (cons current (fib current (+ prev current) (dec n))))))
         */

(println (fib 0 1 20))


        GC_collect_garbage(env, res);
#endif
        return res;
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
    if (arguments->kind != O_LIST && arguments->kind != O_NIL) 
        return object_error_new("\\: expected list, got %s", object_type_as_string(arguments->kind));

    if (arguments->kind == O_LIST) {
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

static Object *_builtin_if(Env *e, Object *o)
{
    EASSERT(o->kind = O_LIST, "if requires 3 arguments");
    Object *expr = eval_expr(e, o->list.car);
    EASSERT(o->list.cdr->kind == O_LIST, "if requires 3 arguments");
    Object *if_true = o->list.cdr->list.car;
    EASSERT(o->list.cdr->list.cdr->kind == O_LIST, "if requires 3 arguments");
    Object *if_false = o->list.cdr->list.cdr->list.car;

    if (expr->kind == O_NIL) /* falsey value */ {
        return eval_expr(e, if_false);
    } else {
        return eval_expr(e, if_true);
    }
}

static Object *_builtin_equals(Env *e, Object *o)
{
    EASSERT(o->kind == O_LIST, "=: needs two arguments");
    Object *a = eval_expr(e, o->list.car);
    EASSERT(o->list.cdr->kind, "=: needs two arguments");
    Object *b = eval_expr(e, o->list.cdr->list.car);

    if (a->kind != b->kind) return object_nil_new();
    else {
        switch (a->kind) {
            case O_NUM:
                return a->num == b->num ? object_num_new(1) : object_nil_new();
                break;

            case O_STR: case O_IDENT: case O_ERROR:
                return memcmp(a->str.ptr, b->str.ptr, a->str.len) == 0 ? object_num_new(1) : object_nil_new();
                break;

            case O_LIST:
                return object_error_new("=: list comparisons arent implemented yet :)");
                break;

            case O_NIL: /* only one state so its always equal to any other nil */
                return object_num_new(1); 
                break;

            case O_BUILTIN: case O_FUNCTION:
                return object_error_new("=: function comparisons arent implemented :)");
                break;
         }
    }
    assert(0 && "infallible");
}

/* this is basically copy-pasted from object.c but the quotes from the strings are removed */
static inline void _print_slice(struct StringSlice s)
{
    for (size_t i = 0; i < s.len; i++)
        putchar(s.ptr[i]);
}

static void print(Object *o)
{
    switch (o->kind) {
            case O_STR:
                _print_slice(o->str);
                break;
            case O_NUM:
                printf("%d", o->num);
                break;
            case O_IDENT:
                _print_slice(o->str);
                break;
            case O_LIST:
                printf("("); 
                Object *cursor = o;
                for (;;) {
                    print(cursor->list.car);
                    cursor = cursor->list.cdr;
                    if (cursor->kind == O_NIL) break;
                    else if (cursor->kind == O_LIST) putchar(' ');
                    else { printf(" . "); print(cursor); break; }
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
                print(o->function.arguments);
                printf(" -> ");
                print(o->function.body);
                break;
        }
}

static Object *_builtin_print(Env *e, Object *o)
{
    EASSERT(o->kind == O_LIST, "print: needs an argument");
    print(eval(e, o->list.car));
    return object_nil_new();
}

static Object *_builtin_println(Env *e, Object *o)
{
    if (o->kind != O_LIST) { putchar('\n'); return object_nil_new(); }
    print(eval(e, o->list.car));
    putchar('\n');
    return object_nil_new();
}

static Object *_builtin_list(Env *e, Object *o)
{
    if (o->kind == O_NIL) return object_nil_new();
    else {
        EASSERT(o->kind == O_LIST, "list: needs arguments");

        Object *cursor = o;
        Object *ret = object_new_generic();
        Object *ret_cursor = ret;

        while (cursor->kind == O_LIST) {
            ret_cursor->kind = O_LIST;
            ret_cursor->list.car = eval_expr(e, cursor->list.car);

            cursor = cursor->list.cdr;
            if (cursor->kind != O_LIST) {
                ret_cursor->list.cdr = object_nil_new();
                break;
            } else {
                ret_cursor->list.cdr = object_new_generic();
                ret_cursor = ret_cursor->list.cdr;
                ret_cursor->kind = O_LIST;
            }
        }

        return ret;
    }
}

static Object *_builtin_mod(Env *e, Object *o)
{
    EASSERT(o->kind == O_LIST, "mod: needs two arguments");
    EASSERT(o->list.cdr->kind == O_LIST, "mod: needs two arguments");

    Object *lhs = eval_expr(e, o->list.car);
    Object *rhs = eval_expr(e, o->list.cdr->list.car);

    return object_num_new(lhs->num % rhs->num);
}

static Object *_builtin_not(Env *e, Object *o)
{
    EASSERT(o->kind == O_LIST, "not: needs an argument");

    Object *boolean = eval_expr(e, o->list.car);
    return boolean->kind == O_NIL ? object_num_new(1) : object_nil_new();
}

static Object *_builtin_and(Env *e, Object *o)
{
    while (o->kind == O_LIST) {
        Object *boolean = eval_expr(e, o->list.car);
        if (boolean->kind == O_NIL) return boolean;
        o = o->list.cdr;
    }

    return object_num_new(1);
}

static Object *_builtin_or(Env *e, Object *o)
{
    while (o->kind == O_LIST) {
        Object *boolean = eval_expr(e, o->list.car);
        if (boolean->kind != O_NIL) return boolean;
        o = o->list.cdr;
    }

    return object_nil_new();
}

static Object *_builtin_lt(Env *e, Object *o)
{
    EASSERT(o->kind == O_LIST, "<: needs two arguments");
    EASSERT(o->list.cdr->kind == O_LIST, "<: needs two arguments");

    Object *lhs = eval_expr(e, o->list.car);
    EASSERT_TYPE("<", lhs, O_NUM);
    Object *rhs = eval_expr(e, o->list.cdr->list.car);
    EASSERT_TYPE("<", rhs, O_NUM);

    return  lhs->num < rhs->num ? object_num_new(1) : object_nil_new();
}

static Object *_builtin_gt(Env *e, Object *o)
{
    EASSERT(o->kind == O_LIST, ">: needs two arguments");
    EASSERT(o->list.cdr->kind == O_LIST, ">: needs two arguments");

    Object *lhs = eval_expr(e, o->list.car);
    EASSERT_TYPE(">", lhs, O_NUM);
    Object *rhs = eval_expr(e, o->list.cdr->list.car);
    EASSERT_TYPE(">", rhs, O_NUM);

    return  lhs->num > rhs->num ? object_num_new(1) : object_nil_new();
}
