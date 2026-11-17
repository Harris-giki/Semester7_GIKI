CC = gcc
CFLAGS = -std=c99 -Wall -Wextra

PROGS = q1_identifiers q2_numbers q3_word_count q4_tokenizer

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

test: all
	./q1_identifiers ../samples/sample.c
	./q2_numbers ../samples/sample.c
	./q3_word_count ../samples/sample.txt
	./q4_tokenizer ../samples/sample.c

clean:
	rm -f $(PROGS)

.PHONY: all test clean
