#include <assert.h>
#include <stdlib.h>
#include "arena.h"
#include "parser.h"
#include "object.h"
#include "util.h"

static Object *_parser_parse_expr(Parser *p);
static Object *_parse_list(Parser *p);
static Object *_parser_parse_string_token(Token *t);

const char * const parser_error_as_string_arr[] = {
    [PE_NO_ERROR] = "no errors :)",
    [PE_UNEXPECTED_RPAREN] = "unexpected right parenthese )",
    [PE_UNEXPECTED_EOF] = "unexpected end of file",
    [PE_ILLEGAL_TOKEN] = "illegal token found",
};

Parser *parser_new(Lexer *l, Arena *a)
{
    Parser *ret = arena_alloc(a, sizeof(Parser));

    ret->input = lexer_collect_tokens(l);
    ret->cursor = ret->input;
    ret->error = PE_NO_ERROR;
    ret->line = ret->cursor->token->line;

    return ret;
}

inline static void _parser_next_token(Parser *p)
{
    if (p->cursor == NULL || p->cursor->token->type == t_EOF) p->error = PE_UNEXPECTED_EOF;
    else p->cursor = p->cursor->next;
    p->prevline = p->line;
    p->line = p->cursor->token->line;
}

static Object *_parser_parse_expr(Parser *p)
{
    Object *ret = NULL;
    Token *current_token = p->cursor->token;
    switch (current_token->type) {
        case t_LPAREN: {
            ret = _parse_list(p); 
        } break;
        case t_QUOTE: {
            _parser_next_token(p);
            if (p->error) break;
            ret = _parser_parse_expr(p);
            if (p->error) { ret = NULL; break; }
            ret->eval = false;
        } break; 
        case t_STR: {
            ret = _parser_parse_string_token(current_token);
        } break;
        case t_IDENT: {
            ret = object_ident_new(current_token->string_slice.ptr, current_token->string_slice.len);
        } break;
        case t_NUM: {
            ret = object_num_new_token(current_token);
        } break;
        case t_CHAR: {
            ret = object_char_new(current_token->character);
        } break;
        case t_RPAREN: {
            p->error = PE_UNEXPECTED_RPAREN;
            ret = NULL;
        } break;
        case t_EOF: {
            p->error = PE_UNEXPECTED_EOF;
            ret = NULL;
        } break;
        case t_ILLEGAL: {
            p->error = PE_ILLEGAL_TOKEN;
            ret = NULL;
        } break;
    }

    return ret;
}

static Object *_parse_list(Parser *p)
{
    _parser_next_token(p);
    if (p->error) return NULL;
    if (p->cursor->token->type == t_RPAREN) {
        return object_nil_new();
    }

    Object *ret = object_new_generic();
    ret->kind = O_LIST;
    
    Object *cursor = ret;
    for (;;) {
        cursor->list.car = _parser_parse_expr(p);
        if (p->error) return NULL;
        _parser_next_token(p);
        if (p->error) return NULL;
        if (p->cursor->token->type == t_RPAREN) {
            cursor->list.cdr = object_nil_new();
            break;
        } else if (p->cursor->token->type == t_EOF) {
            p->error = PE_UNEXPECTED_EOF;
        } else {
            cursor->list.cdr = object_new_generic();
            cursor->list.cdr->kind = O_LIST;
            cursor = cursor->list.cdr;
        }
    }

    return ret;
}

Object *parser_parse(Parser *p)
{
    Object *ret = _parser_parse_expr(p); 
    _parser_next_token(p);
    return ret;
}

const char *parser_error_string(Parser *p)
{
    return parser_error_as_string_arr[p->error];
}

bool parser_at_eof(Parser *p)
{
    return p->cursor->token->type == t_EOF;
}

static Object *_parser_parse_string_token(Token *t)
{
    Object *ret = object_new_generic();
    ret->kind = O_STR;
    ret->str.capacity = t->string_slice.len;
    ret->str.ptr = malloc(sizeof(char) * ret->str.capacity);
    CHECK_ALLOC(ret->str.ptr);
    
    const char *str = t->string_slice.ptr;
    size_t len = t->string_slice.len;

    size_t i = 0;

    bool escaped = false;
    for (size_t str_index = 0; str_index < len; str_index++) {
        if (str[str_index] == '\\' && !escaped) {
            escaped = true;
            continue;
        } 
        escaped ^= escaped;
        
        ret->str.ptr[i++] = str[str_index];
    }

    ret->str.len = i;

    return ret;
}


