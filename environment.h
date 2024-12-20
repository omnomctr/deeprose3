#ifndef ENVIRONMENT_HEADER__
#define ENVIRONMENT_HEADER__

#include "object.h"

/* definition of env is in object.h because of recursive
 * definitions */

Env *env_new(Env *parent /* nullable */);
void env_free(Env *e);
void env_put(Env *e, Object *ident, Object *value);
Object *env_get(Env *e, Object *ident);

#endif
