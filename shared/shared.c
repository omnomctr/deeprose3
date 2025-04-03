// example of C FFI w/ deeprose
// to use it, make a shared object file (see the makefile)
// and then open it with (import-shared <path to .so file> <function name>)


#include "../object.h"
#include "../eval.h"

#include <stdlib.h>

Object *doom(Env *e, Object *o) 
{
    EASSERT(o->kind == O_NIL, "doom: expected 0 arguments");

    system("gzdoom");

    return object_nil_new();
}
