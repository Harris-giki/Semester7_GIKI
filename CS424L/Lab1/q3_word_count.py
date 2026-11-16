"""
CS424L - Compiler Construction Lab | Lab 1 | Exercise 3

Question:
3. Write a program that outputs a count of all characters, words and lines in a given text file.

Usage:
    python3 q3_word_count.py <text_file>
"""

import sys


def count(path):
    chars = words = lines = 0
    in_word = False
    with open(path, encoding="utf-8", errors="replace") as f:
        for ch in f.read():
            chars += 1
            if ch == "\n":
                lines += 1
            if ch.isspace():
                in_word = False
            elif not in_word:
                in_word = True
                words += 1
    # A last line without a trailing newline still counts as a line.
    if chars > 0 and ch != "\n":
        lines += 1
    return chars, words, lines


def main():
    if len(sys.argv) != 2:
        print("Usage: python3 q3_word_count.py <text_file>")
        sys.exit(1)

    chars, words, lines = count(sys.argv[1])
    print(f"Characters: {chars}")
    print(f"Words     : {words}")
    print(f"Lines     : {lines}")


if __name__ == "__main__":
    main()
