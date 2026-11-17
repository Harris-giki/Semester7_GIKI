/*
 * CS424L - Compiler Construction Lab | Lab 3 | Exercise 1
 *
 * Question:
 * 1. Write a parser for statements of the form id= expr;. Examples of valid input are a=a+b*c;,
 * a=3*b; The grammar for this is the following.
 *     stmt   -> id = expr ;
 *     expr   -> expr + term
 *             | term
 *     term   -> term * factor
 *             | factor
 *     factor -> id
 *             | num
 *             | (expr)
 * Here id and num are the tokens for identifiers and numerals respectively. You need to extend
 * the lexical analyzer to return these tokens. As it stands the grammar is left-recursive, so you
 * need to rewrite the grammar to remove left recursion. As before the parser should output
 * "syntax error" if the input is invalid.
 *
 * ---------------------------------------------------------------------------
 * Grammar after removing left recursion (A -> A a | b  becomes  A -> b A', A' -> a A' | e):
 *     stmt   -> id = expr ;
 *     expr   -> term expr'
 *     expr'  -> + term expr' | e
 *     term   -> factor term'
 *     term'  -> * factor term' | e
 *     factor -> id | num | ( expr )
 *
 * Each non-terminal is one function of a recursive-descent (predictive) parser.
 * Every statement in the input is checked separately. A valid statement is printed in
 * postfix form, which shows that * binds tighter than + and both are left-associative.
 *
 * Build & run:
 *     gcc q1_assignment_parser.c -o q1_assignment_parser
 *     ./q1_assignment_parser tests/q1_input.txt        (or type statements on stdin)
 * ---------------------------------------------------------------------------
 */

#include <ctype.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ----------------------------- Lexical analyzer ----------------------------- */

typedef enum { ID, NUM, ASSIGN, PLUS, MUL, LPAREN, RPAREN, SEMI, END, INVALID } TokenType;

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
    /* skip whitespace and // comments */
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

    if (isalpha((unsigned char)c) || c == '_') {                           /* id */
        while (isalnum((unsigned char)src[pos]) || src[pos] == '_') pos++;
        t.type = ID;
    } else if (isdigit((unsigned char)c)) {                                /* num */
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
    return t;
}

/* ---------------------------- Predictive parser ----------------------------- */

static Token   look;               /* one-token lookahead */
static char    postfix[1024];      /* translation of the current statement */
static char    err_msg[256];
static jmp_buf on_error;           /* jump here on a syntax error */

void syntax_error(const char *expected) {
    snprintf(err_msg, sizeof err_msg, "line %d: expected %s but found '%s'",
             look.line, expected, look.lexeme);
    longjmp(on_error, 1);
}

void match(TokenType t, const char *what) {
    if (look.type != t) syntax_error(what);
    look = next_token();
}

void emit(const char *s) {
    strncat(postfix, s, sizeof postfix - strlen(postfix) - 2);
    strcat(postfix, " ");
}

void expr(void);

/* factor -> id | num | ( expr ) */
void factor(void) {
    if (look.type == ID || look.type == NUM) {
        emit(look.lexeme);
        look = next_token();
    } else if (look.type == LPAREN) {
        match(LPAREN, "'('");
        expr();
        match(RPAREN, "')'");
    } else {
        syntax_error("id, num or '('");
    }
}

/* term' -> * factor term' | e */
void term_prime(void) {
    if (look.type == MUL) {
        match(MUL, "'*'");
        factor();
        emit("*");
        term_prime();
    }
    /* else epsilon */
}

/* term -> factor term' */
void term(void) { factor(); term_prime(); }

/* expr' -> + term expr' | e */
void expr_prime(void) {
    if (look.type == PLUS) {
        match(PLUS, "'+'");
        term();
        emit("+");
        expr_prime();
    }
}

/* expr -> term expr' */
void expr(void) { term(); expr_prime(); }

/* stmt -> id = expr ; */
void stmt(void) {
    char target[64];
    strcpy(target, look.lexeme);
    match(ID, "an identifier");
    match(ASSIGN, "'='");
    expr();
    match(SEMI, "';'");
    printf("valid    postfix: %s %s=\n", target, postfix);
}

/* After an error skip past the next ';' so the remaining statements are still checked. */
void recover(void) {
    while (look.type != SEMI && look.type != END) look = next_token();
    if (look.type == SEMI) look = next_token();
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
    int n = 0, errors = 0;
    while (look.type != END) {
        n++;
        postfix[0] = '\0';
        printf("stmt %d: ", n);
        if (setjmp(on_error) == 0) {
            stmt();
        } else {
            printf("syntax error (%s)\n", err_msg);
            errors++;
            recover();
        }
    }
    free(src);
    return errors ? 1 : 0;
}
