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
        return expr;
    }

    return NULL;
}

/******************************************************************************/

static void print_bool(Expr* e) {
    assert(e->type == EXPR_BOOL);

    if (e->b.value) { puts("#t"); } else { puts("#f"); }
}

static void print_char(Expr* e) {
    assert(e->type == EXPR_CHAR);

    printf("#\\");
    switch (e->c.value) {
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
        printf("%c", e->c.value);
        break;
    }
    printf("\n");
}

static void print_string(Expr* e) {
    assert(e->type == EXPR_STRING);

    char* c = e->s.value;
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
    printf("\"\n");
}

static void print_int(Expr* e) {
    assert(e->type == EXPR_INT);

    printf("%d\n", e->i.value);
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
    }
}
