#include <stdbool.h>

#include "error.h"
#include "lexer.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} Lexer;

Lexer lexer;

void init_lexer(const char* source) {
    lexer.start = source;
    lexer.current = source;
    lexer.line = 1;
}

/******************************************************************************/

static bool is_comment(char c) { return c == ';'; }

static bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/******************************************************************************/

static bool at_end(void) {
    return *lexer.current == '\0';
}

static char advance(void) {
    return at_end() ? '\0' : *lexer.current++;
}

static char peek(void) {
    return *lexer.current;
}

static char peek_next(void) {
    return at_end() ? '\0' : lexer.current[1];
}

static void skip_whitespace_and_comments(void) {
    for (;;) {
        char c = peek();
        if (is_whitespace(c)) {
            if (c == '\n') { lexer.line++; }
            advance();
        } else if (is_comment(c)) {
            do { advance(); } while (peek() != '\n' && !at_end());
        } else {
            break;
        }
    }
}

/******************************************************************************/

static Token make_token(TokenType type) {
    Token t;
    t.type = type;
    t.start = lexer.start;
    t.length = (int)(lexer.current - lexer.start);
    t.line = lexer.line;

    return t;
}

static Token boolean(void) {
    char c = peek();

    // Handle dangling `#`. R6RS supports `#;`, we don't.
    if (is_whitespace(c) || is_comment(c) || at_end()) {
        DATA_ERROR(lexer.line, "dangling '#'");
    }

    advance();
    switch (c) {
    case 't':
        return make_token(TOK_TRUE);
    case 'f':
        return make_token(TOK_FALSE);
    default:
        DATA_ERROR(lexer.line, "invalid sequence '#%c'", c);
    }
}

Token token(void) {
    skip_whitespace_and_comments();

    lexer.start = lexer.current;

    if (at_end()) { return make_token(TOK_EOF); }

    char c = advance();
    switch (c) {
    case '#':
        return boolean();
    default:
        DATA_ERROR(lexer.line, "unexpected character '%c'", c);
    }
}
