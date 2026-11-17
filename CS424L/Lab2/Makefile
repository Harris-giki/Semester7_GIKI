CC = gcc
CFLAGS = -std=c99 -Wall -Wextra

PROGS = q1_numbers q2_identifiers q3_tokens

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

test: all
	./q1_numbers ../samples/sample.c
	./q2_identifiers ../samples/sample.c
	./q2_identifiers ../samples/Sample.java
	./q3_tokens ../samples/sample.c

clean:
	rm -f $(PROGS)

.PHONY: all test clean
