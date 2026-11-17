CC = gcc
CFLAGS = -std=c99 -Wall -Wextra

all: scope_parser

scope_parser: scope_parser.c
	$(CC) $(CFLAGS) $< -o $@

test: all
	-./scope_parser tests/valid.txt
	-./scope_parser tests/multiple_decl.txt
	-./scope_parser tests/undeclared.txt

clean:
	rm -f scope_parser

.PHONY: all test clean
