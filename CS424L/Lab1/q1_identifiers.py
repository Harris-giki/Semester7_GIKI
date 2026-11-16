"""
CS424L - Compiler Construction Lab | Lab 1 | Exercise 1

Question:
1. Write a program that takes as input a C or Java source file and finds all alphanumeric names
or identifiers (such as keywords, types, variables, function and class names etc) along with
the line numbers where they were first found. You are not required to distinguish between
the various types of labels and identifiers at this point.

Usage:
    python3 q1_identifiers.py <source_file>
"""

import re
import sys

# An alphanumeric name starts with a letter or underscore and continues with
# letters, digits or underscores. \b stops us from matching the tail of a
# number such as the "f" in 1.5f or the "x" in 0x1F.
NAME = re.compile(r"\b[A-Za-z_][A-Za-z0-9_]*\b")


def find_names(path):
    first_seen = {}  # name -> line number of first occurrence (dict keeps insertion order)
    with open(path, encoding="utf-8", errors="replace") as f:
        for line_no, line in enumerate(f, start=1):
            for match in NAME.finditer(line):
                name = match.group()
                # A name glued to a preceding digit (e.g. 0x1F) is part of a number.
                start = match.start()
                if start > 0 and line[start - 1].isdigit():
                    continue
                first_seen.setdefault(name, line_no)
    return first_seen


def main():
    if len(sys.argv) != 2:
        print("Usage: python3 q1_identifiers.py <source_file>")
        sys.exit(1)

    names = find_names(sys.argv[1])
    print(f"{'Name':<25}First found on line")
    print("-" * 45)
    for name, line_no in names.items():
        print(f"{name:<25}{line_no}")
    print(f"\nTotal unique names: {len(names)}")


if __name__ == "__main__":
    main()
