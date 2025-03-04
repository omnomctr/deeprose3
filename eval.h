#ifndef EVAL_HEADER__
#define EVAL_HEADER__

#include "environment.h"
#include "object.h"

#define EASSERT(expr, error, ...) \
    do { if (!(expr)) return object_error_new((error) __VA_OPT__(,) __VA_ARGS__); } while (0);

#define EASSERT_TYPE(f_name, obj, expected_type) \
    do { if ((obj)->kind != (expected_type)) { \
          if ((obj)->kind == O_ERROR) return o; \
          else return object_error_new(f_name ": expected %sc, got %sc", \
                  object_type_as_string((expected_type)), \
                  object_type_as_string((obj)->kind)); \
                      } } while(0);


Object *eval(Env* e, Object *o);
_Noreturn void report_error(Object *o);
void env_add_default_variables(Env *e);
int eval_program(const char *program, Env *env /*nullable*/, bool print_eval);
Object *eval_expr(Env *e, Object *o);

#endif
