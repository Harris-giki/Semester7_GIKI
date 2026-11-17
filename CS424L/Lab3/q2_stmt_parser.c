/*
 * CS424L - Compiler Construction Lab | Lab 3 | Exercise 2
 *
 * Question:
 * 2. Combine the expression parser from the previous exercise with the example. Replace the
 * expr terminal with the grammar for expr.
 *
 * ---------------------------------------------------------------------------
 * The lab example is the predictive parser for statements (Dragon Book, Sec. 2.4.2), in which
 * expr is treated as a single terminal:
 *     stmt    -> expr ;
 *              | if ( expr ) stmt
 *              | for ( optexpr ; optexpr ; optexpr ) stmt
 *              | other
 *     optexpr -> expr | e
 *
 * Here expr is no longer a terminal. It is replaced by the (left-recursion free) expression
 * grammar from Exercise 1, and the assignment statement from Exercise 1 is kept as well:
 *     program -> stmt program | e
 *     stmt    -> id = expr ;            (chosen when the next two tokens are id and '=')
 *              | expr ;
 *              | if ( expr ) stmt
 *              | for ( optexpr ; optexpr ; optexpr ) stmt
 *              | other
 *     optexpr -> expr | e
 *     expr    -> term expr'
 *     expr'   -> + term expr' | e
 *     term    -> factor term'
 *     term'   -> * factor term' | e
 *     factor  -> id | num | ( expr )
 *
 * "if", "for" and "other" are keywords. The parser prints each production it applies,
 * indented by depth, and prints "syntax error" if the input is invalid.
 *
 * Build & run:
 *     gcc q2_stmt_parser.c -o q2_stmt_parser
 *     ./q2_stmt_parser tests/q2_valid.txt
 *     ./q2_stmt_parser tests/q2_invalid.txt
 * ---------------------------------------------------------------------------
 */

#include <ctype.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ----------------------------- Lexical analyzer ----------------------------- */

typedef enum { ID, NUM, IF, FOR, OTHER, ASSIGN, PLUS, MUL, LPAREN, RPAREN, SEMI, END, INVALID } TokenType;

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
            case ';': t.type = SEMI;   break;
            default:  t.type = INVALID;
        }
    }
    size_t n = pos - start;
    if (n > 63) n = 63;
    memcpy(t.lexeme, src + start, n);
    t.lexeme[n] = '\0';

    if (t.type == ID) {                                    /* keywords */
        if (strcmp(t.lexeme, "if") == 0)    t.type = IF;
        if (strcmp(t.lexeme, "for") == 0)   t.type = FOR;
        if (strcmp(t.lexeme, "other") == 0) t.type = OTHER;
    }
    return t;
}

/* ---------------------------- Predictive parser ----------------------------- */

static Token   look;      /* current lookahead */
static Token   peek;      /* the token after look: tells "id = expr ;" apart from "expr ;" */
static int     depth = 0;
static jmp_buf on_error;
static char    err_msg[256];

void advance(void) { look = peek; peek = next_token(); }

/* print a production, indented by the current depth */
void trace(const char *rule, const char *extra) {
    printf("%*s%s", depth * 2, "", rule);
    if (extra) printf("   [%s]", extra);
    printf("\n");
}

void syntax_error(const char *expected) {
    snprintf(err_msg, sizeof err_msg, "line %d: expected %s but found '%s'",
             look.line, expected, look.lexeme);
    longjmp(on_error, 1);
}

void match(TokenType t, const char *what) {
    if (look.type != t) syntax_error(what);
    advance();
}

int starts_expr(void) { return look.type == ID || look.type == NUM || look.type == LPAREN; }

void expr(void);

/* factor -> id | num | ( expr ) */
void factor(void) {
    if (look.type == ID)       { trace("factor -> id", look.lexeme);  advance(); }
    else if (look.type == NUM) { trace("factor -> num", look.lexeme); advance(); }
    else if (look.type == LPAREN) {
        trace("factor -> ( expr )", NULL);
        depth++;
        match(LPAREN, "'('"); expr(); match(RPAREN, "')'");
        depth--;
    } else {
        syntax_error("id, num or '('");
    }
}

/* term' -> * factor term' | e */
void term_prime(void) {
    if (look.type == MUL) {
        trace("term' -> * factor term'", NULL);
        depth++;
        match(MUL, "'*'"); factor(); term_prime();
        depth--;
    } else {
        trace("term' -> e", NULL);
    }
}

/* term -> factor term' */
void term(void) {
    trace("term -> factor term'", NULL);
    depth++; factor(); term_prime(); depth--;
}

/* expr' -> + term expr' | e */
void expr_prime(void) {
    if (look.type == PLUS) {
        trace("expr' -> + term expr'", NULL);
        depth++;
        match(PLUS, "'+'"); term(); expr_prime();
        depth--;
    } else {
        trace("expr' -> e", NULL);
    }
}

/* expr -> term expr' */
void expr(void) {
    trace("expr -> term expr'", NULL);
    depth++; term(); expr_prime(); depth--;
}

/* optexpr -> expr | e */
void optexpr(void) {
    if (starts_expr()) {
        trace("optexpr -> expr", NULL);
        depth++; expr(); depth--;
    } else {
        trace("optexpr -> e", NULL);
    }
}

void stmt(void) {
    switch (look.type) {
        case IF:
            trace("stmt -> if ( expr ) stmt", NULL);
            depth++;
            match(IF, "'if'"); match(LPAREN, "'('"); expr(); match(RPAREN, "')'"); stmt();
            depth--;
            break;
        case FOR:
            trace("stmt -> for ( optexpr ; optexpr ; optexpr ) stmt", NULL);
            depth++;
            match(FOR, "'for'"); match(LPAREN, "'('");
            optexpr(); match(SEMI, "';'");
            optexpr(); match(SEMI, "';'");
            optexpr(); match(RPAREN, "')'");
            stmt();
            depth--;
            break;
        case OTHER:
            trace("stmt -> other", NULL);
            match(OTHER, "'other'");
            break;
        default:
            if (look.type == ID && peek.type == ASSIGN) {
                trace("stmt -> id = expr ;", look.lexeme);
                depth++;
                match(ID, "an identifier"); match(ASSIGN, "'='"); expr(); match(SEMI, "';'");
                depth--;
            } else if (starts_expr()) {
                trace("stmt -> expr ;", NULL);
                depth++;
                expr(); match(SEMI, "';'");
                depth--;
            } else {
                syntax_error("a statement");
            }
    }
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
    peek = next_token();

    int ok;
    if (setjmp(on_error) == 0) {
        while (look.type != END) stmt();          /* program -> stmt program | e */
        printf("\nInput is syntactically valid.\n");
        ok = 1;
    } else {
        printf("\nsyntax error (%s)\n", err_msg);
        ok = 0;
    }
    free(src);
    return ok ? 0 : 1;
}
