/*
 * CS424L - Compiler Construction Lab | Lab 1 | Exercise 1
 *
 * Question:
 * 1. Write a program that takes as input a C or Java source file and finds all alphanumeric names
 * or identifiers (such as keywords, types, variables, function and class names etc) along with
 * the line numbers where they were first found. You are not required to distinguish between
 * the various types of labels and identifiers at this point.
 *
 * Build & run:
 *     gcc q1_identifiers.c -o q1_identifiers
 *     ./q1_identifiers ../samples/sample.c
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define MAX_NAMES 2048
#define MAX_LEN   128

static char names[MAX_NAMES][MAX_LEN];   /* names in the order they were first found */
static int  first_line[MAX_NAMES];
static int  n_names = 0;

/* Remember a name the first time we see it; ignore later occurrences. */
void record(const char *name, int line) {
    for (int i = 0; i < n_names; i++)
        if (strcmp(names[i], name) == 0) return;
    if (n_names < MAX_NAMES) {
        strcpy(names[n_names], name);
        first_line[n_names] = line;
        n_names++;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
        return 1;
    }
    FILE *fp = fopen(argv[1], "r");
    if (!fp) { perror(argv[1]); return 1; }

    int c, line = 1;
    c = fgetc(fp);
    while (c != EOF) {
        if (isalpha(c) || c == '_') {
            /* an alphanumeric name: letter or _ followed by letters, digits or _ */
            char buf[MAX_LEN];
            int len = 0;
            while (c != EOF && (isalnum(c) || c == '_')) {
                if (len < MAX_LEN - 1) buf[len++] = (char)c;
                c = fgetc(fp);
            }
            buf[len] = '\0';
            record(buf, line);
        } else if (isdigit(c)) {
            /* skip a whole number so the "x" in 0x1F or "f" in 1.5f is not taken as a name */
            while (c != EOF && (isalnum(c) || c == '_' || c == '.')) c = fgetc(fp);
        } else {
            if (c == '\n') line++;
            c = fgetc(fp);
        }
    }
    fclose(fp);

    printf("%-25s%s\n", "Name", "First found on line");
    printf("---------------------------------------------\n");
    for (int i = 0; i < n_names; i++)
        printf("%-25s%d\n", names[i], first_line[i]);
    printf("\nTotal unique names: %d\n", n_names);
    return 0;
}
