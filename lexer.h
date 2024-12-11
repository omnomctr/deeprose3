#ifndef LEXER_HEADER__
#define LEXER_HEADER__

#include <stdint.h>
#include <stddef.h>

#include "arena.h"

typedef struct Lexer {
    const char *str; 
    Arena *arena;
    size_t len;
    size_t pos;
    size_t line_number;
    char ch;
} Lexer;

enum TokenType {
    t_LPAREN,
    t_RPAREN,
    t_STR,
    t_IDENT,
    t_NUM,
    t_EOF,
    t_ILLEGAL,
};

typedef struct Token {
    size_t line; /* line number */
    enum TokenType type;
    union {
        int32_t num;
        struct {
            const char *ptr;
            size_t len;
        } string_slice;
   };
} Token;

Lexer *lexer_new(const char *str, Arena *a);
Token *lexer_next_token(Lexer *l);

void token_print(Token *t);

#endif
