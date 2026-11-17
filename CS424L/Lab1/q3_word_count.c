/*
 * CS424L - Compiler Construction Lab | Lab 1 | Exercise 3
 *
 * Question:
 * 3. Write a program that outputs a count of all characters, words and lines in a given text file.
 *
 * Build & run:
 *     gcc q3_word_count.c -o q3_word_count
 *     ./q3_word_count ../samples/sample.txt
 *
 * A word is a maximal run of non-whitespace characters (the same rule as the Unix wc command).
 */

#include <ctype.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <text_file>\n", argv[0]);
        return 1;
    }
    FILE *fp = fopen(argv[1], "r");
    if (!fp) { perror(argv[1]); return 1; }

    long chars = 0, words = 0, lines = 0;
    int in_word = 0, c, last = '\n';

    while ((c = fgetc(fp)) != EOF) {
        chars++;
        if (c == '\n') lines++;
        if (isspace(c)) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            words++;
        }
        last = c;
    }
    /* a last line without a trailing newline still counts as a line */
    if (chars > 0 && last != '\n') lines++;
    fclose(fp);

    printf("Characters: %ld\n", chars);
    printf("Words     : %ld\n", words);
    printf("Lines     : %ld\n", lines);
    return 0;
}
