/*
 * CS424L - Compiler Construction Lab | Lab 5 | Exercise 1
 *
 * Question:
 * The example above only has if statements and assignments of the form id=id or id=num. Add
 * support for while and do-while loops to the example. To do this study the if_stmt class in the
 * example code. Your program should be able to translate the following code.
 *     {
 *     a =1;
 *     if ( a ) {
 *     b = a ;
 *     while ( b ) {
 *     a =2;
 *     b = a ;
 *     }
 *     do{
 *     c = b ;
 *     } while ( b ) ;
 *     }
 *     d = a ;
 *     }
 *
 * ---------------------------------------------------------------------------
 * Grammar:
 *     program -> block
 *     block   -> { stmts }
 *     stmts   -> stmt stmts | e
 *     stmt    -> id = id ;                       (ASSIGN_STMT)
 *              | id = num ;                      (ASSIGN_STMT)
 *              | if ( id ) stmt                  (IF_STMT, from the example)
 *              | while ( id ) stmt               (WHILE_STMT, new)
 *              | do stmt while ( id ) ;          (DO_WHILE_STMT, new)
 *              | block                           (BLOCK_STMT)
 *
 * C has no classes, so each "class" of the example is a kind of syntax-tree node, and its
 * gen() method is a gen_...() function. gen_stmt() dispatches on the kind, like a virtual call.
 * The parser builds the tree; gen_stmt() then emits three-address code:
 *
 *   if_stmt (if (E) S)       while_stmt (while (E) S)     do_while_stmt (do S while (E);)
 *       ifFalse E goto after   begin:                       begin:
 *       S                          ifFalse E goto after         S
 *   after:                         S                            if E goto begin
 *                                  goto begin
 *                              after:
 *
 * while_stmt is if_stmt plus a jump back to a label placed before the test. do_while_stmt
 * runs the body first, and its test jumps back to the top while the condition is true.
 *
 * Build & run:
 *     gcc translator.c -o translator
 *     ./translator tests/lab_example.txt
 * ---------------------------------------------------------------------------
 */

#include <ctype.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ----------------------------- Lexical analyzer ----------------------------- */

typedef enum { ID, NUM, IF, WHILE, DO, ASSIGN, LPAREN, RPAREN, LBRACE, RBRACE, SEMI, END, INVALID } TokenType;

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
        if (strcmp(t.lexeme, "if") == 0)    t.type = IF;
        if (strcmp(t.lexeme, "while") == 0) t.type = WHILE;
        if (strcmp(t.lexeme, "do") == 0)    t.type = DO;
    }
    return t;
}

/* ------------------------------- Syntax tree -------------------------------- */

typedef enum { ASSIGN_STMT, BLOCK_STMT, IF_STMT, WHILE_STMT, DO_WHILE_STMT } StmtKind;

typedef struct Stmt {
    StmtKind kind;
    char target[64];       /* ASSIGN_STMT: left-hand side */
    char value[64];        /* ASSIGN_STMT: right-hand side (id or num) */
    char cond[64];         /* IF / WHILE / DO_WHILE: the condition id */
    struct Stmt *body;     /* IF / WHILE / DO_WHILE: the inner statement; BLOCK: first statement */
    struct Stmt *next;     /* next statement in the same block */
} Stmt;

Stmt *new_stmt(StmtKind kind) {
    Stmt *s = calloc(1, sizeof(Stmt));
    s->kind = kind;
    return s;
}

void free_stmt(Stmt *s) {
    while (s) {
        Stmt *next = s->next;
        free_stmt(s->body);
        free(s);
        s = next;
    }
}

/* ----------------------------- Code generation ------------------------------ */

static int label_count = 0;

int  new_label(void)            { return ++label_count; }
void emit(const char *code)     { printf("        %s\n", code); }
void emit_label(int label)      { printf("L%d:\n", label); }

void gen_stmt(Stmt *s);

void gen_assign(Stmt *s) {
    char code[160];
    snprintf(code, sizeof code, "%s = %s", s->target, s->value);
    emit(code);
}

void gen_block(Stmt *s) {
    for (Stmt *p = s->body; p; p = p->next) gen_stmt(p);
}

/* if ( id ) stmt                -- the if_stmt of the example */
void gen_if(Stmt *s) {
    char code[160];
    int after = new_label();
    snprintf(code, sizeof code, "ifFalse %s goto L%d", s->cond, after);
    emit(code);
    gen_stmt(s->body);
    emit_label(after);
}

/* while ( id ) stmt             -- new */
void gen_while(Stmt *s) {
    char code[160];
    int begin = new_label();
    int after = new_label();
    emit_label(begin);
    snprintf(code, sizeof code, "ifFalse %s goto L%d", s->cond, after);   /* leave when false */
    emit(code);
    gen_stmt(s->body);
    snprintf(code, sizeof code, "goto L%d", begin);                       /* test again */
    emit(code);
    emit_label(after);
}

/* do stmt while ( id ) ;        -- new */
void gen_do_while(Stmt *s) {
    char code[160];
    int begin = new_label();
    emit_label(begin);
    gen_stmt(s->body);                                                    /* runs at least once */
    snprintf(code, sizeof code, "if %s goto L%d", s->cond, begin);        /* repeat while true */
    emit(code);
}

void gen_stmt(Stmt *s) {
    switch (s->kind) {
        case ASSIGN_STMT:   gen_assign(s);   break;
        case BLOCK_STMT:    gen_block(s);    break;
        case IF_STMT:       gen_if(s);       break;
        case WHILE_STMT:    gen_while(s);    break;
        case DO_WHILE_STMT: gen_do_while(s); break;
    }
}

/* --------------------------------- Parser ----------------------------------- */

static Token   look;
static jmp_buf on_error;
static char    err_msg[256];

void syntax_error(const char *expected) {
    snprintf(err_msg, sizeof err_msg, "line %d: expected %s but found '%s'",
             look.line, expected, look.lexeme);
    longjmp(on_error, 1);
}

/* match a token; if out is given, copy its lexeme there */
void match(TokenType t, const char *what, char *out) {
    if (look.type != t) syntax_error(what);
    if (out) strcpy(out, look.lexeme);
    look = next_token();
}

Stmt *statement(void);

/* block -> { stmts } */
Stmt *block(void) {
    Stmt *b = new_stmt(BLOCK_STMT);
    Stmt **tail = &b->body;
    match(LBRACE, "'{'", NULL);
    while (look.type != RBRACE && look.type != END) {
        *tail = statement();
        tail = &(*tail)->next;
    }
    match(RBRACE, "'}'", NULL);
    return b;
}

/* ( id ) */
void condition(char *out) {
    match(LPAREN, "'('", NULL);
    match(ID, "an identifier", out);
    match(RPAREN, "')'", NULL);
}

Stmt *statement(void) {
    Stmt *s;
    switch (look.type) {
        case ID:                                            /* id = id ; | id = num ; */
            s = new_stmt(ASSIGN_STMT);
            match(ID, "an identifier", s->target);
            match(ASSIGN, "'='", NULL);
            if (look.type == ID || look.type == NUM) {
                strcpy(s->value, look.lexeme);
                look = next_token();
            } else {
                syntax_error("an identifier or number");
            }
            match(SEMI, "';'", NULL);
            return s;
        case IF:                                            /* if ( id ) stmt */
            s = new_stmt(IF_STMT);
            match(IF, "'if'", NULL);
            condition(s->cond);
            s->body = statement();
            return s;
        case WHILE:                                         /* while ( id ) stmt */
            s = new_stmt(WHILE_STMT);
            match(WHILE, "'while'", NULL);
            condition(s->cond);
            s->body = statement();
            return s;
        case DO:                                            /* do stmt while ( id ) ; */
            s = new_stmt(DO_WHILE_STMT);
            match(DO, "'do'", NULL);
            s->body = statement();
            match(WHILE, "'while'", NULL);
            condition(s->cond);
            match(SEMI, "';'", NULL);
            return s;
        case LBRACE:
            return block();
        default:
            syntax_error("a statement");
            return NULL;
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
    if (setjmp(on_error) != 0) {
        printf("syntax error (%s)\n", err_msg);
        free(src);
        return 1;
    }

    Stmt *program = block();                              /* program -> block */
    if (look.type != END) syntax_error("end of input");

    printf("Three-address code:\n\n");
    gen_stmt(program);

    free_stmt(program);
    free(src);
    return 0;
}
