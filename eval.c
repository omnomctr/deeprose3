#include <sys/param.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <stdio.h>
#include "eval.h"
#include "lexer.h"
#include "parser.h"
#include "util.h"

jmp_buf on_error_jmp_buf;
Object *on_error_error = NULL;

#define EASSERT(expr, error) \
    do { if (!(expr)) return object_error_new((error)); } while (0);

#define EASSERT_TYPE(f_name, obj, expected_type) \
    do { if ((obj)->kind != (expected_type)) { \
          if ((obj)->kind == O_ERROR) return o; \
          else return object_error_new(f_name ": expected %sc, got %sc", \
                  object_type_as_string((expected_type)), \
                  object_type_as_string((obj)->kind)); \
                      } } while(0);

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
static Object *_builtin_load(Env *e, Object *o);
static Object *_builtin_let(Env *e, Object *o);
static Object *_builtin_do(Env *e, Object *o);
static Object *_builtin_cond(Env *e, Object *o);

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
    { "load", _builtin_load },
    { "let", _builtin_let },
    { "do", _builtin_do },
    { "cond", _builtin_cond },
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

int eval_program(const char *program, Env *env /*nullable*/, bool print_eval)
{
    bool free_env = false;
    if (env == NULL) {
        free_env = true;
        env = env_new(NULL);
        env_add_default_variables(env);
    }

    Arena *parser_arena = arena_new(0);
    Lexer *lex = lexer_new(program, parser_arena);
    Parser *parser = parser_new(lex, parser_arena);

    Object *o = parser_parse(parser);

    for (;;) {
        if (parser->error) printf("parser has error \"%s\"\n", parser_error_string(parser)); 
        else if (o == NULL) break;
        else {
            if (print_eval) {
                object_print(eval(env, o));
                putchar('\n');
            } else eval(env, o);
        }

        if (parser_at_eof(parser)) break;
        o = parser_parse(parser);
        GC_collect_garbage(env, o);
    }
    
    if (free_env) {
        GC_collect_garbage(NULL);
    } else {
        GC_collect_garbage(env);
    }

    arena_destroy(parser_arena);
    return 0;
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
    int64_t num = 0;
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
    int64_t lhs = lhs_object->num;
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
    int64_t num = 1;

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
    int64_t lhs = lhs_object->num;
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

static Object *_eval_list_elements(Env *e, Object *o)
{
    if (o->kind != O_LIST) return object_new_generic();
    Object *ret = object_new_generic();
    ret->kind = O_LIST;

    Object *cursor = ret;
    while (o->kind != O_NIL) {
        cursor->list.car = eval_expr(e, o->list.car);
        o = o->list.cdr;
        if (o->kind != O_LIST) {
            cursor->list.cdr = object_nil_new();
        } else {
            cursor->list.cdr = object_new_generic();
            cursor = cursor->list.cdr;
            cursor->kind = O_LIST;
        }
    }

    return ret;
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
            return object_error_new("invalid function call, expected function got %sc", object_type_as_string(f->kind));
        }
        Env *env = env_new(f->function.env);
        
        bool variadic = false;
        {
            Object *cursor = f->function.arguments;
            Object *args_cursor = o->list.cdr;
            while (cursor->kind != O_NIL) {
                char ampersand[] = "&";
                if (cursor->list.car->str.len == strlen(ampersand) 
                        && memcmp(cursor->list.car->str.ptr, ampersand, cursor->list.car->str.len) == 0) {

                    EASSERT(cursor->list.cdr->kind != O_NIL 
                            && cursor->list.cdr->list.car->kind == O_IDENT, 
                            "function needs identifier past ampersand for variadics");

                    /* give it an empty list if there are no variadic args */
                    if (args_cursor->kind == O_NIL) env_put(env, cursor->list.cdr->list.car, object_nil_new());
                    else env_put(env, cursor->list.cdr->list.car, _eval_list_elements(e, args_cursor));

                    variadic = true;
                    break;
                }
                if (args_cursor->kind == O_NIL) {
                    return object_error_new("function passed too few values");
                }
                EASSERT(args_cursor->kind == O_LIST, "invalid function call form");
                env_put(env, cursor->list.car, eval_expr(e, args_cursor->list.car));
                
                cursor = cursor->list.cdr;
                args_cursor = args_cursor->list.cdr;
            }
            if (!variadic && args_cursor->kind != O_NIL) {
                return object_error_new("function %s passed too many values", o->list.car);
            }
        }

        Object *res = eval_expr(env, f->function.body);
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
    if (arguments->kind != O_LIST && arguments->kind != O_NIL) {
        return object_error_new("\\: expected list, got %sc", object_type_as_string(arguments->kind));
    }

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

    return object_function_new(e, arguments, body);
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
                return a->str.len == b->str.len && memcmp(a->str.ptr, b->str.ptr, a->str.len) == 0 ? object_num_new(1) : object_nil_new();
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
                printf("%ld", o->num);
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

static Object *_builtin_load(Env *e, Object *o)
{
    EASSERT(o->kind == O_LIST, "load: needs an argument");
    Object *file_path = eval_expr(e, o->list.car);
    EASSERT_TYPE("load", file_path, O_STR);

    char *file_path_cstr = malloc(sizeof(char) * (file_path->str.len + 1));
    CHECK_ALLOC(file_path_cstr);

    memcpy(file_path_cstr, file_path->str.ptr, file_path->str.len);
    file_path_cstr[file_path->str.len] = '\0';

    FILE *f = fopen(file_path_cstr, "r");
    if (f == NULL) return object_error_new("load: couldn't open file");
    char *program = file_to_str(f);
    fclose(f);
    int status = eval_program(program, e, false);
    free(program); free(file_path_cstr);
    if (status != 0) return object_error_new("load: error when evaluating program");

    return object_nil_new();
}

static Object *_builtin_let(Env *e, Object *o)
{
    EASSERT(o->kind == O_LIST, "let: needs at least two arguments");
    EASSERT(o->list.cdr->kind == O_LIST, "let: needs at least two arguments");

    Env *new_env = env_new(e);
    Object *vars = o->list.car;
    while (vars->kind == O_LIST) {
        Object *ident = vars->list.car;
        if (ident->kind != O_IDENT) {
            return object_error_new("let: exepected identifier in argslist, got %sc", object_type_as_string(ident->kind));
        }
        EASSERT(vars->list.cdr->kind == O_LIST, "let: needs an even number of variable declarations");
        Object *value = eval_expr(new_env, vars->list.cdr->list.car);
        env_put(new_env, ident, value);
        
        vars = vars->list.cdr->list.cdr;
    }

    Object *ret = eval_expr(new_env, o->list.cdr->list.car);
    return ret;
}

static Object *_builtin_do(Env *e, Object *o)
{
    while (o->kind == O_LIST) {
        if (o->list.cdr->kind == O_NIL) return eval_expr(e, o->list.car);
        (void)eval_expr(e, o->list.car);
        o = o->list.cdr;
    }

    return o;
}

static Object *_builtin_cond(Env *e, Object *o)
{
    while (o->kind == O_LIST) {
        EASSERT(o->list.cdr->kind == O_LIST, "cond: needs an even number of arguments");
        Object *boolean = eval_expr(e, o->list.car);

        if (boolean->kind != O_NIL) {
            return eval_expr(e, o->list.cdr->list.car);
        } else {
            o = o->list.cdr->list.cdr;
        }
    }

    return object_nil_new();
}
