/*
 * CS424L - Compiler Construction Lab | Lab 2 | Exercise 2
 *
 * Question:
 * 2. Write a program that takes as input a C or Java source file and finds all identifiers including
 * variables, function and class names etc., along with the line numbers where they were first
 * found. Remember to remove comments and whitespace.
 *
 * Build & run:
 *     gcc q2_identifiers.c -o q2_identifiers
 *     ./q2_identifiers ../samples/sample.c
 *     ./q2_identifiers ../samples/Sample.java
 *
 * Steps:
 *   1. remove_comments(): comments, string/char literals and #preprocessor lines are replaced by
 *      spaces (newlines kept, so line numbers stay correct).
 *   2. The remaining text is scanned; whitespace is skipped, and every name
 *      [A-Za-z_][A-Za-z0-9_]* that is not a C/Java keyword is an identifier.
 *   3. Each identifier is stored only the first time it is seen, with that line number.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_IDS 2048
#define MAX_LEN 128

/* C and Java reserved words: these are not identifiers. */
const char *keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do", "double",
    "else", "enum", "extern", "float", "for", "goto", "if", "int", "long", "register",
    "return", "short", "signed", "sizeof", "static", "struct", "switch", "typedef",
    "union", "unsigned", "void", "volatile", "while",
    "abstract", "assert", "boolean", "byte", "catch", "class", "extends", "final",
    "finally", "implements", "import", "instanceof", "interface", "native", "new",
    "package", "private", "protected", "public", "super", "synchronized", "this",
    "throw", "throws", "transient", "try", "true", "false", "null", NULL
};

static char ids[MAX_IDS][MAX_LEN];
static int  id_line[MAX_IDS];
static int  n_ids = 0;

int is_keyword(const char *w) {
    for (int k = 0; keywords[k]; k++)
        if (strcmp(w, keywords[k]) == 0) return 1;
    return 0;
}

void record(const char *name, int line) {
    if (is_keyword(name)) return;
    for (int k = 0; k < n_ids; k++)
        if (strcmp(ids[k], name) == 0) return;   /* already seen: keep the first line */
    if (n_ids < MAX_IDS) {
        strcpy(ids[n_ids], name);
        id_line[n_ids] = line;
        n_ids++;
    }
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

/* Blank out comments, literals and preprocessor lines, keeping newlines. */
void remove_comments(char *s) {
    size_t i = 0;
    int line_start = 1;
    while (s[i]) {
        if (s[i] == '\n') { line_start = 1; i++; continue; }
        if (line_start && s[i] == '#') {
            while (s[i] && s[i] != '\n') s[i++] = ' ';
            continue;
        }
        if (!isspace((unsigned char)s[i])) line_start = 0;

        if (s[i] == '/' && s[i + 1] == '/') {
            while (s[i] && s[i] != '\n') s[i++] = ' ';
        } else if (s[i] == '/' && s[i + 1] == '*') {
            s[i++] = ' '; s[i++] = ' ';
            while (s[i] && !(s[i] == '*' && s[i + 1] == '/')) { if (s[i] != '\n') s[i] = ' '; i++; }
            if (s[i]) { s[i++] = ' '; s[i++] = ' '; }
        } else if (s[i] == '"' || s[i] == '\'') {
            char quote = s[i];
            s[i++] = ' ';
            while (s[i] && s[i] != quote && s[i] != '\n') {
                if (s[i] == '\\' && s[i + 1]) s[i++] = ' ';
                s[i++] = ' ';
            }
            if (s[i] == quote) s[i++] = ' ';
        } else {
            i++;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
        return 1;
    }
    char *s = read_file(argv[1]);
    if (!s) { perror(argv[1]); return 1; }
    remove_comments(s);

    int line = 1;
    size_t i = 0;
    while (s[i]) {
        char c = s[i];
        if (c == '\n') { line++; i++; }
        else if (isspace((unsigned char)c)) { i++; }                      /* remove whitespace */
        else if (isdigit((unsigned char)c)) {                             /* numbers like 0x1F, 1.5f */
            while (isalnum((unsigned char)s[i]) || s[i] == '_' || s[i] == '.') i++;
        } else if (isalpha((unsigned char)c) || c == '_') {
            char word[MAX_LEN];
            int len = 0;
            while (isalnum((unsigned char)s[i]) || s[i] == '_') {
                if (len < MAX_LEN - 1) word[len++] = s[i];
                i++;
            }
            word[len] = '\0';
            record(word, line);
        } else {
            i++;                                                          /* operators, punctuation */
        }
    }

    printf("%-25s%s\n", "Identifier", "First found on line");
    printf("---------------------------------------------\n");
    for (int k = 0; k < n_ids; k++)
        printf("%-25s%d\n", ids[k], id_line[k]);
    printf("\nTotal identifiers: %d\n", n_ids);
    free(s);
    return 0;
}
