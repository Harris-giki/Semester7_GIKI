/*
 * CS424L - Compiler Construction Lab | Lab 2 | Exercise 3
 *
 * Question:
 * 3. Write a program that takes as input a C source file and generates a stream of tokens. For
 * example if (a<b) should be converted into <IF><LPAREN><ID><LT><ID><RPAREN>. Handle
 * as many tokens as possible and ignore whitespace and comments that begin with //.
 *
 * Build & run:
 *     flex -o q3_tokens.c q3_tokens.l && cc q3_tokens.c -o q3_tokens
 *     ./q3_tokens <c_source_file>
 *
 * Output: the token stream for each source line, prefixed by the line number.
 */

%option noyywrap yylineno nounput noinput

%{
#include <stdio.h>
int current_line = 0;

/* Print a token; start a new output line whenever the source line changes. */
void tok(const char *name) {
    if (yylineno != current_line) {
        if (current_line != 0) printf("\n");
        printf("%4d: ", yylineno);
        current_line = yylineno;
    }
    printf("<%s>", name);
}
%}

%x BLOCK_COMMENT

%%

"//".*                  { /* ignore // comments */ }
"/*"                    { BEGIN(BLOCK_COMMENT); }
<BLOCK_COMMENT>"*/"     { BEGIN(INITIAL); }
<BLOCK_COMMENT>.|\n     { }
^[ \t]*#.*              { /* ignore preprocessor directives */ }

"auto"      { tok("AUTO"); }
"break"     { tok("BREAK"); }
"case"      { tok("CASE"); }
"char"      { tok("CHAR"); }
"const"     { tok("CONST"); }
"continue"  { tok("CONTINUE"); }
"default"   { tok("DEFAULT"); }
"do"        { tok("DO"); }
"double"    { tok("DOUBLE"); }
"else"      { tok("ELSE"); }
"enum"      { tok("ENUM"); }
"extern"    { tok("EXTERN"); }
"float"     { tok("FLOAT"); }
"for"       { tok("FOR"); }
"goto"      { tok("GOTO"); }
"if"        { tok("IF"); }
"int"       { tok("INT"); }
"long"      { tok("LONG"); }
"register"  { tok("REGISTER"); }
"return"    { tok("RETURN"); }
"short"     { tok("SHORT"); }
"signed"    { tok("SIGNED"); }
"sizeof"    { tok("SIZEOF"); }
"static"    { tok("STATIC"); }
"struct"    { tok("STRUCT"); }
"switch"    { tok("SWITCH"); }
"typedef"   { tok("TYPEDEF"); }
"union"     { tok("UNION"); }
"unsigned"  { tok("UNSIGNED"); }
"void"      { tok("VOID"); }
"volatile"  { tok("VOLATILE"); }
"while"     { tok("WHILE"); }

[A-Za-z_][A-Za-z0-9_]*                  { tok("ID"); }
[0-9]+"."[0-9]+([eE][+-]?[0-9]+)?       { tok("FLOAT_NUM"); }
[0-9]+                                  { tok("NUM"); }
\"(\\.|[^"\\\n])*\"                     { tok("STRING"); }
\'(\\.|[^'\\\n])*\'                     { tok("CHAR_LIT"); }

"++"    { tok("INC"); }
"--"    { tok("DEC"); }
"+="    { tok("PLUSEQ"); }
"-="    { tok("MINUSEQ"); }
"*="    { tok("MULEQ"); }
"/="    { tok("DIVEQ"); }
"->"    { tok("ARROW"); }
"<="    { tok("LE"); }
">="    { tok("GE"); }
"=="    { tok("EQ"); }
"!="    { tok("NE"); }
"&&"    { tok("AND"); }
"||"    { tok("OR"); }
"<<"    { tok("SHL"); }
">>"    { tok("SHR"); }
"<"     { tok("LT"); }
">"     { tok("GT"); }
"="     { tok("ASSIGN"); }
"!"     { tok("NOT"); }
"+"     { tok("PLUS"); }
"-"     { tok("MINUS"); }
"*"     { tok("MUL"); }
"/"     { tok("DIV"); }
"%"     { tok("MOD"); }
"&"     { tok("BITAND"); }
"|"     { tok("BITOR"); }
"^"     { tok("BITXOR"); }
"~"     { tok("BITNOT"); }
"("     { tok("LPAREN"); }
")"     { tok("RPAREN"); }
"{"     { tok("LBRACE"); }
"}"     { tok("RBRACE"); }
"["     { tok("LBRACKET"); }
"]"     { tok("RBRACKET"); }
";"     { tok("SEMI"); }
","     { tok("COMMA"); }
"."     { tok("DOT"); }
":"     { tok("COLON"); }
"?"     { tok("QUESTION"); }

[ \t\r\n]+  { /* ignore whitespace */ }
.           { tok("UNKNOWN"); }

%%

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <c_source_file>\n", argv[0]);
        return 1;
    }
    yyin = fopen(argv[1], "r");
    if (!yyin) { perror(argv[1]); return 1; }
    yylex();
    printf("\n");
    fclose(yyin);
    return 0;
}
