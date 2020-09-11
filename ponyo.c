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
    TY_COMP_PROC,
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
        // Compound procedure.
        struct {
            Val* params;
            Val* body;
            Val* env;
        };
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

static Val* make_comp_proc(Val* params, Val* body, Val* env) {
    Val* val = malloc_val(TY_COMP_PROC);
    val->params = params;
    val->body = body;
    val->env = env;
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
static Val* intern_symbol(char* str) {
    for (Val* s = symbol_list; s != EMPTY_LIST; s = s->cdr) {
        if (strcmp(str, s->car->str) == 0) {
            return s->car;
        }
    }
    Val* sym = make_string_or_symbol(TY_SYMBOL, str);
    symbol_list = cons(sym, symbol_list);
    return sym;
}

/*------------------------------------------------------------------------------
 | PARSER
 -----------------------------------------------------------------------------*/

#define STRING_MAX_LEN 2000
#define SYMBOL_MAX_LEN 200

const char extended_symbols[] = "!$%&*+-./:<=>?@^_~";

static Val* read_c(FILE* fp, int c);
static Val* read(FILE* fp);

static int peek(FILE* fp) {
    int c = getc(fp);
    ungetc(c, fp);
    return c;
}

static int get_non_whitespace_char(FILE* fp) {
    for (;;) {
        int c = getc(fp);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            continue;
        } else if (c == ';') {
            do {
                c = getc(fp);
            } while (c != '\n' && c != EOF);
        } else {
            return c;
        }
    }
}

// Assumes a '#' has already been read.
static Val* read_bool(FILE* fp) {
    int c = getc(fp);
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

static int read_int(FILE* fp, int num) {
    while (isdigit(peek(fp))) {
        num = num * 10 + (getc(fp) - '0');
    }
    return num;
}

// Assumes a '(' has already been read.
static Val* read_list(FILE* fp, int c) {
    if (c == EOF) {
        ERROR("unterminated list");
    } else if (c == ')') {
        return EMPTY_LIST;
    }
    // NULL check unnecessary due to earlier EOF check.
    Val* car = read_c(fp, c);

    c = get_non_whitespace_char(fp);
    if (c == '.') {
        // NULL check unnecessary due to subsequent ')' check.
        Val* cdr = read(fp);
        if (get_non_whitespace_char(fp) != ')') {
            ERROR("expected list terminator");
        }
        return cons(car, cdr);
    } else {
        Val* cdr = read_list(fp, c);
        return cons(car, cdr);
    }
}

static Val* read_quote(FILE* fp) {
    Val* quote = read(fp);
    if (quote == NULL) {
        ERROR("unexpected EOF reading quote");
    }
    Val* sym = intern_symbol("quote");
    return cons(sym, cons(quote, EMPTY_LIST));
}

static Val* read_string(FILE* fp) {
    char buffer[STRING_MAX_LEN + 1];
    int i = 0;
    for (int c = getc(fp); c != '"'; c = getc(fp)) {
        if (c == EOF) {
            ERROR("unterminated string");
        } else if (c == '\\') {
            c = getc(fp);
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
    }
    buffer[i] = '\0';
    return make_string_or_symbol(TY_STRING, buffer);
}

static Val* read_symbol(FILE* fp, int c) {
    char buffer[SYMBOL_MAX_LEN + 1];
    buffer[0] = c;
    int i = 1;
    while (isalnum(peek(fp)) || strchr(extended_symbols, peek(fp))) {
        if (i < SYMBOL_MAX_LEN) {
            buffer[i++] = getc(fp);
        } else {
            ERROR("identifier too long")
        }
    }
    buffer[i] = '\0';
    return intern_symbol(buffer);
}

static Val* read_c(FILE* fp, int c) {
    if (c == EOF) {
        return NULL;
    }
    if (c == '#') {
        return read_bool(fp);
    }
    if (c == '"') {
        return read_string(fp);
    }
    if (c == '(') {
        return read_list(fp, get_non_whitespace_char(fp));
    }
    if (c == '\'') {
        return read_quote(fp);
    }
    if (isdigit(c)) {
        return make_int(read_int(fp, c - '0'));
    }
    if (c == '-' && isdigit(peek(fp))) {
        return make_int(-read_int(fp, getc(fp) - '0'));
    }
    if (isalpha(c) || strchr(extended_symbols, c)) {
        return read_symbol(fp, c);
    }
    ERROR("unexpected character '%c'", c);
}

static Val* read(FILE* fp) {
    int c = get_non_whitespace_char(fp);
    return read_c(fp, c);
}

/*------------------------------------------------------------------------------
 | LIST HELPERS
 -----------------------------------------------------------------------------*/

static int len(Val* list) {
    int len = 0;
    for (; list->ty == TY_PAIR; list = list->cdr) {
        len++;
    }
    // -1 if list is improper.
    return list == EMPTY_LIST ? len : -1;
}

static Val* rev(Val* list) {
    Val* prev = EMPTY_LIST;
    while (list != EMPTY_LIST) {
        Val* next = list->cdr;
        list->cdr = prev;
        prev = list;
        list = next;
    }
    return prev;
}

/*------------------------------------------------------------------------------
 | ENVIRONMENT
 -----------------------------------------------------------------------------*/

static Val* extend_env(Val* vars, Val* vals, Val* env) {
    if (len(vars) != len(vals)) {
        ERROR("length of vars and vals do not match");
    }
    return cons(cons(vars, vals), env);
}

static Val* lookup_variable(Val* var, Val* env) {
    for (; env != EMPTY_LIST; env = env->cdr) {
        Val* frame = env->car;
        Val* vars = frame->car;
        Val* vals = frame->cdr;
        for (; vars != EMPTY_LIST; vars = vars->cdr, vals = vals->cdr) {
            // Note: `strcmp` is unnecessary. Symbols are interned, so
            // pointer comparison is sufficient.
            if (var == vars->car) {
                return vals->car;
            }
        }
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
    for (; vars != EMPTY_LIST; vars = vars->cdr, vals = vals->cdr) {
        // Change the binding if it exists.
        if (var == vars->car) {
            vals->car = val;
            return;
        }
    }
    // If binding doesn't exist, adjoin one to the first frame.
    add_binding(var, val, first_frame);
}

/*------------------------------------------------------------------------------
 | EVALUATOR
 -----------------------------------------------------------------------------*/

static Val* eval(Val* val, Val* env);

static Val* apply(Val* proc, Val* args, Val* env) {
    if (proc->ty == TY_PRIM_PROC) {
        return proc->proc(args, env);
    } else if (proc->ty == TY_COMP_PROC) {
        Val* params = proc->params;
        if (len(params) != len(args)) {
            ERROR("incorrect number of arguments to procedure");
        }

        // Extend the base environment carried by the procedure to include a
        // frame that binds the parameters of the procedure to the arguments
        // to which the procedure is to be applied.
        Val* vars = proc->params;
        Val* vals = EMPTY_LIST;
        for (; args != EMPTY_LIST; args = args->cdr) {
            vals = cons(eval(args->car, env), vals);
        }
        Val* penv = extend_env(vars, rev(vals), proc->env);

        // Evaluate the expressions in the procedure body, returning the
        // result of the final expression.
        Val* result;
        for (Val* b = proc->body; b != EMPTY_LIST; b = b->cdr) {
            result = eval(b->car, penv);
        }
        return result;
    } else {
        ERROR("unknown procedure type");
    }
}

static Val* eval(Val* val, Val* env) {
    switch (val->ty) {
    case TY_FALSE:
    case TY_TRUE:
    case TY_COMP_PROC:
    case TY_INT:
    case TY_PRIM_PROC:
    case TY_STRING:
        return val;
    case TY_EMPTY_LIST:
        ERROR("empty application: ()");
    case TY_PAIR: {
        Val* proc = eval(val->car, env);
        Val* args = val->cdr;
        return apply(proc, args, env);
    }
    case TY_SYMBOL:
        return lookup_variable(val, env);
    }
    return NULL;
}

/*------------------------------------------------------------------------------
 | PRIMITIVE PROCEDURES
 -----------------------------------------------------------------------------*/

#define PRIM_ADD     "+"
#define PRIM_SUB     "-"
#define PRIM_MUL     "*"
#define PRIM_DIV     "/"
#define PRIM_ABS     "abs"
#define PRIM_LT      "<"
#define PRIM_GT      ">"
#define PRIM_NUM_EQ  "="
#define PRIM_EQ      "eq?"
#define PRIM_CAR     "car"
#define PRIM_CDR     "cdr"
#define PRIM_CONS    "cons"
#define PRIM_COND    "cond"
#define PRIM_IF      "if"
#define PRIM_AND     "and"
#define PRIM_OR      "or"
#define PRIM_NOT     "not"
#define PRIM_DEFINE  "define"
#define PRIM_LAMBDA  "lambda"
#define PRIM_LET     "let"
#define PRIM_LIST    "list"
#define PRIM_QUOTE   "quote"
#define PRIM_SET     "set!"
#define PRIM_SET_CAR "set-car!"
#define PRIM_SET_CDR "set-cdr!"
#define PRIM_IS_INT  "integer?"
#define PRIM_IS_NULL "null?"
#define PRIM_IS_PAIR "pair?"
#define PRIM_IS_PROC "procedure?"
#define PRIM_IS_STR  "string?"
#define PRIM_IS_SYM  "symbol?"
#define PRIM_DISPLAY "display"
#define PRIM_LOAD    "load"

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
        char* ty;
        switch (exp) {
        case TY_FALSE:
        case TY_TRUE:
            ty = "boolean";
            break;
        case TY_INT:
            ty = "number";
            break;
        case TY_PAIR:
            ty = "pair";
            break;
        case TY_STRING:
            ty = "string";
            break;
        case TY_SYMBOL:
            ty = "symbol";
            break;
        default:
            ERROR("argument is not of the expected type");
        }
        ERROR("%s: argument is not a %s", proc, ty);
    }
}

static Val* prim_add(Val* args, Val* env) {
    int sum = 0;
    for (; args != EMPTY_LIST; args = args->cdr) {
        Val* num = eval(args->car, env);
        check_typ(PRIM_ADD, num, TY_INT);
        sum += num->num;
    }
    return make_int(sum);
}

static Val* prim_sub(Val* args, Val* env) {
    check_len(PRIM_SUB, args, gt, 0);
    Val* num = eval(args->car, env);
    check_typ(PRIM_SUB, num, TY_INT);
    if (args->cdr == EMPTY_LIST) {
        return make_int(-num->num);
    }
    int sum = num->num;
    for (args = args->cdr; args != EMPTY_LIST; args = args->cdr) {
        num = eval(args->car, env);
        check_typ(PRIM_SUB, num, TY_INT);
        sum -= num->num;
    }
    return make_int(sum);
}

static Val* prim_mul(Val* args, Val* env) {
    int sum = 1;
    for (; args != EMPTY_LIST; args = args->cdr) {
        Val* num = eval(args->car, env);
        check_typ(PRIM_MUL, num, TY_INT);
        sum *= num->num;
    }
    return make_int(sum);
}

// No support for rational values (yet?).
static Val* prim_div(Val* args, Val* env) {
    check_len(PRIM_DIV, args, gt, 0);
    Val* num = eval(args->car, env);
    check_typ(PRIM_DIV, num, TY_INT);
    if (args->cdr == EMPTY_LIST) {
        return make_int(num->num);
    }
    int sum = num->num;
    for (args = args->cdr; args != EMPTY_LIST; args = args->cdr) {
        num = eval(args->car, env);
        check_typ(PRIM_DIV, num, TY_INT);
        sum /= num->num;
    }
    return make_int(sum);
}

static Val* prim_abs(Val* args, Val* env) {
    check_len(PRIM_ABS, args, eq, 1);
    Val* num = eval(args->car, env);
    check_typ(PRIM_ABS, num, TY_INT);
    return num->num < 0 ? make_int(-num->num) : make_int(num->num);
}

static Val* prim_lt(Val* args, Val* env) {
    check_len(PRIM_LT, args, gt, 0);
    Val* prev = eval(args->car, env);
    check_typ(PRIM_LT, prev, TY_INT);
    for (args = args->cdr; args != EMPTY_LIST; args = args->cdr) {
        Val* curr = eval(args->car, env);
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
    for (args = args->cdr; args != EMPTY_LIST; args = args->cdr) {
        Val* curr = eval(args->car, env);
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
    for (args = args->cdr; args != EMPTY_LIST; args = args->cdr) {
        Val* curr = eval(args->car, env);
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
    Val* list = eval(args->car, env);
    check_typ(PRIM_CAR, list, TY_PAIR);
    return list->car;
}

static Val* prim_cdr(Val* args, Val* env) {
    check_len(PRIM_CDR, args, eq, 1);
    Val* list = eval(args->car, env);
    check_typ(PRIM_CDR, list, TY_PAIR);
    return list->cdr;
}

static Val* prim_cons(Val* args, Val* env) {
    check_len(PRIM_CONS, args, eq, 2);
    Val* head = eval(args->car, env);
    Val* tail = eval(args->cdr->car, env);
    return cons(head, tail);
}

static Val* prim_cond(Val* args, Val* env) {
    check_len(PRIM_COND, args, gt, 0);
    for (Val* a = args; a != EMPTY_LIST; a = a->cdr) {
        check_typ(PRIM_COND, a->car, TY_PAIR);
        Val* test = a->car->car;
        Val* exps = a->car->cdr;
        if (test == intern_symbol("else")) {
            if (a->cdr != EMPTY_LIST) {
                ERROR("%s: else clause must be last", PRIM_COND);
            }
        } else {
            if (eval(test, env) == FALSE) {
                continue;
            }
        }
        Val* result;
        for (; exps != EMPTY_LIST; exps = exps->cdr) {
            result = eval(exps->car, env);
        }
        return result;
    }
    return NULL;
}

// Note: only `#f` is considered false in conditional expressions.
static Val* prim_if(Val* args, Val* env) {
    check_len(PRIM_IF, args, gt, 1);
    Val* test = eval(args->car, env);
    if (test != FALSE) {
        Val* conseq = args->cdr;
        return eval(conseq->car, env);
    } else {
        // If test yields a false value and no alternate is specified, the
        // result of the expression is unspecified.
        Val* altern = args->cdr->cdr;
        return altern == EMPTY_LIST ? NULL : eval(altern->car, env);
    }
}

static Val* prim_and(Val* args, Val* env) {
    Val* result = TRUE;
    for (; args != EMPTY_LIST; args = args->cdr) {
        result = eval(args->car, env);
        if (result == FALSE) {
            break;
        }
    }
    return result;
}

static Val* prim_or(Val* args, Val* env) {
    Val* result = FALSE;
    for (; args != EMPTY_LIST; args = args->cdr) {
        result = eval(args->car, env);
        if (result != FALSE) {
            break;
        }
    }
    return result;
}

static Val* prim_not(Val* args, Val* env) {
    check_len(PRIM_NOT, args, eq, 1);
    Val* result = eval(args->car, env);
    if (result == FALSE) {
        return TRUE;
    } else {
        return FALSE;
    }
}

static Val* prim_define_proc(Val* args, Val* env) {
    Val* params = args->car->cdr;
    for (Val* p = params; p != EMPTY_LIST; p = p->cdr) {
        check_typ(PRIM_DEFINE, p->car, TY_SYMBOL);
    }
    Val* body = args->cdr;
    return make_comp_proc(params, body, env);
}

static Val* prim_define(Val* args, Val* env) {
    check_len(PRIM_DEFINE, args, gt, 1);
    Val* var = args->car;
    if (var->ty == TY_SYMBOL) {
        check_len(PRIM_DEFINE, args->cdr, eq, 1);
        Val* val = eval(args->cdr->car, env);
        define_variable(var, val, env);
    } else if (var->ty == TY_PAIR) {
        var = var->car;
        Val* val = prim_define_proc(args, env);
        define_variable(var, val, env);
    } else {
        ERROR("%s: argument is not a symbol or a pair", PRIM_DEFINE);
    }
    return NULL;
}

static Val* prim_lambda(Val* args, Val* env) {
    check_len(PRIM_LAMBDA, args, gt, 1);
    Val* params = args->car;
    for (Val* p = params; p != EMPTY_LIST; p = p->cdr) {
        check_typ(PRIM_LAMBDA, p->car, TY_SYMBOL);
    }
    Val* body = args->cdr;
    return make_comp_proc(params, body, env);
}

// Desugars `(let ((var val)) body)` to `((lambda (var) body) val)`.
static Val* prim_let(Val* args, Val* env) {
    check_len(PRIM_LET, args, gt, 1);
    Val* vars = EMPTY_LIST;
    Val* vals = EMPTY_LIST;
    for (Val* b = args->car; b != EMPTY_LIST; b = b->cdr) {
        check_typ(PRIM_LET, b->car, TY_PAIR);
        check_len(PRIM_LET, b->car, eq, 2);

        Val* var = b->car->car;
        check_typ(PRIM_LET, var, TY_SYMBOL);
        vars = cons(var, vars);

        Val* val = b->car->cdr->car;
        vals = cons(val, vals);
    }
    Val* body = args->cdr;
    Val* lambda = make_comp_proc(vars, body, env);
    return eval(cons(lambda, vals), env);
}

static Val* prim_list(Val* args, Val* env) {
    Val* list = EMPTY_LIST;
    for (; args != EMPTY_LIST; args = args->cdr) {
        list = cons(eval(args->car, env), list);
    }
    return rev(list);
}

static Val* prim_quote(Val* args, Val* env) {
    check_len(PRIM_QUOTE, args, eq, 1);
    return args->car;
}

static Val* prim_set(Val* args, Val* env) {
    check_len(PRIM_SET, args, eq, 2);
    Val* var = args->car;
    check_typ(PRIM_SET, var, TY_SYMBOL);
    Val* val = eval(args->cdr->car, env);
    define_variable(var, val, env);
    return NULL;
}

static Val* prim_set_car(Val* args, Val* env) {
    check_len(PRIM_SET_CAR, args, eq, 2);
    Val* var = eval(args->car, env);
    check_typ(PRIM_SET_CAR, var, TY_PAIR);
    Val* val = eval(args->cdr->car, env);
    var->car = val;
    return NULL;
}

static Val* prim_set_cdr(Val* args, Val* env) {
    check_len(PRIM_SET_CDR, args, eq, 2);
    Val* var = eval(args->car, env);
    check_typ(PRIM_SET_CDR, var, TY_PAIR);
    Val* val = eval(args->cdr->car, env);
    var->cdr = val;
    return NULL;
}

static void add_prim_proc(char* name, PrimProc* p, Val* env) {
    Val* sym = intern_symbol(name);
    Val* proc = make_prim_proc(p);
    define_variable(sym, proc, env);
}

static Val* prim_is_int(Val* args, Val* env) {
    check_len(PRIM_IS_INT, args, eq, 1);
    Val* val = eval(args->car, env);
    return val->ty == TY_INT ? TRUE : FALSE;
}

static Val* prim_is_null(Val* args, Val* env) {
    check_len(PRIM_IS_NULL, args, eq, 1);
    Val* val = eval(args->car, env);
    return val->ty == TY_EMPTY_LIST ? TRUE : FALSE;
}

static Val* prim_is_pair(Val* args, Val* env) {
    check_len(PRIM_IS_PAIR, args, eq, 1);
    Val* val = eval(args->car, env);
    return val->ty == TY_PAIR ? TRUE : FALSE;
}

static Val* prim_is_proc(Val* args, Val* env) {
    check_len(PRIM_IS_PROC, args, eq, 1);
    Val* val = eval(args->car, env);
    return val->ty == TY_COMP_PROC || val->ty == TY_PRIM_PROC ? TRUE : FALSE;
}

static Val* prim_is_str(Val* args, Val* env) {
    check_len(PRIM_IS_STR, args, eq, 1);
    Val* val = eval(args->car, env);
    return val->ty == TY_STRING ? TRUE : FALSE;
}

static Val* prim_is_sym(Val* args, Val* env) {
    check_len(PRIM_IS_SYM, args, eq, 1);
    Val* val = eval(args->car, env);
    return val->ty == TY_SYMBOL ? TRUE : FALSE;
}

static void print(Val* val);

static Val* prim_display(Val* args, Val* env) {
    check_len(PRIM_DISPLAY, args, eq, 1);
    Val* val = eval(args->car, env);
    if (val->ty == TY_STRING) {
        printf("%s", val->str);
    } else {
        print(val);
    }
    return NULL;
}

static void load(FILE* fp, char print_vals, Val* env);

static Val* prim_load(Val* args, Val* env) {
    check_len(PRIM_LOAD, args, eq, 1);
    check_typ(PRIM_LOAD, args->car, TY_STRING);
    char* path = args->car->str;

    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        ERROR("could not load '%s'", path);
    }
    load(fp, 0, env);
    fclose(fp);
    return NULL;
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

    add_prim_proc(PRIM_COND, prim_cond, env);
    add_prim_proc(PRIM_IF, prim_if, env);
    add_prim_proc(PRIM_AND, prim_and, env);
    add_prim_proc(PRIM_OR, prim_or, env);
    add_prim_proc(PRIM_NOT, prim_not, env);

    add_prim_proc(PRIM_DEFINE, prim_define, env);
    add_prim_proc(PRIM_LAMBDA, prim_lambda, env);
    add_prim_proc(PRIM_LET, prim_let, env);
    add_prim_proc(PRIM_LIST, prim_list, env);
    add_prim_proc(PRIM_QUOTE, prim_quote, env);

    add_prim_proc(PRIM_SET, prim_set, env);
    add_prim_proc(PRIM_SET_CAR, prim_set_car, env);
    add_prim_proc(PRIM_SET_CDR, prim_set_cdr, env);

    add_prim_proc(PRIM_IS_INT, prim_is_int, env);
    add_prim_proc(PRIM_IS_NULL, prim_is_null, env);
    add_prim_proc(PRIM_IS_PAIR, prim_is_pair, env);
    add_prim_proc(PRIM_IS_PROC, prim_is_proc, env);
    add_prim_proc(PRIM_IS_STR, prim_is_str, env);
    add_prim_proc(PRIM_IS_SYM, prim_is_sym, env);

    add_prim_proc(PRIM_DISPLAY, prim_display, env);
    add_prim_proc(PRIM_LOAD, prim_load, env);
}

/*------------------------------------------------------------------------------
 | PRINTER
 -----------------------------------------------------------------------------*/

static void print_list(Val* list) {
    printf("(");
    print(list->car);
    for (list = list->cdr; list->ty == TY_PAIR; list = list->cdr) {
        printf(" ");
        print(list->car);
    }
    if (list != EMPTY_LIST) {
        printf(" . ");
        print(list);
    }
    printf(")");
}

static void print_string(Val* str) {
    printf("\"");
    for (char* c = str->str; *c != '\0'; c++) {
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
        case '\\':
            printf("\\\\");
            break;
        case '"':
            printf("\\\"");
            break;
        default:
            printf("%c", *c);
            break;
        }
    }
    printf("\"");
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
    case TY_COMP_PROC:
        printf("#<compound-procedure>");
        break;
    case TY_INT:
        printf("%d", val->num);
        break;
    case TY_PAIR:
        print_list(val);
        break;
    case TY_PRIM_PROC:
        printf("#<primitive-procedure>");
        break;
    case TY_STRING:
        print_string(val);
        break;
    case TY_SYMBOL:
        printf("%s", val->str);
        break;
    }
}

/*------------------------------------------------------------------------------
 | PONYO!
 -----------------------------------------------------------------------------*/

static void load(FILE* fp, char print_vals, Val* env) {
    for (Val* val = read(fp); val != NULL; val = read(fp)) {
        val = eval(val, env);
        if (val != NULL && print_vals) {
            print(val);
            printf("\n");
        }
    }
}

int main(void) {
    symbol_list = EMPTY_LIST;
    global_env = EMPTY_LIST;
    global_env = extend_env(EMPTY_LIST, EMPTY_LIST, global_env);

    define_prim_procs(global_env);
    load(stdin, 1, global_env);

    return 0;
}
