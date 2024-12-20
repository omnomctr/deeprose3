#ifndef OBJECT_HEADER__
#define OBJECT_HEADER__

/* garbage collected lisp objects */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "arena.h"

#define INITIAL_OBJECT_GC_MAX 32

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

#define OBJECT_ERROR_STR_MAX_SIZE 1024

enum ObjectKind {
    O_STR, O_NUM, O_LIST, O_IDENT, O_NIL, O_ERROR, O_BUILTIN,
};


typedef struct EnvValueStore EnvValueStore;
struct EnvValueStore {
    Object *ident;
    Object *value;
    EnvValueStore *next; /* nullable */
};
typedef struct Env Env;
struct Env {
    Env *parent; /* nullable */
    Arena *arena;
    EnvValueStore *store; /* nullable */
};


typedef Object *(*Builtin)(Env*, Object*);
struct Object {
    Object *obj_next; /* nullable - for garbage collector */
    Mark gc_mark;
    
    enum ObjectKind kind;
    bool eval;
    union {
        struct StringSlice str;
        int32_t num;
        struct List list;
        Builtin builtin;
   };
};

Object *object_new_generic(void);
Object *object_list_new(Object *car, Object *cdr);
Object *object_string_slice_new(const char *s, size_t len);
Object *object_ident_new(const char *s, size_t len);
Object *object_num_new(int32_t num);
Object *object_nil_new(void);
Object *object_builtin_new(Builtin f);
const char *object_type_as_string(enum ObjectKind k);
Object *object_error_new(const char *fmt, ...);
Object *object_error_new_from_string_slice(Object *o);
void object_print(Object *o);
void object_free(Object *o);

void GC_collect_garbage(Env *e);
void GC_debug_print_status(void);

#endif
