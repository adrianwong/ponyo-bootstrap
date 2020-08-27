#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

typedef enum {
    EXPR_BOOL
} ExprType;

typedef struct {
    ExprType type;
    bool value;
} Bool;

typedef union {
    ExprType type;
    Bool b;
} Expr;

Expr* parse_expr(void);

#endif // PARSER_H
