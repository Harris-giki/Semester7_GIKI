/*
 * CS424L - Compiler Construction Lab | Lab 1 | Exercise 2
 *
 * Question:
 * 2. Write a program that finds all numerical constants used in a given source file. Examples are
 * 123, 123.10, -1234.
 *
 * Build & run:
 *     gcc q2_numbers.c -o q2_numbers
 *     ./q2_numbers ../samples/sample.c
 *
 * A number is: digits, optionally followed by .digits, optionally followed by an exponent
 * (e.g. 2.5e3). A '-' is taken as the sign of the number only when it cannot be subtraction,
 * i.e. when the token before it is not an operand (identifier, number, ')' or ']').
 * So "x = -1234" gives -1234, while "count-5" gives 5.
 * Numbers inside comments, strings and identifiers (like x1) are ignored.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Read the whole file into a NUL-terminated buffer. */
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
        fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
        return 1;
    }
    char *s = read_file(argv[1]);
    if (!s) { perror(argv[1]); return 1; }

    int line = 1, count = 0;
    int after_operand = 0;   /* 1 if the last token can end an operand */
    size_t i = 0;

    printf("%-8s%s\n", "Line", "Constant");
    printf("-------------------------\n");

    while (s[i]) {
        char c = s[i];

        if (c == '\n') { line++; i++; continue; }
        if (isspace((unsigned char)c)) { i++; continue; }

        /* // comment */
        if (c == '/' && s[i + 1] == '/') {
            while (s[i] && s[i] != '\n') i++;
            continue;
        }
        /* block comment */
        if (c == '/' && s[i + 1] == '*') {
            i += 2;
            while (s[i] && !(s[i] == '*' && s[i + 1] == '/')) { if (s[i] == '\n') line++; i++; }
            if (s[i]) i += 2;
            continue;
        }
        /* string or character literal */
        if (c == '"' || c == '\'') {
            char quote = c;
            i++;
            while (s[i] && s[i] != quote && s[i] != '\n') { if (s[i] == '\\' && s[i + 1]) i++; i++; }
            if (s[i] == quote) i++;
            after_operand = 1;
            continue;
        }
        /* identifier: skip it whole, so the 1 in x1 is not a number */
        if (isalpha((unsigned char)c) || c == '_') {
            while (isalnum((unsigned char)s[i]) || s[i] == '_') i++;
            after_operand = 1;
            continue;
        }
        /* number, possibly with a leading minus sign */
        int negative = 0;
        size_t start = i;
        if (c == '-' && !after_operand) {
            size_t j = i + 1;
            while (s[j] == ' ' || s[j] == '\t') j++;
            if (isdigit((unsigned char)s[j])) { negative = 1; start = j; }
        }
        if (isdigit((unsigned char)c) || negative) {
            size_t j = start;
            while (isdigit((unsigned char)s[j])) j++;
            if (s[j] == '.' && isdigit((unsigned char)s[j + 1])) {
                j++;
                while (isdigit((unsigned char)s[j])) j++;
            }
            if ((s[j] == 'e' || s[j] == 'E') &&
                (isdigit((unsigned char)s[j + 1]) ||
                 ((s[j + 1] == '+' || s[j + 1] == '-') && isdigit((unsigned char)s[j + 2])))) {
                j += 2;
                while (isdigit((unsigned char)s[j])) j++;
            }
            printf("%-8d%s%.*s\n", line, negative ? "-" : "", (int)(j - start), s + start);
            count++;
            i = j;
            after_operand = 1;
            continue;
        }
        /* any other character: ')' and ']' end an operand, everything else does not */
        after_operand = (c == ')' || c == ']');
        i++;
    }

    printf("\nTotal numerical constants: %d\n", count);
    free(s);
    return 0;
}
