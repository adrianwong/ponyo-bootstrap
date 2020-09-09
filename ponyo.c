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
    TY_PRIM_PROC,
    TY_STRING,
    TY_SYMBOL,
} Type;

typedef struct Val Val;
typedef Val* PrimProc(Val* args, Val* env);
struct Val {
    Type ty;

    union {
        // Int.
        int num;
        // Pair.
        struct {
            Val* car;
            Val* cdr;
        };
        // Primitive procedure.
        PrimProc* proc;
        // String or symbol.
        char* str;
    };
};

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

static Val* make_int(int num) {
    Val* val = malloc_val(TY_INT);
    val->num = num;
    return val;
}

static Val* cons(Val* car, Val* cdr) {
    Val* val = malloc_val(TY_PAIR);
    val->car = car;
    val->cdr = cdr;
    return val;
}

static Val* make_prim_proc(PrimProc* proc) {
    Val* val = malloc_val(TY_PRIM_PROC);
    val->proc = proc;
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
    Val* sym = intern_symbol("quote");
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
 | ENVIRONMENT
 -----------------------------------------------------------------------------*/

static Val* lookup_variable(Val* var, Val* env) {
    while (env != EMPTY_LIST) {
        Val* frame = env->car;
        Val* vars = frame->car;
        Val* vals = frame->cdr;
        while (vars != EMPTY_LIST) {
            // Note: `strcmp` is unnecessary. Symbols are interned, so
            // pointer comparison is sufficient.
            if (var == vars->car) {
                return vals->car;
            }
            vars = vars->cdr;
            vals = vals->cdr;
        }
        // Not found in current frame, search enclosing environment.
        env = env->cdr;
    }
    ERROR("unbound variable: %s", var->str);
}

static void add_binding(Val* var, Val* val, Val* frame) {
    frame->car = cons(var, frame->car);
    frame->cdr = cons(val, frame->cdr);
}

static void define_variable(Val* var, Val* val, Val* env) {
    Val* first_frame = env->car;
    Val* vars = first_frame->car;
    Val* vals = first_frame->cdr;
    while (vars != EMPTY_LIST) {
        // Change the binding if it exists.
        if (var == vars->car) {
            vals->car = val;
            return;
        }
        vars = vars->cdr;
        vals = vals->cdr;
    }
    // If binding doesn't exist, adjoin one to the first frame.
    add_binding(var, val, first_frame);
}

/*------------------------------------------------------------------------------
 | EVALUATOR
 -----------------------------------------------------------------------------*/

static Val* apply(Val* val, Val* args, Val* env) {
    if (val->ty == TY_PRIM_PROC) {
        return val->proc(args, env);
    } else {
        ERROR("unknown procedure type");
    }
}

static Val* eval(Val* val, Val* env) {
    switch (val->ty) {
    case TY_FALSE:
    case TY_TRUE:
    case TY_INT:
    case TY_STRING:
    case TY_PRIM_PROC:
        return val;
    case TY_EMPTY_LIST:
        ERROR("invalid syntax: ()");
    case TY_PAIR: {
        Val* proc = eval(val->car, env);
        Val* args = val->cdr;
        return apply(proc, args, env);
    }
    case TY_SYMBOL:
        return lookup_variable(val, env);
    default:
        return NULL;
    }
}

/*------------------------------------------------------------------------------
 | PRIMITIVE PROCEDURES
 -----------------------------------------------------------------------------*/

#define PRIM_ADD    "+"
#define PRIM_SUB    "-"
#define PRIM_MUL    "*"
#define PRIM_DIV    "/"
#define PRIM_ABS    "abs"
#define PRIM_LT     "<"
#define PRIM_GT     ">"
#define PRIM_NUM_EQ "="
#define PRIM_EQ     "eq?"
#define PRIM_CAR    "car"
#define PRIM_CDR    "cdr"
#define PRIM_CONS   "cons"
#define PRIM_IF     "if"
#define PRIM_AND    "and"
#define PRIM_OR     "or"
#define PRIM_NOT    "not"
#define PRIM_DEFINE "define"
#define PRIM_QUOTE  "quote"

static int len(Val* val) {
    int len = 0;
    while (val->ty == TY_PAIR) {
        len++;
        val = val->cdr;
    }
    // -1 if list is improper.
    return val == EMPTY_LIST ? len : -1;
}

static char gt(int a, int b) { return a  > b ? 1 : 0; }
static char eq(int a, int b) { return a == b ? 1 : 0; }
static void check_len(char* proc, Val* args, char (*op)(int, int), int exp) {
    if (!op(len(args), exp)) {
        ERROR("%s: incorrect argument count", proc);
    }
}

// Yes, "typ" without the "e" so the function name has the same number of
// characters as `check_len`. Fight me.
static void check_typ(char* proc, Val* arg, Type exp) {
    if (arg->ty != exp) {
        char* type;
        switch (exp) {
        case TY_FALSE:
        case TY_TRUE:
            type = "boolean";
            break;
        case TY_INT:
            type = "number";
            break;
        case TY_PAIR:
            type = "pair";
            break;
        case TY_STRING:
            type = "string";
            break;
        case TY_SYMBOL:
            type = "symbol";
            break;
        default:
            ERROR("argument is not of the expected type");
        }
        ERROR("%s: argument is not a %s", proc, type);
    }
}

static Val* prim_add(Val* args, Val* env) {
    int sum = 0;
    for (Val* a = args; a != EMPTY_LIST; a = a->cdr) {
        Val* val = eval(a->car, env);
        check_typ(PRIM_ADD, val, TY_INT);
        sum += val->num;
    }
    return make_int(sum);
}

static Val* prim_sub(Val* args, Val* env) {
    check_len(PRIM_SUB, args, gt, 0);
    Val* val = eval(args->car, env);
    check_typ(PRIM_SUB, val, TY_INT);
    if (args->cdr == EMPTY_LIST) {
        return make_int(-val->num);
    }
    int sum = val->num;
    for (Val* a = args->cdr; a != EMPTY_LIST; a = a->cdr) {
        Val* val = eval(a->car, env);
        check_typ(PRIM_SUB, val, TY_INT);
        sum -= val->num;
    }
    return make_int(sum);
}

static Val* prim_mul(Val* args, Val* env) {
    int sum = 1;
    for (Val* a = args; a != EMPTY_LIST; a = a->cdr) {
        Val* val = eval(a->car, env);
        check_typ(PRIM_MUL, val, TY_INT);
        sum *= val->num;
    }
    return make_int(sum);
}

// No support for rational values (yet?).
static Val* prim_div(Val* args, Val* env) {
    check_len(PRIM_DIV, args, gt, 0);
    Val* val = eval(args->car, env);
    check_typ(PRIM_DIV, val, TY_INT);
    if (args->cdr == EMPTY_LIST) {
        return make_int(val->num);
    }
    int sum = val->num;
    for (Val* a = args->cdr; a != EMPTY_LIST; a = a->cdr) {
        Val* val = eval(a->car, env);
        check_typ(PRIM_DIV, val, TY_INT);
        sum /= val->num;
    }
    return make_int(sum);
}

static Val* prim_abs(Val* args, Val* env) {
    check_len(PRIM_ABS, args, eq, 1);
    Val* val = eval(args->car, env);
    check_typ(PRIM_ABS, val, TY_INT);
    return val->num < 0 ? make_int(-val->num) : make_int(val->num);
}

static Val* prim_lt(Val* args, Val* env) {
    check_len(PRIM_LT, args, gt, 0);
    Val* prev = eval(args->car, env);
    check_typ(PRIM_LT, prev, TY_INT);
    for (Val* a = args->cdr; a != EMPTY_LIST; a = a->cdr) {
        Val* curr = eval(a->car, env);
        check_typ(PRIM_LT, curr, TY_INT);
        if (prev->num >= curr->num) {
            return FALSE;
        }
        prev = curr;
    }
    return TRUE;
}

static Val* prim_gt(Val* args, Val* env) {
    check_len(PRIM_GT, args, gt, 0);
    Val* prev = eval(args->car, env);
    check_typ(PRIM_GT, prev, TY_INT);
    for (Val* a = args->cdr; a != EMPTY_LIST; a = a->cdr) {
        Val* curr = eval(a->car, env);
        check_typ(PRIM_GT, curr, TY_INT);
        if (prev->num <= curr->num) {
            return FALSE;
        }
        prev = curr;
    }
    return TRUE;
}

static Val* prim_num_eq(Val* args, Val* env) {
    check_len(PRIM_NUM_EQ, args, gt, 0);
    Val* prev = eval(args->car, env);
    check_typ(PRIM_NUM_EQ, prev, TY_INT);
    for (Val* a = args->cdr; a != EMPTY_LIST; a = a->cdr) {
        Val* curr = eval(a->car, env);
        check_typ(PRIM_NUM_EQ, curr, TY_INT);
        if (prev->num != curr->num) {
            return FALSE;
        }
        prev = curr;
    }
    return TRUE;
}

static Val* prim_eq(Val* args, Val* env) {
    check_len(PRIM_EQ, args, eq, 2);
    Val* l = eval(args->car, env);
    Val* r = eval(args->cdr->car, env);
    if (l->ty == TY_INT && r->ty == TY_INT) {
        return l->num == r->num ? TRUE : FALSE;
    } else {
        return l == r ? TRUE : FALSE;
    }
}

static Val* prim_car(Val* args, Val* env) {
    check_len(PRIM_CAR, args, eq, 1);
    Val* val = eval(args->car, env);
    check_typ(PRIM_CAR, val, TY_PAIR);
    return val->car;
}

static Val* prim_cdr(Val* args, Val* env) {
    check_len(PRIM_CDR, args, eq, 1);
    Val* val = eval(args->car, env);
    check_typ(PRIM_CDR, val, TY_PAIR);
    return val->cdr;
}

static Val* prim_cons(Val* args, Val* env) {
    check_len(PRIM_CONS, args, eq, 2);
    Val* head = eval(args->car, env);
    Val* tail = eval(args->cdr->car, env);
    return cons(head, tail);
}

// Note: only `#f` is considered false in conditional expressions.
static Val* prim_if(Val* args, Val* env) {
    check_len(PRIM_IF, args, gt, 1);
    Val* test = eval(args->car, env);
    if (test != FALSE) {
        Val* con = args->cdr;
        return eval(con->car, env);
    } else {
        Val* alt = args->cdr->cdr;
        // If test yields a false value and no alternate is specified, the
        // result of the expression is unspecified.
        return alt == EMPTY_LIST ? NULL : eval(alt->car, env);
    }
}

static Val* prim_and(Val* args, Val* env) {
    Val* ret = TRUE;
    for (Val* test = args; test != EMPTY_LIST; test = test->cdr) {
        ret = eval(test->car, env);
        if (ret == FALSE) {
            break;
        }
    }
    return ret;
}

static Val* prim_or(Val* args, Val* env) {
    Val* ret = FALSE;
    for (Val* test = args; test != EMPTY_LIST; test = test->cdr) {
        ret = eval(test->car, env);
        if (ret != FALSE) {
            break;
        }
    }
    return ret;
}

static Val* prim_not(Val* args, Val* env) {
    check_len(PRIM_NOT, args, eq, 1);
    Val* val = eval(args->car, env);
    if (val == FALSE) {
        return TRUE;
    } else {
        return FALSE;
    }
}

static Val* prim_define(Val* args, Val* env) {
    check_len(PRIM_DEFINE, args, eq, 2);
    check_typ(PRIM_DEFINE, args->car, TY_SYMBOL);
    Val* var = args->car;
    Val* val = eval(args->cdr->car, env);
    define_variable(var, val, env);
    return NULL;
}

static Val* prim_quote(Val* args, Val* env) {
    check_len(PRIM_QUOTE, args, eq, 1);
    return args->car;
}

static void add_prim_proc(char* name, PrimProc* p, Val* env) {
    Val* sym = intern_symbol(name);
    Val* proc = make_prim_proc(p);
    define_variable(sym, proc, env);
}

static void define_prim_procs(Val* env) {
    add_prim_proc(PRIM_ADD, prim_add, env);
    add_prim_proc(PRIM_SUB, prim_sub, env);
    add_prim_proc(PRIM_MUL, prim_mul, env);
    add_prim_proc(PRIM_DIV, prim_div, env);
    add_prim_proc(PRIM_ABS, prim_abs, env);

    add_prim_proc(PRIM_LT, prim_lt, env);
    add_prim_proc(PRIM_GT, prim_gt, env);
    add_prim_proc(PRIM_NUM_EQ, prim_num_eq, env);
    add_prim_proc(PRIM_EQ, prim_eq, env);

    add_prim_proc(PRIM_CAR, prim_car, env);
    add_prim_proc(PRIM_CDR, prim_cdr, env);
    add_prim_proc(PRIM_CONS, prim_cons, env);

    add_prim_proc(PRIM_IF, prim_if, env);
    add_prim_proc(PRIM_AND, prim_and, env);
    add_prim_proc(PRIM_OR, prim_or, env);
    add_prim_proc(PRIM_NOT, prim_not, env);

    add_prim_proc(PRIM_DEFINE, prim_define, env);
    add_prim_proc(PRIM_QUOTE, prim_quote, env);
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
    case TY_PRIM_PROC:
        printf("#<primitive-procedure>");
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
    define_prim_procs(global_env);
    for (;;) {
        Val* val = read();
        if (val != NULL) {
            val = eval(val, global_env);
            if (val != NULL) {
                print(val);
                printf("\n");
            }
        } else {
            return 0;
        }
    }
}
