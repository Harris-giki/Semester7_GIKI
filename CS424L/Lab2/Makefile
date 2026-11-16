CC = cc
CFLAGS = -Wall -Wno-unused-function

PROGS = q1_numbers q2_identifiers q3_tokens

all: $(PROGS)

%: %.l
	flex -o $@.yy.c $<
	$(CC) $(CFLAGS) $@.yy.c -o $@

test: all
	./q1_numbers ../samples/sample.c
	./q2_identifiers ../samples/sample.c
	./q2_identifiers ../samples/Sample.java
	./q3_tokens ../samples/sample.c

clean:
	rm -f $(PROGS) *.yy.c

.PHONY: all test clean
