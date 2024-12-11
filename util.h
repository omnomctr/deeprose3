#ifndef UTIL_HEADER__
#define UTIL_HEADER__

#include <assert.h>

#define CHECK_ALLOC(ptr) \
    assert((ptr) && "ran out of memory");

#endif
