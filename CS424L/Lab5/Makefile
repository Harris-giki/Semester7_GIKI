CC = gcc
CFLAGS = -std=c99 -Wall -Wextra

all: translator

translator: translator.c
	$(CC) $(CFLAGS) $< -o $@

test: all
	-./translator tests/lab_example.txt
	-./translator tests/syntax_error.txt

clean:
	rm -f translator

.PHONY: all test clean
