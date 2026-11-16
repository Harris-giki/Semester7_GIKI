"""
CS424L - Compiler Construction Lab | Lab 1 | Exercise 2

Question:
2. Write a program that finds all numerical constants used in a given source file. Examples are
123, 123.10, -1234.

Usage:
    python3 q2_numbers.py <source_file>
"""

import re
import sys

# Optional minus sign, integer part, optional fraction, optional exponent.
# The look-behind stops us from matching digits inside identifiers like x1 or arr2d.
NUMBER = re.compile(r"(?<![A-Za-z0-9_.])(-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)(?![A-Za-z_])")


def minus_is_sign(line, minus_pos):
    """A '-' is a sign (not subtraction) if the previous non-space character
    is not something that ends an operand: an identifier, digit or ')' / ']'."""
    i = minus_pos - 1
    while i >= 0 and line[i] in " \t":
        i -= 1
    if i < 0:
        return True
    return not (line[i].isalnum() or line[i] in "_)]")


def strip_strings(line):
    """Blank out string and char literals so numbers inside them are ignored."""
    return re.sub(r'"(\\.|[^"\\])*"|\'(\\.|[^\'\\])*\'', lambda m: " " * len(m.group()), line)


def find_numbers(path):
    results = []  # (line_no, number)
    with open(path, encoding="utf-8", errors="replace") as f:
        source = f.read()
    # Blank out /* block comments */ but keep their newlines so line numbers stay right.
    source = re.sub(r"/\*[\s\S]*?\*/", lambda m: re.sub(r"[^\n]", " ", m.group()), source)
    for line_no, raw in enumerate(source.split("\n"), start=1):
        line = strip_strings(raw.split("//", 1)[0])  # ignore // comments
        for match in NUMBER.finditer(line):
            text = match.group(1)
            # In "a-5" the minus is subtraction, so the constant is just 5.
            if text.startswith("-") and not minus_is_sign(line, match.start(1)):
                text = text[1:]
            results.append((line_no, text))
    return results


def main():
    if len(sys.argv) != 2:
        print("Usage: python3 q2_numbers.py <source_file>")
        sys.exit(1)

    numbers = find_numbers(sys.argv[1])
    print(f"{'Line':<8}Constant")
    print("-" * 25)
    for line_no, text in numbers:
        print(f"{line_no:<8}{text}")
    print(f"\nTotal numerical constants: {len(numbers)}")


if __name__ == "__main__":
    main()
