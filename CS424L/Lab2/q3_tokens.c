/*
 * CS424L - Compiler Construction Lab | Lab 2 | Exercise 3
 *
 * Question:
 * 3. Write a program that takes as input a C source file and generates a stream of tokens. For
 * example if (a<b) should be converted into <IF><LPAREN><ID><LT><ID><RPAREN>. Handle
 * as many tokens as possible and ignore whitespace and comments that begin with //.
 *
 * Build & run:
 *     gcc q3_tokens.c -o q3_tokens
 *     ./q3_tokens ../samples/sample.c
 *
 * This version is organised the way a real compiler's lexer is: a function get_token()
 * returns one token (type + lexeme + line) per call, and main() keeps calling it until
 * end of file. Integers and reals get different tokens (<NUM> and <REAL>).
 * Block comments and #preprocessor lines are skipped as well as // comments.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char name[16];      /* token name, e.g. "IF", "ID", "LT" */
    char lexeme[128];   /* the matched text */
    int  line;
} Token;

const char *keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do", "double",
    "else", "enum", "extern", "float", "for", "goto", "if", "int", "long", "register",
    "return", "short", "signed", "sizeof", "static", "struct", "switch", "typedef",
    "union", "unsigned", "void", "volatile", "while", NULL
};

/* Longest operators first so that "<=" wins over "<". */
struct { const char *text; const char *name; } ops[] = {
    {"++", "INC"}, {"--", "DEC"}, {"+=", "PLUSEQ"}, {"-=", "MINUSEQ"}, {"*=", "MULEQ"},
    {"/=", "DIVEQ"}, {"->", "ARROW"}, {"<=", "LE"}, {">=", "GE"}, {"==", "EQ"}, {"!=", "NE"},
    {"&&", "AND"}, {"||", "OR"}, {"<<", "SHL"}, {">>", "SHR"},
    {"<", "LT"}, {">", "GT"}, {"=", "ASSIGN"}, {"!", "NOT"}, {"+", "PLUS"}, {"-", "MINUS"},
    {"*", "MUL"}, {"/", "DIV"}, {"%", "MOD"}, {"&", "BITAND"}, {"|", "BITOR"}, {"^", "BITXOR"},
    {"~", "BITNOT"}, {"(", "LPAREN"}, {")", "RPAREN"}, {"{", "LBRACE"}, {"}", "RBRACE"},
    {"[", "LBRACKET"}, {"]", "RBRACKET"}, {";", "SEMI"}, {",", "COMMA"}, {".", "DOT"},
    {":", "COLON"}, {"?", "QUESTION"}, {NULL, NULL}
};

/* ---- lexer state ---- */
static char  *src;
static size_t pos = 0;
static int    line = 1;
static int    at_line_start = 1;

static void make(Token *t, const char *name, size_t start, size_t end) {
    strcpy(t->name, name);
    size_t n = end - start;
    if (n > sizeof t->lexeme - 1) n = sizeof t->lexeme - 1;
    memcpy(t->lexeme, src + start, n);
    t->lexeme[n] = '\0';
    t->line = line;
}

/* Return the next token; name is "EOF" at the end of the input. */
Token get_token(void) {
    Token t;
    for (;;) {                                   /* skip whitespace, comments, # lines */
        char c = src[pos];
        if (c == '\n') { line++; pos++; at_line_start = 1; }
        else if (isspace((unsigned char)c)) pos++;
        else if (c == '#' && at_line_start) { while (src[pos] && src[pos] != '\n') pos++; }
        else if (c == '/' && src[pos + 1] == '/') { while (src[pos] && src[pos] != '\n') pos++; }
        else if (c == '/' && src[pos + 1] == '*') {
            pos += 2;
            while (src[pos] && !(src[pos] == '*' && src[pos + 1] == '/')) { if (src[pos] == '\n') line++; pos++; }
            if (src[pos]) pos += 2;
        } else break;
    }
    at_line_start = 0;

    size_t start = pos;
    char c = src[pos];

    if (c == '\0') { make(&t, "EOF", pos, pos); return t; }

    if (isalpha((unsigned char)c) || c == '_') {               /* identifier / keyword */
        while (isalnum((unsigned char)src[pos]) || src[pos] == '_') pos++;
        make(&t, "ID", start, pos);
        for (int k = 0; keywords[k]; k++) {
            if (strcmp(t.lexeme, keywords[k]) == 0) {
                for (int m = 0; t.lexeme[m]; m++) t.name[m] = (char)toupper((unsigned char)t.lexeme[m]);
                t.name[strlen(t.lexeme)] = '\0';
                break;
            }
        }
        return t;
    }

    if (isdigit((unsigned char)c)) {                          /* NUM or REAL */
        int real = 0;
        while (isdigit((unsigned char)src[pos])) pos++;
        if (src[pos] == '.' && isdigit((unsigned char)src[pos + 1])) {
            real = 1;
            pos++;
            while (isdigit((unsigned char)src[pos])) pos++;
        }
        if ((src[pos] == 'e' || src[pos] == 'E') && isdigit((unsigned char)src[pos + 1])) {
            real = 1;
            pos++;
            while (isdigit((unsigned char)src[pos])) pos++;
        }
        make(&t, real ? "REAL" : "NUM", start, pos);
        return t;
    }

    if (c == '"' || c == '\'') {                               /* string / char literal */
        char quote = c;
        pos++;
        while (src[pos] && src[pos] != quote && src[pos] != '\n') { if (src[pos] == '\\' && src[pos + 1]) pos++; pos++; }
        if (src[pos] == quote) pos++;
        make(&t, quote == '"' ? "STRING" : "CHAR_LIT", start, pos);
        return t;
    }

    for (int k = 0; ops[k].text; k++) {                        /* operators / punctuation */
        size_t n = strlen(ops[k].text);
        if (strncmp(src + pos, ops[k].text, n) == 0) {
            pos += n;
            make(&t, ops[k].name, start, pos);
            return t;
        }
    }

    pos++;
    make(&t, "UNKNOWN", start, pos);
    return t;
}

char *read_file(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);
    char *buf = malloc(size + 1);
    size_t got = fread(buf, 1, size, fp);
    buf[got] = '\0';
    fclose(fp);
    return buf;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <c_source_file>\n", argv[0]);
        return 1;
    }
    src = read_file(argv[1]);
    if (!src) { perror(argv[1]); return 1; }

    int printed_line = 0, count = 0;
    Token t;
    while (strcmp((t = get_token()).name, "EOF") != 0) {
        if (t.line != printed_line) {
            if (printed_line) printf("\n");
            printf("%4d: ", t.line);
            printed_line = t.line;
        }
        printf("<%s>", t.name);
        count++;
    }
    printf("\n\nTotal tokens: %d\n", count);
    free(src);
    return 0;
}
