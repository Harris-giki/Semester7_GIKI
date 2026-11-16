/*
 * CS424L - Compiler Construction Lab | Lab 2 | Exercise 2
 *
 * Question:
 * 2. Write a program that takes as input a C or Java source file and finds all identifiers including
 * variables, function and class names etc., along with the line numbers where they were first
 * found. Remember to remove comments and whitespace.
 *
 * Build & run:
 *     flex -o q2_identifiers.c q2_identifiers.l && cc q2_identifiers.c -o q2_identifiers
 *     ./q2_identifiers <source_file>
 */

%option noyywrap yylineno nounput noinput

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_IDS 1024

/* Identifiers in the order they were first found, with that line number. */
char *names[MAX_IDS];
int   lines[MAX_IDS];
int   n_ids = 0;

/* Keywords of C and Java: these are reserved words, not identifiers. */
const char *keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do", "double",
    "else", "enum", "extern", "float", "for", "goto", "if", "int", "long", "register",
    "return", "short", "signed", "sizeof", "static", "struct", "switch", "typedef",
    "union", "unsigned", "void", "volatile", "while",
    "abstract", "assert", "boolean", "byte", "catch", "class", "extends", "final",
    "finally", "implements", "import", "instanceof", "interface", "native", "new",
    "package", "private", "protected", "public", "super", "synchronized", "this",
    "throw", "throws", "transient", "try", "true", "false", "null",
    NULL
};

int is_keyword(const char *s) {
    for (int i = 0; keywords[i]; i++)
        if (strcmp(keywords[i], s) == 0) return 1;
    return 0;
}

void record(const char *name, int line) {
    if (is_keyword(name)) return;
    for (int i = 0; i < n_ids; i++)
        if (strcmp(names[i], name) == 0) return;   /* already seen: keep first line */
    if (n_ids < MAX_IDS) {
        names[n_ids] = strdup(name);
        lines[n_ids] = line;
        n_ids++;
    }
}
%}

%x BLOCK_COMMENT

%%

"/*"                    { BEGIN(BLOCK_COMMENT); }
<BLOCK_COMMENT>"*/"     { BEGIN(INITIAL); }
<BLOCK_COMMENT>.|\n     { /* remove comment */ }

"//".*                  { /* remove line comment */ }
^[ \t]*#.*              { /* skip preprocessor lines such as #include <stdio.h> */ }
\"(\\.|[^"\\\n])*\"     { /* text inside strings is not an identifier */ }
\'(\\.|[^'\\\n])*\'     { /* char literal */ }

[0-9][A-Za-z0-9_.]*     { /* numbers (including 0x1F, 1.5f) */ }
[A-Za-z_][A-Za-z0-9_]*  { record(yytext, yylineno); }

[ \t\r\n]+              { /* remove whitespace */ }
.                       { /* operators and punctuation */ }

%%

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
        return 1;
    }
    yyin = fopen(argv[1], "r");
    if (!yyin) { perror(argv[1]); return 1; }

    yylex();

    printf("%-25s%s\n", "Identifier", "First found on line");
    printf("---------------------------------------------\n");
    for (int i = 0; i < n_ids; i++) {
        printf("%-25s%d\n", names[i], lines[i]);
        free(names[i]);
    }
    printf("\nTotal identifiers: %d\n", n_ids);
    fclose(yyin);
    return 0;
}
