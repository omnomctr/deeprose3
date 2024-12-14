#ifndef UTIL_HEADER__
#define UTIL_HEADER__

#include <assert.h>
#include <stdio.h>

#define CHECK_ALLOC(ptr) \
    assert((ptr) && "ran out of memory");

#ifdef DEBUG
#define DBG(fmt, __VA_ARGS__) \
    fprintf(stderr, fmt __VA_OPT_(,)_ __VA_ARGS__);
#else
#define DBG(__VA_ARGS)
#endif

#endif
