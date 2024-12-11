#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include "lexer.h"
#include "util.h"

static void _lexer_skip_whitespace(Lexer *l);
static void _lexer_read_char(Lexer *l);

static Token *_token_new(Lexer *l, enum TokenType t);
static Token *_read_string(Lexer *l);
static Token *_read_number(Lexer *l);
static Token *_read_identifier(Lexer *l);
static Token* _lexer_read(Lexer *l, enum TokenType type, bool(*pred)(char));

static bool _is_identifier_special_char(char c);

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
    _lexer_skip_whitespace(l);

    Token *tok;
    switch (l->ch) {
        case '\0': tok = _token_new(l, t_EOF); break;
        case '(': tok = _token_new(l, t_LPAREN); break;
        case ')': tok = _token_new(l, t_RPAREN); break;
        case '"': tok = _read_string(l); break;
        default:
            if (isalpha(l->ch) || _is_identifier_special_char(l->ch)) {
                tok = _read_identifier(l);
                break;
            } else if (isdigit(l->ch)) {
                tok = _read_number(l);
                break;
            }
            
            tok = _token_new(l, t_ILLEGAL);
    }

    _lexer_read_char(l);

    return tok;
}

static inline void _print_token_string_slice(Token *t)
{
    for (size_t i = 0; i < t->string_slice.len; i++)
        putchar(t->string_slice.ptr[i]);
}

void token_print(Token *t)
{
    switch (t->type) {
        case t_LPAREN: printf("{left parenthese}"); break;
        case t_RPAREN: printf("{right parenthese}"); break;
        case t_EOF: printf("{end of file}"); break;
        case t_STR: 
            printf("{string \""); 
            _print_token_string_slice(t);
            printf("\"}");
            break;
        case t_IDENT:
            printf("{identifier ");
            _print_token_string_slice(t);
            printf("}");
            break;
        case t_NUM: printf("{number %d}", t->num); break;
        case t_ILLEGAL: printf("{illegal token: line %zu}", t->line); break;
    }
}

static void _lexer_skip_whitespace(Lexer *l)
{
    while (l->ch == ' ' || l->ch == '\t' || l->ch == '\n' || l->ch == '\r' || l->ch == ';') {
        if (l->ch == ';') {
            while (l->ch != '\n' && l->ch != '\0') {
                l->line_number++;
                _lexer_read_char(l);
            }
        }
        _lexer_read_char(l);
    }
}

static void _lexer_read_char(Lexer *l)
{
    l->ch = l->pos >= l->len ? '\0' : l->str[++l->pos];
}

static Token *_token_new(Lexer *l, enum TokenType t)
{
    Token *ret = arena_alloc(l->arena, sizeof(Token));
    ret->line = l->line_number;
    ret->type = t;

    return ret;
}

static bool _isnt_unquote(char c) { return c != '"'; }
static Token *_read_string(Lexer *l)
{
   _lexer_read_char(l);
   return _lexer_read(l, t_STR, _isnt_unquote);
}

static bool _is_identifier_letter(char c)
{
    return isalnum(c) || _is_identifier_special_char(c);
}



static bool _is_identifier_special_char(char c)
{
    char *allowed_characters = "?!<>=%^*+-/";
    for (; *allowed_characters != '\0'; allowed_characters++)
        if (*allowed_characters == c) return true;

    return false;
}

static Token *_read_identifier(Lexer *l)
{
    return _lexer_read(l, t_IDENT, _is_identifier_letter);
}

static bool _is_digit(char c) { return isdigit(c); }
static Token *_read_number(Lexer *l)
{
    Token *t = _lexer_read(l, t_NUM, _is_digit);

    int32_t num = 0;
    for (size_t i = 0; i < t->string_slice.len; i++) {
        num *= 10;
        num += (int32_t)t->string_slice.ptr[i] - (int32_t)'0';
    }

    /* overwrite the stringslice struct */
    t->num = num;

    return t;
}

static Token* _lexer_read(Lexer *l, enum TokenType type, bool(*pred)(char)) 
{
   const char *ptr = &l->str[l->pos];
   size_t original_pos = l->pos;

   while ((*pred)(l->ch)) {
       if (l->ch == '\0') {
           fprintf(stderr, "unexpected end of file\n");
           exit(-1);
       }
       _lexer_read_char(l);
   }

   Token *ret = _token_new(l, type);
   ret->string_slice.ptr = ptr;
   ret->string_slice.len = l->pos - original_pos;

   return ret;
}

