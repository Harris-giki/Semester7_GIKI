CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

all: scope_parser

scope_parser: scope_parser.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

test: all
	-./scope_parser tests/valid.txt
	-./scope_parser tests/multiple_decl.txt
	-./scope_parser tests/undeclared.txt

clean:
	rm -f scope_parser

.PHONY: all test clean
