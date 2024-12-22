#ifndef EVAL_HEADER__
#define EVAL_HEADER__

#include "environment.h"
#include "object.h"

Object *eval(Env* e, Object *o);
_Noreturn void report_error(Object *o);
void env_add_default_variables(Env *e);
int eval_program(const char *program, Env *env /*nullable*/, bool print_eval);

#endif
