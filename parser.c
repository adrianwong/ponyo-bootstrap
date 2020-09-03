#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "lexer.h"
#include "parser.h"

// Constants.
static Expr* TRUE = &(Expr){ .bl.type = EXPR_BOOL, .bl.value = true };
static Expr* FALSE = &(Expr){ .bl.type = EXPR_BOOL, .bl.value = false };
static Expr* NIL = &(Expr){ .type = EXPR_NIL };

static Expr* malloc_expr(void) {
    // No GC... yet.
    Expr* e = (Expr*)malloc(sizeof(Expr));
    if (e == NULL) { ERROR(EX_IOERR, "error: out of memory"); }

    return e;
}

static Expr* make_char(Token t) {
    assert(t.length >= 3); // Hash, backslash, and at least one character.
    assert(t.start[0] == '#' && t.start[1] == '\\');

    const char* sp = "#\\space";
    const char* tb = "#\\tab";
    const char* rt = "#\\return";
    const char* nl = "#\\newline";

    char c;
    if (t.length == 3) {
        c = t.start[2];
    } else {
        if (memcmp(t.start, sp, strlen(sp)) == 0) {
            c = ' ';
        } else if (memcmp(t.start, tb, strlen(tb)) == 0) {
            c = '\t';
        } else if (memcmp(t.start, rt, strlen(rt)) == 0) {
            c = '\r';
        } else if (memcmp(t.start, nl, strlen(nl)) == 0) {
            c = '\n';
        } else {
            DATA_ERROR(t.line, "invalid character name '%.*s'",
                t.length, t.start);
        }
    }

    Expr* e = malloc_expr();
    e->ch.type = EXPR_CHAR;
    e->ch.value = c;

    return e;
}

static Expr* make_string(Token t) {
    assert(t.length >= 2); // Opening and closing quotes.
    assert(t.start[0] == '"' && t.start[t.length - 1] == '"');

    Expr* e = malloc_expr();
    e->st.type = EXPR_STRING;

    int length = t.length - 1; // -2 to omit quotes, +1 for null terminator.
    e->st.value = (char*)malloc(length);
    if (e->st.value == NULL) { ERROR(EX_IOERR, "error: out of memory"); }
    memcpy(e->st.value, t.start + 1, length);
    e->st.value[length - 1] = '\0';

    return e;
}

static Expr* make_int(Token t) {
    assert(t.length >= 1);

    const char* c = t.start;
    int sign = 1;
    if (*c == '-') {
        assert(t.length >= 2);
        c++;
        sign = -1;
    }

    int value = 0;
    while (c < t.start + t.length) {
        value = value * 10 + (*c - '0');
        c++;
    }

    Expr* e = malloc_expr();
    e->in.type = EXPR_INT;
    e->in.value = sign * value;

    return e;
}

static Expr* make_quote(void) {
    Token t = token();
    if (t.type == TOK_EOF) { DATA_ERROR(t.line, "dangling quote"); }

    Expr* e = malloc_expr();
    e->qt.type = EXPR_QUOTE;
    e->qt.value = parse_expr(t);

    return e;
}

static Expr* make_list(void) {
    Token t = token();
    switch (t.type) {
    case TOK_RPAREN:
        return NIL;
    case TOK_EOF:
        DATA_ERROR(t.line, "dangling '('");
    default:
        DATA_ERROR(t.line, "unsupported");
    }

    return NULL;
}

Expr* parse_expr(Token t) {
    switch (t.type) {
    case TOK_TRUE:
        return TRUE;
    case TOK_FALSE:
        return FALSE;
    case TOK_CHAR:
        return make_char(t);
    case TOK_STRING:
        return make_string(t);
    case TOK_INT:
        return make_int(t);
    case TOK_QUOTE:
        return make_quote();
    case TOK_LPAREN:
        return make_list();
    case TOK_RPAREN:
        DATA_ERROR(t.line, "dangling ')'");
    case TOK_EOF:
        return NULL;
    }

    return NULL;
}
