#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "lexer.h"
#include "parser.h"

static Expr* make_bool(bool b) {
    // No GC... yet.
    Expr* e = (Expr*)malloc(sizeof(Expr));
    e->b.type = EXPR_BOOL;
    e->b.value = b;

    return e;
}

static Expr* make_char(Token t) {
    assert(t.length >= 3); // Hash, backslash, and at least one character.
    assert(t.start[0] == '#' && t.start[1] == '\\');

    char c;
    if (t.length == 3) {
        c = t.start[2];
    } else {
        if (memcmp(t.start, "#\\space", t.length) == 0) {
            c = ' ';
        } else if (memcmp(t.start, "#\\tab", t.length) == 0) {
            c = '\t';
        } else if (memcmp(t.start, "#\\return", t.length) == 0) {
            c = '\r';
        } else if (memcmp(t.start, "#\\newline", t.length) == 0) {
            c = '\n';
        } else {
            DATA_ERROR(t.line, "invalid character name '%.*s'",
                t.length, t.start);
        }
    }

    // No GC... yet.
    Expr* e = (Expr*)malloc(sizeof(Expr));
    e->c.type = EXPR_CHAR;
    e->c.value = c;

    return e;
}

Expr* parse_expr(void) {
    Token t = token();
    switch (t.type) {
    case TOK_TRUE:
        return make_bool(true);
    case TOK_FALSE:
        return make_bool(false);
    case TOK_CHAR:
        return make_char(t);
    case TOK_EOF:
        return NULL;
    }
}
