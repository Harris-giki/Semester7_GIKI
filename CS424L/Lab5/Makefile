CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

all: translator

translator: translator.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

test: all
	-./translator tests/lab_example.txt
	-./translator tests/syntax_error.txt

clean:
	rm -f translator

.PHONY: all test clean
