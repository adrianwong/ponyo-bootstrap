#include <assert.h>
#include <stdio.h>

#include "eval.h"

Expr* eval(Expr* expr) {
    assert(expr != NULL);

    switch (expr->type) {
    case EXPR_BOOL:
    case EXPR_CHAR:
    case EXPR_STRING:
    case EXPR_INT:
    case EXPR_NIL:
    case EXPR_CELL:
        return expr;
    case EXPR_QUOTE:
        return expr->qt.value;
    }

    return NULL;
}

/******************************************************************************/

static void print_bool(Expr* e) {
    assert(e->type == EXPR_BOOL);

    if (e->bl.value) { printf("#t"); } else { printf("#f"); }
}

static void print_char(Expr* e) {
    assert(e->type == EXPR_CHAR);

    printf("#\\");
    switch (e->ch.value) {
    case ' ':
        printf("space");
        break;
    case '\t':
        printf("tab");
        break;
    case '\r':
        printf("return");
        break;
    case '\n':
        printf("newline");
        break;
    default:
        printf("%c", e->ch.value);
        break;
    }
}

static void print_string(Expr* e) {
    assert(e->type == EXPR_STRING);

    char* c = e->st.value;
    printf("\"");
    while (*c != '\0') {
        switch (*c) {
        case '\t':
            printf("\\t");
            break;
        case '\r':
            printf("\\r");
            break;
        case '\n':
            printf("\\n");
            break;
        default:
            printf("%c", *c);
            break;
        }
        c++;
    }
    printf("\"");
}

static void print_int(Expr* e) {
    assert(e->type == EXPR_INT);

    printf("%d", e->in.value);
}

static void print_list(Expr* e) {
    assert(e->type == EXPR_CELL);

    printf("(");
    print_expr(e->ce.car);

    Expr* cdr = e->ce.cdr;
    while (cdr->type != EXPR_NIL) {
        assert(cdr->type == EXPR_CELL);

        printf(" ");
        print_expr(cdr->ce.car);
        cdr = cdr->ce.cdr;
    }
    printf(")");
}

void print_expr(Expr* expr) {
    assert(expr != NULL);

    switch (expr->type) {
    case EXPR_BOOL:
        print_bool(expr);
        break;
    case EXPR_CHAR:
        print_char(expr);
        break;
    case EXPR_STRING:
        print_string(expr);
        break;
    case EXPR_INT:
        print_int(expr);
        break;
    case EXPR_QUOTE:
        printf("'");
        print_expr(expr->qt.value);
        break;
    case EXPR_NIL:
        printf("()");
        break;
    case EXPR_CELL:
        print_list(expr);
        break;
    }
}
