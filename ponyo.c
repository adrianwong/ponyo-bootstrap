#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*------------------------------------------------------------------------------
 | ERROR REPORTING
 -----------------------------------------------------------------------------*/

#define ERROR(...) {              \
    fprintf(stderr, "error: ");   \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n");        \
    exit(1);                      \
}

/*------------------------------------------------------------------------------
 | SCHEME VALUES
 -----------------------------------------------------------------------------*/

typedef enum Type {
    TY_FALSE = 0,
    TY_TRUE = 1,
    TY_EMPTY_LIST,
    TY_INT,
    TY_PAIR,
    TY_STRING,
    TY_SYMBOL,
} Type;

typedef struct Val {
    Type ty;

    union {
        // Int.
        int num;
        // Pair.
        struct {
            struct Val* car;
            struct Val* cdr;
        };
        // String or symbol.
        char* str;
    };
} Val;

// Constants.
static Val* FALSE = &(Val){ TY_FALSE };
static Val* TRUE = &(Val){ TY_TRUE };
static Val* EMPTY_LIST = &(Val){ TY_EMPTY_LIST };

// A "table" for interned symbols.
static Val* symbol_list;

// From SICP: "...we represent an environment as a list of frames. The enclosing
// environment of an environment is the `cdr` of the list. The empty environment
// is simply the empty list.
//
// Each frame of an environment is represented as a pair of lists: a list of the
// variables bound in that frame and a list of the associated values."
static Val* global_env;

/*------------------------------------------------------------------------------
 | CONSTRUCTORS
 -----------------------------------------------------------------------------*/

static Val* malloc_val(Type ty) {
    // No GC... yet.
    Val* val = (Val*)malloc(sizeof(Val));
    if (val == NULL) {
        ERROR("out of memory");
    }
    val->ty = ty;
    return val;
}

static Val* cons(Val* car, Val* cdr) {
    Val* val = malloc_val(TY_PAIR);
    val->car = car;
    val->cdr = cdr;
    return val;
}

static Val* make_int(int num) {
    Val* val = malloc_val(TY_INT);
    val->num = num;
    return val;
}

static Val* make_string_or_symbol(Type ty, char* str) {
    Val* val = malloc_val(ty);
    val->str = (char*)malloc(strlen(str) + 1);
    if (val->str == NULL) {
        ERROR("out of memory");
    }
    strcpy(val->str, str);
    return val;
}

// Returns a symbol if it already exists, creates it otherwise.
static Val* intern_symbol(char* sym) {
    for (Val* s = symbol_list; s != EMPTY_LIST; s = s->cdr) {
        if (strcmp(sym, s->car->str) == 0) {
            return s->car;
        }
    }
    Val* val = make_string_or_symbol(TY_SYMBOL, sym);
    symbol_list = cons(val, symbol_list);
    return val;
}

/*------------------------------------------------------------------------------
 | PARSER
 -----------------------------------------------------------------------------*/

#define STRING_MAX_LEN 2000
#define SYMBOL_MAX_LEN 200

const char extended_symbols[] = "!$%&*+-./:<=>?@^_~";

static Val* read_c(int c);
static Val* read(void);

static int peek(void) {
    int c = getchar();
    ungetc(c, stdin);
    return c;
}

static int get_non_whitespace_char(void) {
    for (;;) {
        int c = getchar();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            continue;
        } else if (c == ';') {
            do {
                c = getchar();
            } while (c != '\n' && c != EOF);
        } else {
            return c;
        }
    }
}

// Assumes a '#' has already been read.
static Val* read_bool(void) {
    int c = getchar();
    switch (c) {
    case EOF:
        ERROR("invalid '#' prefix");
    case 'f':
        return FALSE;
    case 't':
        return TRUE;
    default:
        ERROR("invalid '#%c' prefix", c);
    }
}

static int read_int(int num) {
    while (isdigit(peek())) {
        num = num * 10 + (getchar() - '0');
    }
    return num;
}

// Assumes a '(' has already been read.
static Val* read_list(int c) {
    if (c == EOF) {
        ERROR("unterminated list");
    } else if (c == ')') {
        return EMPTY_LIST;
    }
    // NULL check unnecessary due to earlier EOF check.
    Val* car = read_c(c);

    c = get_non_whitespace_char();
    if (c == '.') {
        // NULL check unnecessary due to subsequent ')' check.
        Val* cdr = read();
        if (get_non_whitespace_char() != ')') {
            ERROR("expected list terminator");
        }
        return cons(car, cdr);
    } else {
        Val* cdr = read_list(c);
        return cons(car, cdr);
    }
}

static Val* read_quote(void) {
    Val* val = read();
    if (val == NULL) {
        ERROR("unexpected EOF reading quote");
    }
    Val* sym = make_string_or_symbol(TY_SYMBOL, "quote");
    return cons(sym, cons(val, EMPTY_LIST));
}

static Val* read_string(void) {
    char buffer[STRING_MAX_LEN + 1];
    int i = 0;
    int c = getchar();
    while (c != '"') {
        if (c == EOF) {
            ERROR("unterminated string");
        } else if (c == '\\') {
            c = getchar();
            switch (c) {
            case 't':
                c = '\t';
                break;
            case 'r':
                c = '\r';
                break;
            case 'n':
                c = '\n';
                break;
            default:
                break;
            }
        }
        if (i < STRING_MAX_LEN) {
            buffer[i++] = c;
        } else {
            ERROR("string too long");
        }
        c = getchar();
    }
    buffer[i] = '\0';
    return make_string_or_symbol(TY_STRING, buffer);
}

static Val* read_symbol(int c) {
    char buffer[SYMBOL_MAX_LEN + 1];
    buffer[0] = c;
    int i = 1;
    while (isalnum(peek()) || strchr(extended_symbols, peek())) {
        if (i < SYMBOL_MAX_LEN) {
            buffer[i++] = getchar();
        } else {
            ERROR("identifier too long")
        }
    }
    buffer[i] = '\0';
    return intern_symbol(buffer);
}

static Val* read_c(int c) {
    if (c == EOF) {
        return NULL;
    }
    if (c == '#') {
        return read_bool();
    }
    if (c == '"') {
        return read_string();
    }
    if (c == '(') {
        return read_list(get_non_whitespace_char());
    }
    if (c == '\'') {
        return read_quote();
    }
    if (isdigit(c)) {
        return make_int(read_int(c - '0'));
    }
    if (c == '-' && isdigit(peek())) {
        return make_int(-read_int(getchar() - '0'));
    }
    if (isalpha(c) || strchr(extended_symbols, c)) {
        return read_symbol(c);
    }
    ERROR("unexpected character '%c'", c);
}

static Val* read(void) {
    int c = get_non_whitespace_char();
    return read_c(c);
}

/*------------------------------------------------------------------------------
 | EVALUATOR
 -----------------------------------------------------------------------------*/

static Val* eval(Val* val) {
    switch (val->ty) {
    case TY_FALSE:
    case TY_TRUE:
    case TY_EMPTY_LIST:
    case TY_INT:
    case TY_STRING:
    case TY_PAIR:
    case TY_SYMBOL:
        return val;
    }
    return NULL;
}

/*------------------------------------------------------------------------------
 | PRINTER
 -----------------------------------------------------------------------------*/

static void print(Val* val);

static void print_string(Val* val) {
    printf("\"");
    char* str = val->str;
    while (*str != '\0') {
        switch (*str) {
        case '\t':
            printf("\\t");
            break;
        case '\r':
            printf("\\r");
            break;
        case '\n':
            printf("\\n");
            break;
        case '\\':
            printf("\\\\");
            break;
        case '"':
            printf("\\\"");
            break;
        default:
            printf("%c", *str);
            break;
        }
        str++;
    }
    printf("\"");
}

static void print_list(Val* val) {
    printf("(");
    print(val->car);
    Val* cdr = val->cdr;
    while (cdr->ty == TY_PAIR) {
        printf(" ");
        print(cdr->car);
        cdr = cdr->cdr;
    }
    if (cdr != EMPTY_LIST) {
        printf(" . ");
        print(cdr);
    }
    printf(")");
}

static void print(Val* val) {
    switch (val->ty) {
    case TY_FALSE:
        printf("#f");
        break;
    case TY_TRUE:
        printf("#t");
        break;
    case TY_EMPTY_LIST:
        printf("()");
        break;
    case TY_INT:
        printf("%d", val->num);
        break;
    case TY_STRING:
        print_string(val);
        break;
    case TY_PAIR:
        print_list(val);
        break;
    case TY_SYMBOL:
        printf("%s", val->str);
        break;
    }
}

/*------------------------------------------------------------------------------
 | PONYO!
 -----------------------------------------------------------------------------*/

int main(void) {
    symbol_list = EMPTY_LIST;
    global_env = cons(cons(EMPTY_LIST, EMPTY_LIST), EMPTY_LIST);
    for (;;) {
        Val* val = read();
        if (val != NULL) {
            print(eval(val));
            printf("\n");
        } else {
            return 0;
        }
    }
}
