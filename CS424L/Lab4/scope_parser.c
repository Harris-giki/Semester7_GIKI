/*
 * CS424L - Compiler Construction Lab | Lab 4 | Exercise 1
 *
 * Question:
 * 1. Add support for declarations and scopes to the parser for assignment statements from the
 * previous lab. Assignment statements are of the form id= expr;. Each variable must be
 * declared before it can be used. To do this write a grammar. Convert the grammar into a
 * syntax directed translation scheme. Then implement the SDT inside a predictive parser.
 * For example, a valid input can be of the form:
 *     {
 *     int x ;
 *     int y ;
 *     x = 2 + 3*4;
 *     {
 *     int x ;
 *     x = 5+10;
 *     }
 *     y = 4 + x ;
 *     }
 * Another example:
 *     {
 *     int x ;
 *     int y ;
 *     bool y ;
 *     x = 2 + 3*4;
 *     y = 4 + x ;
 *     }
 * This should show a multiple declarations error, since y has been declared twice.
 *
 * ---------------------------------------------------------------------------
 * 1) Grammar (left-recursion free, so it is LL(1)):
 *
 *     program -> block
 *     block   -> { decls stmts }
 *     decls   -> decl decls | e
 *     decl    -> type id ;
 *     type    -> int | bool
 *     stmts   -> stmt stmts | e
 *     stmt    -> id = expr ;
 *              | block
 *     expr    -> term expr'
 *     expr'   -> + term expr' | e
 *     term    -> factor term'
 *     term'   -> * factor term' | e
 *     factor  -> id | num | ( expr )
 *
 * 2) Syntax directed translation scheme (actions in { braces } run at that point of the body).
 *    "top" is the symbol table of the innermost scope; each table points to its enclosing one.
 *
 *     program -> { top = null; }
 *                block
 *     block   -> '{'  { saved = top; top = new Env(top); }      // open a new scope
 *                decls stmts
 *                '}'  { top = saved; }                          // close the scope
 *     decl    -> type id ;
 *                     { if top.containsLocally(id.lexeme)
 *                           error("multiple declarations of " + id.lexeme);
 *                       else top.put(id.lexeme, type.t); }
 *     type    -> int  { type.t = "int"; }
 *              | bool { type.t = "bool"; }
 *     stmt    -> id   { if top.get(id.lexeme) == null error(id.lexeme + " is not declared"); }
 *                = expr ;  { print(id.lexeme " = " expr.postfix); }
 *     factor  -> id   { if top.get(id.lexeme) == null error(id.lexeme + " is not declared");
 *                       print(id.lexeme); }
 *              | num  { print(num.value); }
 *              | ( expr )
 *     expr'   -> + term { print("+"); } expr'
 *     term'   -> * factor { print("*"); } term'
 *
 *    top.get(name) searches the current scope first and then each enclosing scope, so an
 *    inner "int x" hides the outer x (first example). containsLocally only looks at the
 *    current scope, so a name may be declared again in an inner block, but not twice in
 *    the same block (second example).
 *
 * 3) The SDT is implemented below as a recursive-descent predictive parser: one function per
 *    non-terminal, with the semantic actions at the same positions as in the scheme.
 *    Every variable in the output is tagged with the scope depth it resolves to, e.g. x@2.
 *
 * Build & run:
 *     gcc scope_parser.c -o scope_parser
 *     ./scope_parser tests/valid.txt
 *     ./scope_parser tests/multiple_decl.txt
 *     ./scope_parser tests/undeclared.txt
 * ---------------------------------------------------------------------------
 */

#include <ctype.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ----------------------------- Lexical analyzer ----------------------------- */

typedef enum { ID, NUM, INT, BOOL, ASSIGN, PLUS, MUL, LPAREN, RPAREN, LBRACE, RBRACE, SEMI, END, INVALID } TokenType;

typedef struct {
    TokenType type;
    char lexeme[64];
    int line;
} Token;

static char  *src;
static size_t pos = 0;
static int    line = 1;

Token next_token(void) {
    Token t;
    for (;;) {
        if (src[pos] == '\n') { line++; pos++; }
        else if (isspace((unsigned char)src[pos])) pos++;
        else if (src[pos] == '/' && src[pos + 1] == '/') { while (src[pos] && src[pos] != '\n') pos++; }
        else break;
    }
    t.line = line;
    size_t start = pos;
    char c = src[pos];

    if (c == '\0') { t.type = END; strcpy(t.lexeme, "end of input"); return t; }

    if (isalpha((unsigned char)c) || c == '_') {
        while (isalnum((unsigned char)src[pos]) || src[pos] == '_') pos++;
        t.type = ID;
    } else if (isdigit((unsigned char)c)) {
        while (isdigit((unsigned char)src[pos])) pos++;
        t.type = NUM;
    } else {
        pos++;
        switch (c) {
            case '=': t.type = ASSIGN; break;
            case '+': t.type = PLUS;   break;
            case '*': t.type = MUL;    break;
            case '(': t.type = LPAREN; break;
            case ')': t.type = RPAREN; break;
            case '{': t.type = LBRACE; break;
            case '}': t.type = RBRACE; break;
            case ';': t.type = SEMI;   break;
            default:  t.type = INVALID;
        }
    }
    size_t n = pos - start;
    if (n > 63) n = 63;
    memcpy(t.lexeme, src + start, n);
    t.lexeme[n] = '\0';

    if (t.type == ID) {
        if (strcmp(t.lexeme, "int") == 0)  t.type = INT;
        if (strcmp(t.lexeme, "bool") == 0) t.type = BOOL;
    }
    return t;
}

/* ------------------------------ Symbol tables ------------------------------- */

#define MAX_SYMS 64

/* One Env per block. prev points to the enclosing block: a chain of symbol tables. */
typedef struct Env {
    char names[MAX_SYMS][64];
    char types[MAX_SYMS][8];
    int  count;
    int  depth;
    struct Env *prev;
} Env;

Env *new_env(Env *prev) {
    Env *e = calloc(1, sizeof(Env));
    e->prev = prev;
    e->depth = prev ? prev->depth + 1 : 1;
    return e;
}

/* index of name in this scope only, or -1 */
int lookup_local(Env *e, const char *name) {
    for (int i = 0; i < e->count; i++)
        if (strcmp(e->names[i], name) == 0) return i;
    return -1;
}

void put(Env *e, const char *name, const char *type) {
    strcpy(e->names[e->count], name);
    strcpy(e->types[e->count], type);
    e->count++;
}

/* search this scope, then the enclosing ones; return the Env that declares name, or NULL */
Env *get(Env *e, const char *name) {
    for (; e; e = e->prev)
        if (lookup_local(e, name) >= 0) return e;
    return NULL;
}

/* ---------------------------- Parser + SDT actions --------------------------- */

static Token   look;
static Env    *top = NULL;           /* innermost scope */
static char    out[1024];            /* postfix of the current right-hand side */
static jmp_buf on_error;
static char    err_msg[256];

void fail(const char *fmt, const char *arg) {
    char msg[200];
    snprintf(msg, sizeof msg, fmt, arg);
    snprintf(err_msg, sizeof err_msg, "line %d: %s", look.line, msg);
    longjmp(on_error, 1);
}

void match(TokenType t, const char *what) {
    if (look.type != t) {
        char msg[128];
        snprintf(msg, sizeof msg, "syntax error: expected %s but found '%%s'", what);
        fail(msg, look.lexeme);
    }
    look = next_token();
}

void indent(void) { printf("%*s", top ? top->depth * 2 : 0, ""); }

void print_out(const char *s) {
    strncat(out, s, sizeof out - strlen(out) - 2);
    strcat(out, " ");
}

/* semantic action: name must be declared; returns "name@depth" */
const char *resolve(const char *name) {
    static char buf[80];
    Env *e = get(top, name);
    if (!e) fail("semantic error: variable '%s' is used before it is declared", name);
    snprintf(buf, sizeof buf, "%s@%d", name, e->depth);
    return buf;
}

void expr(void);
void block(void);

/* factor -> id | num | ( expr ) */
void factor(void) {
    if (look.type == ID) {
        print_out(resolve(look.lexeme));
        match(ID, "an identifier");
    } else if (look.type == NUM) {
        print_out(look.lexeme);
        match(NUM, "a number");
    } else if (look.type == LPAREN) {
        match(LPAREN, "'('"); expr(); match(RPAREN, "')'");
    } else {
        fail("syntax error: expected id, num or '(' but found '%s'", look.lexeme);
    }
}

/* term -> factor term'        term' -> * factor { print("*"); } term' | e */
void term(void) {
    factor();
    while (look.type == MUL) {
        match(MUL, "'*'");
        factor();
        print_out("*");
    }
}

/* expr -> term expr'          expr' -> + term { print("+"); } expr' | e */
void expr(void) {
    term();
    while (look.type == PLUS) {
        match(PLUS, "'+'");
        term();
        print_out("+");
    }
}

/* stmt -> id = expr ; | block */
void stmt(void) {
    if (look.type == LBRACE) { block(); return; }

    char target[80];
    strcpy(target, resolve(look.lexeme));       /* { id must be declared } */
    match(ID, "an identifier");
    match(ASSIGN, "'='");
    out[0] = '\0';
    expr();
    match(SEMI, "';'");
    indent();
    printf("%s = %s  (postfix)\n", target, out);
}

/* stmts -> stmt stmts | e       FOLLOW(stmts) = { '}' } */
void stmts(void) {
    while (look.type == ID || look.type == LBRACE) stmt();
    if (look.type == INT || look.type == BOOL)
        fail("syntax error: declaration of '%s' after a statement (declarations come first in a block)",
             look.lexeme);
}

/* type -> int | bool */
const char *type(void) {
    if (look.type == INT)  { match(INT, "'int'");   return "int"; }
    if (look.type == BOOL) { match(BOOL, "'bool'"); return "bool"; }
    fail("syntax error: expected a type but found '%s'", look.lexeme);
    return NULL;
}

/* decl -> type id ; */
void decl(void) {
    const char *t = type();
    char name[64];
    strcpy(name, look.lexeme);
    int decl_line = look.line;
    match(ID, "an identifier");
    match(SEMI, "';'");

    int k = lookup_local(top, name);
    if (k >= 0) {                                /* { multiple declarations error } */
        snprintf(err_msg, sizeof err_msg,
                 "line %d: multiple declarations error: '%s' is already declared as %s in this scope",
                 decl_line, name, top->types[k]);
        longjmp(on_error, 1);
    }
    put(top, name, t);                           /* { top.put(id, type) } */
    indent();
    printf("declare %s %s@%d\n", t, name, top->depth);
}

/* decls -> decl decls | e */
void decls(void) {
    while (look.type == INT || look.type == BOOL) decl();
}

/* block -> '{' { saved = top; top = new Env(top); } decls stmts '}' { top = saved; } */
void block(void) {
    match(LBRACE, "'{'");
    Env *saved = top;
    top = new_env(top);
    indent(); printf("enter scope %d\n", top->depth);

    decls();
    stmts();

    indent(); printf("exit scope %d\n", top->depth);
    match(RBRACE, "'}'");
    free(top);
    top = saved;
}

char *read_all(FILE *fp) {
    size_t cap = 4096, len = 0;
    char *buf = malloc(cap);
    int c;
    while ((c = fgetc(fp)) != EOF) {
        if (len + 1 >= cap) buf = realloc(buf, cap *= 2);
        buf[len++] = (char)c;
    }
    buf[len] = '\0';
    return buf;
}

int main(int argc, char *argv[]) {
    FILE *fp = stdin;
    if (argc > 1 && !(fp = fopen(argv[1], "r"))) { perror(argv[1]); return 1; }
    src = read_all(fp);
    if (fp != stdin) fclose(fp);

    look = next_token();
    int ok;
    if (setjmp(on_error) == 0) {
        block();                                  /* program -> block */
        if (look.type != END) fail("syntax error: unexpected '%s' after the program", look.lexeme);
        printf("\nProgram is valid.\n");
        ok = 1;
    } else {
        printf("\nError: %s\n", err_msg);
        ok = 0;
    }
    free(src);
    return ok ? 0 : 1;
}
