/*
 * CS424L - Compiler Construction Lab | Lab 2 | Exercise 1
 *
 * Question:
 * 1. Write a program that finds all numerical constants used in a given source file. Examples are
 * 123, 123.10, -1234.
 *
 * Build & run:
 *     flex -o q1_numbers.c q1_numbers.l && cc q1_numbers.c -o q1_numbers
 *     ./q1_numbers <source_file>
 */

%option noyywrap yylineno nounput noinput

%{
#include <stdio.h>
int count = 0;
/* Set after a token that can end an operand (identifier, number, ')' or ']').
   Then a following '-' is subtraction, not the sign of a constant. */
int after_operand = 0;
%}

DIGIT   [0-9]
INT     {DIGIT}+
FRAC    {DIGIT}+"."{DIGIT}+
EXP     [eE][+-]?{DIGIT}+
NUMBER  ({FRAC}|{INT}){EXP}?

%x BLOCK_COMMENT

%%

"/*"                    { BEGIN(BLOCK_COMMENT); }
<BLOCK_COMMENT>"*/"     { BEGIN(INITIAL); }
<BLOCK_COMMENT>.|\n     { /* skip comment body */ }

"//".*                  { /* skip line comment */ }
\"(\\.|[^"\\\n])*\"     { after_operand = 1; /* skip string literal */ }
\'(\\.|[^'\\\n])*\'     { after_operand = 1; /* skip char literal */ }

[A-Za-z_][A-Za-z0-9_]*  { after_operand = 1; /* identifiers like x1 are not numbers */ }

"-"[ \t]*{NUMBER}       {
                            if (after_operand) {
                                /* subtraction: print only the number part */
                                char *p = yytext + 1;
                                while (*p == ' ' || *p == '\t') p++;
                                printf("Line %-4d %s\n", yylineno, p);
                            } else {
                                /* negative constant: print without inner spaces */
                                printf("Line %-4d -", yylineno);
                                char *p = yytext + 1;
                                while (*p == ' ' || *p == '\t') p++;
                                printf("%s\n", p);
                            }
                            count++;
                            after_operand = 1;
                        }

{NUMBER}                { printf("Line %-4d %s\n", yylineno, yytext); count++; after_operand = 1; }

[)\]]                   { after_operand = 1; }
[ \t\r\n]+              { /* whitespace does not change after_operand */ }
.                       { after_operand = 0; }

%%

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
        return 1;
    }
    yyin = fopen(argv[1], "r");
    if (!yyin) { perror(argv[1]); return 1; }

    printf("Numerical constants in %s:\n", argv[1]);
    yylex();
    printf("\nTotal numerical constants: %d\n", count);
    fclose(yyin);
    return 0;
}
