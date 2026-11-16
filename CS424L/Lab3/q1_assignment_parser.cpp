/*
 * CS424L - Compiler Construction Lab | Lab 3 | Exercise 1
 *
 * Question:
 * 1. Write a parser for statements of the form id= expr;. Examples of valid input are a=a+b*c;,
 * a=3*b; The grammar for this is the following.
 *     stmt   -> id = expr ;
 *     expr   -> expr + term
 *             | term
 *     term   -> term * factor
 *             | factor
 *     factor -> id
 *             | num
 *             | (expr)
 * Here id and num are the tokens for identifiers and numerals respectively. You need to extend
 * the lexical analyzer to return these tokens. As it stands the grammar is left-recursive, so you
 * need to rewrite the grammar to remove left recursion. As before the parser should output
 * "syntax error" if the input is invalid.
 *
 * ---------------------------------------------------------------------------
 * Grammar after removing left recursion (A -> A a | b  becomes  A -> b A', A' -> a A' | e):
 *     stmt   -> id = expr ;
 *     expr   -> term expr'
 *     expr'  -> + term expr' | e
 *     term   -> factor term'
 *     term'  -> * factor term' | e
 *     factor -> id | num | ( expr )
 *
 * Each non-terminal becomes one function of a recursive-descent (predictive) parser.
 * On success the parser also prints the postfix form of the statement, which shows
 * that * binds tighter than + and that both are left-associative.
 *
 * Build & run:
 *     g++ -std=c++17 q1_assignment_parser.cpp -o q1_assignment_parser
 *     ./q1_assignment_parser tests/q1_input.txt        (or type statements on stdin)
 * ---------------------------------------------------------------------------
 */

#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace std;

// ----------------------------- Lexical analyzer -----------------------------

enum TokenType { ID, NUM, ASSIGN, PLUS, MUL, LPAREN, RPAREN, SEMI, END, INVALID };

const char *tokenName(TokenType t) {
    switch (t) {
        case ID: return "id";       case NUM: return "num";
        case ASSIGN: return "=";    case PLUS: return "+";
        case MUL: return "*";       case LPAREN: return "(";
        case RPAREN: return ")";    case SEMI: return ";";
        case END: return "end of input";
        default: return "invalid character";
    }
}

struct Token {
    TokenType type;
    string lexeme;
    int line;
};

class Lexer {
    string src;
    size_t pos = 0;
    int line = 1;

public:
    explicit Lexer(string s) : src(std::move(s)) {}

    Token next() {
        // skip whitespace and // comments
        while (pos < src.size()) {
            if (src[pos] == '\n') { line++; pos++; }
            else if (isspace((unsigned char)src[pos])) pos++;
            else if (src.compare(pos, 2, "//") == 0) {
                while (pos < src.size() && src[pos] != '\n') pos++;
            } else break;
        }
        if (pos >= src.size()) return {END, "", line};

        char c = src[pos];
        if (isalpha((unsigned char)c) || c == '_') {               // id
            size_t start = pos;
            while (pos < src.size() && (isalnum((unsigned char)src[pos]) || src[pos] == '_')) pos++;
            return {ID, src.substr(start, pos - start), line};
        }
        if (isdigit((unsigned char)c)) {                            // num
            size_t start = pos;
            while (pos < src.size() && isdigit((unsigned char)src[pos])) pos++;
            return {NUM, src.substr(start, pos - start), line};
        }
        pos++;
        switch (c) {
            case '=': return {ASSIGN, "=", line};
            case '+': return {PLUS, "+", line};
            case '*': return {MUL, "*", line};
            case '(': return {LPAREN, "(", line};
            case ')': return {RPAREN, ")", line};
            case ';': return {SEMI, ";", line};
            default:  return {INVALID, string(1, c), line};
        }
    }
};

// --------------------------- Predictive parser ------------------------------

struct SyntaxError : runtime_error {
    using runtime_error::runtime_error;
};

class Parser {
    Lexer &lex;
    Token look;          // one-token lookahead
    string postfix;      // translation built while parsing

    void match(TokenType expected) {
        if (look.type != expected) {
            ostringstream msg;
            msg << "line " << look.line << ": expected '" << tokenName(expected)
                << "' but found '" << (look.lexeme.empty() ? tokenName(look.type) : look.lexeme) << "'";
            throw SyntaxError(msg.str());
        }
        look = lex.next();
    }

    void emit(const string &s) { postfix += s + " "; }

    // expr -> term expr'
    void expr() { term(); exprPrime(); }

    // expr' -> + term expr' | e
    void exprPrime() {
        if (look.type == PLUS) {
            match(PLUS); term(); emit("+");
            exprPrime();
        }
        // else: epsilon
    }

    // term -> factor term'
    void term() { factor(); termPrime(); }

    // term' -> * factor term' | e
    void termPrime() {
        if (look.type == MUL) {
            match(MUL); factor(); emit("*");
            termPrime();
        }
    }

    // factor -> id | num | ( expr )
    void factor() {
        if (look.type == ID)       { emit(look.lexeme); match(ID); }
        else if (look.type == NUM) { emit(look.lexeme); match(NUM); }
        else if (look.type == LPAREN) { match(LPAREN); expr(); match(RPAREN); }
        else {
            ostringstream msg;
            msg << "line " << look.line << ": expected id, num or '(' but found '"
                << (look.lexeme.empty() ? tokenName(look.type) : look.lexeme) << "'";
            throw SyntaxError(msg.str());
        }
    }

    // stmt -> id = expr ;
    void stmt() {
        string target = look.lexeme;
        match(ID); match(ASSIGN); expr(); match(SEMI);
        postfix = target + " " + postfix + "=";
    }

    // After an error, skip to just past the next ';' so the remaining statements are still checked.
    void recover() {
        while (look.type != SEMI && look.type != END) look = lex.next();
        if (look.type == SEMI) look = lex.next();
    }

public:
    explicit Parser(Lexer &l) : lex(l) { look = lex.next(); }

    // Parse every statement in the input; return the number of invalid ones.
    int parseAll() {
        int errors = 0, n = 0;
        while (look.type != END) {
            n++;
            postfix.clear();
            try {
                stmt();
                cout << "stmt " << n << ": valid    postfix: " << postfix << "\n";
            } catch (const SyntaxError &e) {
                cout << "stmt " << n << ": syntax error (" << e.what() << ")\n";
                errors++;
                recover();
            }
        }
        return errors;
    }
};

int main(int argc, char **argv) {
    stringstream buffer;
    if (argc > 1) {
        ifstream in(argv[1]);
        if (!in) { cerr << "Cannot open " << argv[1] << "\n"; return 1; }
        buffer << in.rdbuf();
    } else {
        buffer << cin.rdbuf();
    }

    Lexer lexer(buffer.str());
    Parser parser(lexer);
    return parser.parseAll() == 0 ? 0 : 1;
}
