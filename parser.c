#include <stdlib.h>

#include "lexer.h"
#include "parser.h"

static Expr* make_bool(bool b) {
    // No GC... yet.
    Expr* e = (Expr*)malloc(sizeof(Expr));
    e->b.type = EXPR_BOOL;
    e->b.value = b;

    return e;
}

Expr* parse_expr(void) {
    Token t = token();
    switch (t.type) {
    case TOK_TRUE:
        return make_bool(true);
    case TOK_FALSE:
        return make_bool(false);
    case TOK_EOF:
        return NULL;
    }
}
