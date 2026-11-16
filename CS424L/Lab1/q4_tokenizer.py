"""
CS424L - Compiler Construction Lab | Lab 1 | Exercise 4

Question:
4. Write a program that takes as input a C source file and generates a stream of tokens. For
example if (a<b) should be converted into <IF><LPAREN><ID><LT><ID><RPAREN>. Handle
as many tokens as possible and ignore whitespace and comments that begin with //.

Usage:
    python3 q4_tokenizer.py <c_source_file>
"""

import re
import sys

KEYWORDS = {
    "auto", "break", "case", "char", "const", "continue", "default", "do", "double",
    "else", "enum", "extern", "float", "for", "goto", "if", "int", "long", "register",
    "return", "short", "signed", "sizeof", "static", "struct", "switch", "typedef",
    "union", "unsigned", "void", "volatile", "while", "main", "printf", "scanf",
}

# Order matters: longer operators must be tried before their prefixes (<= before <).
TOKEN_SPEC = [
    ("COMMENT",   r"//[^\n]*"),
    ("BLOCKCMT",  r"/\*[\s\S]*?\*/"),
    ("PREPROC",   r"\#[^\n]*"),
    ("STRING",    r'"(\\.|[^"\\])*"'),
    ("CHARLIT",   r"'(\\.|[^'\\])*'"),
    ("NUM",       r"\d+\.\d+([eE][+-]?\d+)?|\d+"),
    ("ID",        r"[A-Za-z_][A-Za-z0-9_]*"),
    ("INC",       r"\+\+"), ("DEC", r"--"),
    ("PLUSEQ",    r"\+="), ("MINUSEQ", r"-="), ("MULEQ", r"\*="), ("DIVEQ", r"/="),
    ("ARROW",     r"->"),
    ("LE",        r"<="), ("GE", r">="), ("EQ", r"=="), ("NE", r"!="),
    ("AND",       r"&&"), ("OR", r"\|\|"),
    ("SHL",       r"<<"), ("SHR", r">>"),
    ("LT",        r"<"), ("GT", r">"), ("ASSIGN", r"="), ("NOT", r"!"),
    ("PLUS",      r"\+"), ("MINUS", r"-"), ("MUL", r"\*"), ("DIV", r"/"), ("MOD", r"%"),
    ("BITAND",    r"&"), ("BITOR", r"\|"), ("BITXOR", r"\^"), ("BITNOT", r"~"),
    ("LPAREN",    r"\("), ("RPAREN", r"\)"), ("LBRACE", r"\{"), ("RBRACE", r"\}"),
    ("LBRACKET",  r"\["), ("RBRACKET", r"\]"),
    ("SEMI",      r";"), ("COMMA", r","), ("DOT", r"\."), ("COLON", r":"), ("QUESTION", r"\?"),
    ("NEWLINE",   r"\n"),
    ("SKIP",      r"[ \t\r]+"),
    ("UNKNOWN",   r"."),
]
MASTER = re.compile("|".join(f"(?P<{name}>{pattern})" for name, pattern in TOKEN_SPEC))


def tokenize(source):
    """Yield (line_no, token_name, lexeme)."""
    line_no = 1
    for m in MASTER.finditer(source):
        kind, text = m.lastgroup, m.group()
        if kind == "NEWLINE":
            line_no += 1
            continue
        if kind in ("SKIP", "COMMENT", "PREPROC"):
            continue
        if kind == "BLOCKCMT":
            line_no += text.count("\n")
            continue
        if kind == "ID" and text in KEYWORDS:
            kind = text.upper()
        yield line_no, kind, text


def main():
    if len(sys.argv) != 2:
        print("Usage: python3 q4_tokenizer.py <c_source_file>")
        sys.exit(1)

    with open(sys.argv[1], encoding="utf-8", errors="replace") as f:
        source = f.read()

    # Print the token stream one source line at a time, like the example in the question.
    current_line, stream = None, []
    for line_no, kind, _ in tokenize(source):
        if line_no != current_line and stream:
            print(f"{current_line:>4}: {''.join(stream)}")
            stream = []
        current_line = line_no
        stream.append(f"<{kind}>")
    if stream:
        print(f"{current_line:>4}: {''.join(stream)}")


if __name__ == "__main__":
    main()
