#ifndef PARSER_HEADER__
#define PARSER_HEADER__

#include "lexer.h"
#include "object.h"

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
} Parser;

Parser *parser_new(Lexer *l);
Object *parser_parse(Parser *p);
const char *parser_error_string(Parser *p);
bool parser_at_eof(Parser *p);

#endif
