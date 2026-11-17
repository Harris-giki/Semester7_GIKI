/*
 * CS424L - Compiler Construction Lab | Lab 1 | Exercise 4
 *
 * Question:
 * 4. Write a program that takes as input a C source file and generates a stream of tokens. For
 * example if (a<b) should be converted into <IF><LPAREN><ID><LT><ID><RPAREN>. Handle
 * as many tokens as possible and ignore whitespace and comments that begin with //.
 *
 * Build & run:
 *     gcc q4_tokenizer.c -o q4_tokenizer
 *     ./q4_tokenizer ../samples/sample.c
 *
 * Output: the token stream of every source line, prefixed with the line number.
 * Besides // comments, block comments and #preprocessor lines are skipped too.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* C keywords: each one becomes its own token, e.g. if -> <IF> */
const char *keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do", "double",
    "else", "enum", "extern", "float", "for", "goto", "if", "int", "long", "register",
    "return", "short", "signed", "sizeof", "static", "struct", "switch", "typedef",
    "union", "unsigned", "void", "volatile", "while", NULL
};

/* Operators and punctuation. Longer ones come first so "<=" is matched before "<". */
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

int printed_line = 0;

/* Print one token, starting a new output line when the source line changes. */
void emit(const char *name, int line) {
    if (line != printed_line) {
        if (printed_line) printf("\n");
        printf("%4d: ", line);
        printed_line = line;
    }
    printf("<%s>", name);
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
    char *s = read_file(argv[1]);
    if (!s) { perror(argv[1]); return 1; }

    int line = 1;
    size_t i = 0;
    int at_line_start = 1;

    while (s[i]) {
        char c = s[i];

        if (c == '\n') { line++; i++; at_line_start = 1; continue; }
        if (isspace((unsigned char)c)) { i++; continue; }

        /* #include, #define ... */
        if (c == '#' && at_line_start) { while (s[i] && s[i] != '\n') i++; continue; }
        at_line_start = 0;

        /* // comment */
        if (c == '/' && s[i + 1] == '/') { while (s[i] && s[i] != '\n') i++; continue; }

        /* block comment */
        if (c == '/' && s[i + 1] == '*') {
            i += 2;
            while (s[i] && !(s[i] == '*' && s[i + 1] == '/')) { if (s[i] == '\n') line++; i++; }
            if (s[i]) i += 2;
            continue;
        }

        /* identifier or keyword */
        if (isalpha((unsigned char)c) || c == '_') {
            char word[128];
            int len = 0;
            while (isalnum((unsigned char)s[i]) || s[i] == '_') {
                if (len < 127) word[len++] = s[i];
                i++;
            }
            word[len] = '\0';
            const char *name = "ID";
            char upper[128];
            for (int k = 0; keywords[k]; k++) {
                if (strcmp(word, keywords[k]) == 0) {
                    for (int m = 0; m <= len; m++) upper[m] = (char)toupper((unsigned char)word[m]);
                    name = upper;
                    break;
                }
            }
            emit(name, line);
            continue;
        }

        /* number: 123, 123.10, 2.5e3 */
        if (isdigit((unsigned char)c)) {
            while (isdigit((unsigned char)s[i])) i++;
            if (s[i] == '.' && isdigit((unsigned char)s[i + 1])) {
                i++;
                while (isdigit((unsigned char)s[i])) i++;
            }
            if ((s[i] == 'e' || s[i] == 'E') && isdigit((unsigned char)s[i + 1])) {
                i++;
                while (isdigit((unsigned char)s[i])) i++;
            }
            emit("NUM", line);
            continue;
        }

        /* string and character literals */
        if (c == '"' || c == '\'') {
            char quote = c;
            i++;
            while (s[i] && s[i] != quote && s[i] != '\n') { if (s[i] == '\\' && s[i + 1]) i++; i++; }
            if (s[i] == quote) i++;
            emit(quote == '"' ? "STRING" : "CHAR_LIT", line);
            continue;
        }

        /* operators and punctuation (longest match first) */
        int matched = 0;
        for (int k = 0; ops[k].text; k++) {
            size_t n = strlen(ops[k].text);
            if (strncmp(s + i, ops[k].text, n) == 0) {
                emit(ops[k].name, line);
                i += n;
                matched = 1;
                break;
            }
        }
        if (!matched) { emit("UNKNOWN", line); i++; }
    }
    printf("\n");
    free(s);
    return 0;
}
