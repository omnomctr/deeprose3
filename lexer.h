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
    t_QUOTE,
    t_STR,
    t_IDENT,
    t_NUM,
    t_EOF,
    t_ILLEGAL,
    t_CHAR,
};

typedef struct Token {
    size_t line; /* line number */
    enum TokenType type;
    union {
        struct {
            const char *ptr;
            size_t len;
        } string_slice;
        char character;
   };
} Token;

Lexer *lexer_new(const char *str, Arena *a);
Token *lexer_next_token(Lexer *l);

void token_print(Token *t);

typedef struct TokenLL TokenLL;
struct TokenLL {
    Token *token;
    TokenLL *next; /* nullable */
};

TokenLL *lexer_collect_tokens(Lexer *l);

#endif
