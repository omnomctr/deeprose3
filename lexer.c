#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include "lexer.h"

static void _lexer_skip_whitespace(Lexer *l);
static void _lexer_read_char(Lexer *l);

static Token *_token_new(Lexer *l, enum TokenType t);
static Token *_read_string(Lexer *l);
static Token *_read_number(Lexer *l);
static Token *_read_identifier(Lexer *l);
static Token *_lexer_read(Lexer *l, enum TokenType type, bool(*pred)(char));

static bool _is_identifier_special_char(char c);

Lexer *lexer_new(const char *str, Arena *a)
{
    Lexer *ret = arena_alloc(a, sizeof(Lexer));

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
        case '\'': tok = _token_new(l, t_QUOTE); break;
        case '(': tok = _token_new(l, t_LPAREN); break;
        case ')': tok = _token_new(l, t_RPAREN); break;
        case '"': tok = _read_string(l); break;
        case '~': {
            _lexer_read_char(l);
            tok = _token_new(l, t_CHAR);
            tok->character = l->ch;
        } break;
        default:
            if (isdigit(l->ch)) {
                tok = _read_number(l);
                return tok;
                break;
            } else if (isalpha(l->ch) || _is_identifier_special_char(l->ch)) {
                tok = _read_identifier(l);
                return tok;
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
        case t_QUOTE: printf("{quote}"); break;
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
        case t_NUM: 
            printf("{number "); 
            _print_token_string_slice(t);
            printf("}");
            break;
        case t_CHAR: printf("{char %c}", t->character); break;
        case t_ILLEGAL: printf("{illegal token: line %zu}", t->line); break;
    }
}

static void _lexer_skip_whitespace(Lexer *l)
{
    while (l->ch == ' ' || l->ch == '\t' || l->ch == '\n' || l->ch == '\r' || l->ch == ';') {
        if (l->ch == '\n') l->line_number++;
        if (l->ch == ';') {
            while (l->ch != '\n' && l->ch != '\0') {
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

static Token *_read_string(Lexer *l)
{
    _lexer_read_char(l);

    const char *ptr = &l->str[l->pos];
    size_t original_pos = l->pos;

    /* its the parsers job to clean up any escape characters since 
     * we dont own the string */

    bool escaped = false;
    while ((l->ch != '"' || escaped) && l->ch != '\0') {
        if (l->ch == '\\' && !escaped) escaped = true;
        else if (escaped) escaped = false;

        _lexer_read_char(l);
    }

    Token *ret = _token_new(l, t_STR);
    ret->string_slice.ptr = ptr;
    ret->string_slice.len = l->pos - original_pos;
    
    return ret;
}

static bool _is_identifier_letter(char c)
{
    return isalnum(c) || _is_identifier_special_char(c);
}

static bool _is_identifier_special_char(char c)
{
    char *allowed_characters = "?!<>=%^*+-/-\\_&";
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
    /* number tokens now return strings so they can be converted to mpz_ts later on */
    return _lexer_read(l, t_NUM, _is_digit);
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

TokenLL *lexer_collect_tokens(Lexer *l)
{
    TokenLL *node = arena_alloc(l->arena, sizeof(TokenLL));
    TokenLL *ret = node;
    Token *t;
    for (;;) {
        t = lexer_next_token(l);
        node->token = t;
        
        if (t->type == t_EOF) {
            node->next = NULL;
            break;
        } else {
            node->next = arena_alloc(l->arena, sizeof(TokenLL));
            node = node->next;
        }
    }

    return ret;
}

