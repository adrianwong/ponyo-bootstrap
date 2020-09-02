#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

#include "lexer.h"

typedef union Expr Expr;

typedef enum {
    EXPR_BOOL,
    EXPR_CHAR,
    EXPR_STRING,
    EXPR_INT,
    EXPR_QUOTE,
    EXPR_EMPTY_LIST
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

typedef struct {
    ExprType type;
    Expr* value;
} Quote;

union Expr {
    ExprType type;
    Bool b;
    Char c;
    String s;
    Int i;
    Quote q;
};

Expr* parse_expr(Token t);

#endif // PARSER_H
