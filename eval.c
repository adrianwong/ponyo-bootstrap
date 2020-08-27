#include <assert.h>
#include <stdio.h>

#include "eval.h"

Expr* eval(Expr* expr) {
    assert(expr != NULL);

    switch (expr->type) {
    case EXPR_BOOL:
        return expr;
    }
}

/******************************************************************************/

static void print_bool(Expr* e) {
    assert(e->type == EXPR_BOOL);

    if (e->b.value) { puts("#t"); } else { puts("#f"); }
}

void print_expr(Expr* expr) {
    assert(expr != NULL);

    switch (expr->type) {
    case EXPR_BOOL:
        print_bool(expr);
        break;
    }
}
