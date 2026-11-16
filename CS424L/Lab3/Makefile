CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

PROGS = q1_assignment_parser q2_stmt_parser

all: $(PROGS)

%: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

test: all
	-./q1_assignment_parser tests/q1_input.txt
	-./q2_stmt_parser tests/q2_valid.txt
	-./q2_stmt_parser tests/q2_invalid.txt

clean:
	rm -f $(PROGS)

.PHONY: all test clean
