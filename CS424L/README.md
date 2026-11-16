# CS424L – Compiler Construction Lab

Solutions to the CS424L lab exercises. Each lab has its own folder. Every source file starts with a comment that holds the full question it answers.

```
CS424L/
├── samples/            shared test inputs for Labs 1 & 2 (sample.c, Sample.java, sample.txt)
├── Lab1/   Python      identifiers, numbers, word count, C tokenizer
├── Lab2/   Flex (Lex)  numbers, identifiers (comments removed), C tokenizer
├── Lab3/   C++         predictive parsers for expressions and statements
├── Lab4/   C++         declarations and scopes, done as a syntax-directed translation
└── Lab5/   C++         three-address code for if / while / do-while
```

## Requirements

| Tool | Used by | Check | Install |
|------|---------|-------|---------|
| Python 3.8+ | Lab 1 | `python3 --version` | python.org, or `brew install python` |
| flex | Lab 2 | `flex --version` | already on macOS; Ubuntu: `sudo apt install flex` |
| C/C++ compiler (gcc/clang, C++17) | Labs 2–5 | `g++ --version` | macOS: `xcode-select --install`; Ubuntu: `sudo apt install build-essential` |
| make | Labs 2–5 | `make --version` | comes with the compiler tools above |

On Windows, use WSL (Ubuntu) and follow the Ubuntu instructions.

Every lab folder from 2 to 5 has a `Makefile`:

```bash
make        # build everything in that lab
make test   # build, then run every program on the sample inputs
make clean  # delete the built programs
```

---

## Lab 1: lexical analysis by hand (Python)

```bash
cd CS424L/Lab1
python3 q1_identifiers.py ../samples/sample.c      # Q1: every alphanumeric name and the line where it first appears
python3 q1_identifiers.py ../samples/Sample.java
python3 q2_numbers.py     ../samples/sample.c      # Q2: numeric constants such as 123, 123.10, -1234
python3 q3_word_count.py  ../samples/sample.txt    # Q3: counts characters, words and lines
python3 q4_tokenizer.py   ../samples/sample.c      # Q4: token stream, e.g. <IF><LPAREN><ID><LT><ID><RPAREN>
```

Notes:
- Q1 does not tell keywords and identifiers apart, as the question allows.
- In Q2, a `-` counts as part of the number only when it is a sign. In `x-5` it is subtraction, so the program reports `5`.

## Lab 2: the same problems with Flex

```bash
cd CS424L/Lab2
make
./q1_numbers     ../samples/sample.c
./q2_identifiers ../samples/sample.c     # skips comments, whitespace, strings and C/Java keywords
./q2_identifiers ../samples/Sample.java
./q3_tokens      ../samples/sample.c
```

To build one program by hand:

```bash
flex -o q3_tokens.yy.c q3_tokens.l
cc q3_tokens.yy.c -o q3_tokens
```

## Lab 3: predictive (recursive-descent) parsers

```bash
cd CS424L/Lab3
make
./q1_assignment_parser tests/q1_input.txt    # Q1: stmt -> id = expr ; with left recursion removed
./q2_stmt_parser tests/q2_valid.txt          # Q2: the if/for/other statement parser with the expr grammar plugged in
./q2_stmt_parser tests/q2_invalid.txt        # prints "syntax error"
```

Each program can also read from standard input:

```bash
echo "a=a+b*c;" | ./q1_assignment_parser
```

- **Q1** checks each statement on its own. A valid statement is printed in postfix form, so `a=a+b*c;` gives `a a b c * + =`. An invalid one prints `syntax error`.
- **Q2** prints the grammar rules the parser applies, indented by depth.

## Lab 4: declarations and scopes (SDT)

```bash
cd CS424L/Lab4
make
./scope_parser tests/valid.txt           # the first example from the question
./scope_parser tests/multiple_decl.txt   # "multiple declarations error" (y declared twice)
./scope_parser tests/undeclared.txt      # a variable used outside its scope
```

The comment at the top of `scope_parser.cpp` has the grammar and the SDT scheme. Every variable in the output is tagged with the scope it belongs to, so in `x@2`, `2` is the inner block. This shows that the inner `int x` hides the outer one.

## Lab 5: three-address code for if / while / do-while

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

`while_stmt` and `do_while_stmt` follow the same pattern as the example's `if_stmt` class. Each one has a `gen()` method that creates new labels and emits jumps.

---

### Note on the question text
The questions were copied from PDF handouts, and the copy garbled a few characters. In the comments at the top of each file, three fixes were made:
- the arrow `æ` is written as `->`
- `f actor` is written as `factor`
- spaced-out keywords such as `i n t` and `i f` are written as `int` and `if`

The wording is otherwise unchanged.
