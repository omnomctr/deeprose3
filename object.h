#ifndef OBJECT_HEADER__
#define OBJECT_HEADER__

/* garbage collected lisp objects */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "lexer.h"
#include <gmp.h>
#include "arena.h"

typedef enum { MARKED, NOT_MARKED } Mark;

typedef struct Object Object;

struct StringSlice {
    char *ptr;
    size_t len, capacity;
};

struct List {
    Object *car;
    Object *cdr;
};

#define ERROR_NUM_MAX_STR_SIZE 256

enum ObjectKind {
    O_STR, O_NUM, O_LIST, O_IDENT, O_NIL, O_ERROR, O_BUILTIN, O_FUNCTION, O_CHAR,
};


typedef struct EnvValueStore EnvValueStore;
struct EnvValueStore {
    Object *ident;
    Object *value;
    EnvValueStore *next; /* nullable */
};
typedef struct Env Env;
struct Env {
    Env *env_next; /* nullable - for gc */
    Mark gc_mark;
    Env *parent; /* nullable */
    EnvValueStore *store; /* nullable */
};

struct Function {
    Object *arguments;
    Object *body;
    Env *env;
};

typedef Object *(*Builtin)(Env*, Object*);
struct Object {
    Object *obj_next; /* nullable - for garbage collector */
    Mark gc_mark;
    
    enum ObjectKind kind;
    bool eval;
    union {
        struct StringSlice str;
        mpz_t num;
        struct List list;
        Builtin builtin;
        struct Function function;
        char character;
   };
};

Object *object_new_generic(void);
Object *object_list_new(Object *car, Object *cdr);
Object *object_string_slice_new(const char *s, size_t len);
Object *object_ident_new(const char *s, size_t len);
Object *object_num_new(int64_t num);
Object *object_num_new_token(Token *t);
Object *object_nil_new(void);
Object *object_builtin_new(Builtin f);
const char *object_type_as_string(enum ObjectKind k);
Object *object_error_new(const char *fmt, ...);
Object *object_error_new_from_string_slice(Object *o);
Object *object_function_new(Env *e, Object *args, Object *body);
Object *object_char_new(char c);
Object *object_shallow_copy(Object *o);
void object_print(Object *o);
void object_free(Object *o);

#define GC_collect_garbage(env, ...) \
    _GC_collect_garbage(env __VA_OPT__(,) __VA_ARGS__, NULL);
void _GC_collect_garbage(Env *e, ...);
void GC_debug_print_status(void);

#endif
