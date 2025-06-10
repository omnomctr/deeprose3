#ifndef PARSER_HEADER__
#define PARSER_HEADER__

#include "lexer.h"
#include "object.h"
#include "arena.h"

enum ParserError {
    PE_NO_ERROR = 0,
    PE_UNEXPECTED_RPAREN,
    PE_UNEXPECTED_EOF,
    PE_ILLEGAL_TOKEN,
};

typedef struct Parser {
    TokenLL *input;
    TokenLL *cursor;

    enum ParserError error;
    size_t line;
    size_t prevline; // terrible fix but we need to preserve
                     // the previous token's line so the parser_parse
                     // caller can get it. bad
} Parser;

Parser *parser_new(Lexer *l, Arena *a);
Object *parser_parse(Parser *p);
const char *parser_error_string(Parser *p);
bool parser_at_eof(Parser *p);


#endif
