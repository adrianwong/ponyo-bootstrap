#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

typedef enum {
    EXPR_BOOL,
    EXPR_CHAR,
    EXPR_STRING,
    EXPR_INT
} ExprType;

typedef struct {
    ExprType type;
    bool value;
} Bool;

typedef struct {
    ExprType type;
    char value;
} Char;

typedef struct {
    ExprType type;
    char* value;
} String;

typedef struct {
    ExprType type;
    int value;
} Int;

typedef union {
    ExprType type;
    Bool b;
    Char c;
    String s;
    Int i;
} Expr;

Expr* parse_expr(void);

#endif // PARSER_H
