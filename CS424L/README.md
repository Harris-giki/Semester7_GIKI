# CS424L – Compiler Construction Lab

Solutions to the CS424L lab exercises, all written in **C**. Each lab has its own folder. Every source file starts with a comment that holds the full question it answers.

```
CS424L/
├── samples/   shared test inputs for Labs 1 & 2 (sample.c, Sample.java, sample.txt)
├── Lab1/      identifiers, numeric constants, word count, C tokenizer
├── Lab2/      numeric constants, identifiers (comments removed), C tokenizer
├── Lab3/      predictive (recursive-descent) parsers for expressions and statements
├── Lab4/      declarations and scopes as a syntax-directed translation
└── Lab5/      three-address code for if / while / do-while
```

## Requirements

You need a C compiler (gcc or clang) and `make`.

| OS | Install |
|----|---------|
| macOS | `xcode-select --install` (`gcc` is clang under that name) |
| Ubuntu / Debian | `sudo apt install build-essential` |
| Windows | use WSL (Ubuntu) and follow the Ubuntu line, or MinGW/MSYS2 |

Check that it works with `gcc --version` and `make --version`.

Every lab folder has a `Makefile`:

```bash
make        # compile every program in that lab
make test   # compile, then run every program on the sample inputs
make clean  # delete the compiled programs
```

To compile one file without make: `gcc file.c -o file`, then run `./file <input>`.

---

## Lab 1

```bash
cd CS424L/Lab1
make
./q1_identifiers ../samples/sample.c     # Q1: every alphanumeric name and the line where it first appears
./q1_identifiers ../samples/Sample.java
./q2_numbers     ../samples/sample.c     # Q2: numeric constants such as 123, 123.10, -1234
./q3_word_count  ../samples/sample.txt   # Q3: counts characters, words and lines
./q4_tokenizer   ../samples/sample.c     # Q4: token stream, e.g. <IF><LPAREN><ID><LT><ID><RPAREN>
```

- **Q1** does not tell keywords and identifiers apart, as the question allows.
- **Q2** counts a `-` as part of the number only when it is a sign. In `x-5` it is subtraction, so the program reports `5`.

## Lab 2

```bash
cd CS424L/Lab2
make
./q1_numbers     ../samples/sample.c     # constants, each labelled integer or real
./q2_identifiers ../samples/sample.c     # identifiers only: comments, strings and C/Java keywords removed
./q2_identifiers ../samples/Sample.java
./q3_tokens      ../samples/sample.c     # token stream from a get_token() lexer function
```

Lab 2 repeats some Lab 1 questions, so its programs are built differently:
- **Q1 and Q2** first remove the comments in a separate pass, then scan what is left.
- **Q3** is written like a real compiler's lexer: `get_token()` returns one token per call.

## Lab 3

```bash
cd CS424L/Lab3
make
./q1_assignment_parser tests/q1_input.txt   # Q1: stmt -> id = expr ; with left recursion removed
./q2_stmt_parser tests/q2_valid.txt         # Q2: the if/for/other statement parser with the expr grammar plugged in
./q2_stmt_parser tests/q2_invalid.txt       # prints "syntax error"
```

Each program can also read from standard input:

```bash
echo "a=a+b*c;" | ./q1_assignment_parser
```

- **Q1** checks each statement on its own. A valid statement is printed in postfix form, so `a=a+b*c;` gives `a a b c * + =`. An invalid one prints `syntax error`.
- **Q2** prints the grammar rules the parser applies, indented by depth.

## Lab 4

```bash
cd CS424L/Lab4
make
./scope_parser tests/valid.txt           # the first example from the question
./scope_parser tests/multiple_decl.txt   # "multiple declarations error" (y declared twice)
./scope_parser tests/undeclared.txt      # a variable used outside its scope
```

The comment at the top of `scope_parser.c` has the grammar and the SDT scheme. Each block gets its own symbol table, linked to the table of the block around it. Every variable in the output is tagged with the scope it belongs to, so in `x@2`, `2` is the inner block. This shows that the inner `int x` hides the outer one.

## Lab 5

```bash
cd CS424L/Lab5
make
./translator tests/lab_example.txt
```

Output for the program in the question:

```
        a = 1
        ifFalse a goto L1
        b = a
L2:
        ifFalse b goto L3
        a = 2
        b = a
        goto L2
L3:
L4:
        c = b
        if b goto L4
L1:
        d = a
```

C has no classes, so each statement type is a kind of syntax-tree node: `IF_STMT`, `WHILE_STMT` or `DO_WHILE_STMT`. Each kind has its own code-generation function: `gen_if()`, `gen_while()` or `gen_do_while()`. `gen_while()` and `gen_do_while()` follow the same pattern as the example's `if_stmt`, creating new labels and emitting jumps.

---

### Note on the question text
The questions were copied from PDF handouts, and the copy garbled a few characters. In the comments at the top of each file, three fixes were made:
- the arrow `æ` is written as `->`
- `f actor` is written as `factor`
- spaced-out keywords such as `i n t` and `i f` are written as `int` and `if`

The wording is otherwise unchanged.
