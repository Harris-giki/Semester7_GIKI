/*
 * CS424L - Compiler Construction Lab | Lab 2 | Exercise 1
 *
 * Question:
 * 1. Write a program that finds all numerical constants used in a given source file. Examples are
 * 123, 123.10, -1234.
 *
 * Build & run:
 *     gcc q1_numbers.c -o q1_numbers
 *     ./q1_numbers ../samples/sample.c
 *
 * The scanner works in two passes:
 *   1. remove_comments() replaces every comment and string/char literal with spaces
 *      (newlines are kept, so line numbers stay correct).
 *   2. A small state machine (a DFA) recognises numbers in what is left:
 *          number -> digits ( . digits )? ( (e|E) (+|-)? digits )?
 *      A '-' belongs to the number only if the token before it is not an operand
 *      (identifier, number, ')' or ']'); in "count-5" the '-' is subtraction and 5 is reported.
 * Each constant is classified as an integer or a real constant.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* Pass 1: blank out comments and literals in place, keeping newlines. */
void remove_comments(char *s) {
    size_t i = 0;
    while (s[i]) {
        if (s[i] == '/' && s[i + 1] == '/') {
            while (s[i] && s[i] != '\n') s[i++] = ' ';
        } else if (s[i] == '/' && s[i + 1] == '*') {
            s[i++] = ' '; s[i++] = ' ';
            while (s[i] && !(s[i] == '*' && s[i + 1] == '/')) { if (s[i] != '\n') s[i] = ' '; i++; }
            if (s[i]) { s[i++] = ' '; s[i++] = ' '; }
        } else if (s[i] == '"' || s[i] == '\'') {
            /* literal becomes the placeholder character '_' so it still counts as an operand */
            char quote = s[i];
            s[i++] = '_';
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

    int line = 1, n_int = 0, n_real = 0, after_operand = 0;
    size_t i = 0;

    printf("%-8s%-14s%s\n", "Line", "Constant", "Type");
    printf("--------------------------------\n");

    while (s[i]) {
        char c = s[i];
        if (c == '\n') { line++; i++; continue; }
        if (isspace((unsigned char)c)) { i++; continue; }

        if (isalpha((unsigned char)c) || c == '_') {          /* identifier: skip */
            while (isalnum((unsigned char)s[i]) || s[i] == '_') i++;
            after_operand = 1;
            continue;
        }

        size_t start = i;
        int negative = 0;
        if (c == '-' && !after_operand) {
            size_t j = i + 1;
            while (s[j] == ' ' || s[j] == '\t') j++;
            if (isdigit((unsigned char)s[j])) { negative = 1; start = j; }
        }

        if (isdigit((unsigned char)c) || negative) {
            /* DFA: state 1 = integer part, 2 = fraction, 3 = exponent */
            size_t j = start;
            int is_real = 0;
            while (isdigit((unsigned char)s[j])) j++;                          /* state 1 */
            if (s[j] == '.' && isdigit((unsigned char)s[j + 1])) {             /* state 2 */
                is_real = 1;
                j++;
                while (isdigit((unsigned char)s[j])) j++;
            }
            if ((s[j] == 'e' || s[j] == 'E') &&                                 /* state 3 */
                (isdigit((unsigned char)s[j + 1]) ||
                 ((s[j + 1] == '+' || s[j + 1] == '-') && isdigit((unsigned char)s[j + 2])))) {
                is_real = 1;
                j += 2;
                while (isdigit((unsigned char)s[j])) j++;
            }
            char text[64];
            snprintf(text, sizeof text, "%s%.*s", negative ? "-" : "", (int)(j - start), s + start);
            printf("%-8d%-14s%s\n", line, text, is_real ? "real" : "integer");
            if (is_real) n_real++; else n_int++;
            i = j;
            after_operand = 1;
            continue;
        }

        after_operand = (c == ')' || c == ']');
        i++;
    }

    printf("\nInteger constants: %d\nReal constants   : %d\nTotal            : %d\n",
           n_int, n_real, n_int + n_real);
    free(s);
    return 0;
}
