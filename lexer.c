#include <stdbool.h>
#include <string.h>

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

static bool is_alpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

static bool is_comment(char c) { return c == ';'; }

static bool is_digit(char c) { return '0' <= c && c <= '9'; }

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

static Token character(void) {
    if (at_end()) { DATA_ERROR(lexer.line, "dangling '#\\'"); }

    char c = advance();
    // Handle character names.
    if (is_alpha(c)) {
        while (is_alpha(peek())) { advance(); }
    }

    return make_token(TOK_CHAR);
}

static Token boolean_or_character(void) {
    // Handle dangling `#`. R6RS supports `#;`, we don't.
    if (is_whitespace(peek()) || is_comment(peek()) || at_end()) {
        DATA_ERROR(lexer.line, "dangling '#'");
    }

    char c = advance();
    switch (c) {
    case 't':
        return make_token(TOK_TRUE);
    case 'f':
        return make_token(TOK_FALSE);
    case '\\':
        return character();
    default:
        DATA_ERROR(lexer.line, "invalid sequence '#%c'", c);
    }
}

static Token integer(void) {
    while (is_digit(peek())) { advance(); }

    return make_token(TOK_INT);
}

static Token string(void) {
    while (peek() != '"') {
        if (at_end()) { DATA_ERROR(lexer.line, "unterminated string"); }

        if (peek() == '\\' && peek_next() == '"') {
            // Consume escaped quote.
            if (peek_next() == '"') { advance(); }
        } else if (peek() == '\n') {
            lexer.line++;
        }

        advance();
    }
    advance(); // Consume closing quote.

    return make_token(TOK_STRING);
}

Token token(void) {
    skip_whitespace_and_comments();

    lexer.start = lexer.current;

    if (at_end()) { return make_token(TOK_EOF); }

    char c = advance();

    if (is_digit(c)) { return integer(); }

    switch (c) {
    case '#':
        return boolean_or_character();
    case '"':
        return string();
    case '-':
        if (is_digit(peek())) { return integer(); }
    case '\'':
        return make_token(TOK_QUOTE);
    case '(':
        return make_token(TOK_LPAREN);
    case ')':
        return make_token(TOK_RPAREN);
    default:
        DATA_ERROR(lexer.line, "unexpected character '%c'", c);
    }
}
