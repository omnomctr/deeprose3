#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "lexer.h"
#include "util.h"

static void lexer_skip_whitespace(Lexer *l);
static void lexer_read_char(Lexer *l);

static Token *token_new(Lexer *l, enum TokenType t);


Lexer *lexer_new(const char *str, Arena *a)
{
    Lexer *ret = malloc(sizeof(Lexer));
    CHECK_ALLOC(ret);

    *ret = (Lexer) {
        .pos = 0,
        .str = str,
        .len = strlen(str),
        .arena = a,
        .line_number = 1,
    };
    assert(ret->len >= 1);
    ret->ch = ret->str[0];

    return ret;
}

Token *lexer_next_token(Lexer *l)
{
    lexer_skip_whitespace(l);

    Token *tok;
    switch (l->ch) {
        case '\0': tok = NULL; break;
        case '(': 
            tok = token_new(l, t_LPAREN);
            break;
        case ')':
            tok = token_new(l, t_RPAREN);
            break;

        default:
            assert(0 && "TODO");
    }

    lexer_read_char(l);

    return tok;
}

void token_print(Token *t)
{
    switch (t->type) {
        case t_LPAREN: printf("{left parenthese}"); break;
        case t_RPAREN: printf("{right parenthese}"); break;
        default: assert(0 && "todo");
    };
}

static void lexer_skip_whitespace(Lexer *l)
{
    while (l->ch == ' ' || l->ch == '\t' || l->ch == '\n' || l->ch == '\r' || l->ch == ';') {
        if (l->ch == ';') {
            while (l->ch != '\n' && l->ch != '\0') {
                l->line_number++;
                lexer_read_char(l);
            }
        }
        lexer_read_char(l);
    }
}

void lexer_read_char(Lexer *l)
{
    l->ch = l->pos >= l->len ? '\0' : l->str[++l->pos];
}

Token *token_new(Lexer *l, enum TokenType t)
{
    Token *ret = arena_alloc(l->arena, sizeof(Token));
    ret->line = l->line_number;
    ret->type = t;

    return ret;
}
