#ifndef EVAL_HEADER__
#define EVAL_HEADER__

#include "object.h"

Object *eval(Object *o);
_Noreturn void report_error(Object *o);
#endif
