#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

typedef enum {
    EXPR_BOOL,
    EXPR_CHAR
} ExprType;

typedef struct {
    ExprType type;
    bool value;
} Bool;

typedef struct {
    ExprType type;
    char value;
} Char;

typedef union {
    ExprType type;
    Bool b;
    Char c;
} Expr;

Expr* parse_expr(void);

#endif // PARSER_H
