#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOK_TRUE,
    TOK_FALSE,
    TOK_CHAR,
    TOK_STRING,
    TOK_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

void init_lexer(const char* source);
Token token(void);

#endif // LEXER_H
