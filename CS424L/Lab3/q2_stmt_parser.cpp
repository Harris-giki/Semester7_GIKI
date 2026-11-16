/*
 * CS424L - Compiler Construction Lab | Lab 3 | Exercise 2
 *
 * Question:
 * 2. Combine the expression parser from the previous exercise with the example. Replace the
 * expr terminal with the grammar for expr.
 *
 * ---------------------------------------------------------------------------
 * The lab example is the predictive parser for statements (Dragon Book, Sec. 2.4.2), in which
 * expr is treated as a single terminal:
 *     stmt    -> expr ;
 *              | if ( expr ) stmt
 *              | for ( optexpr ; optexpr ; optexpr ) stmt
 *              | other
 *     optexpr -> expr | e
 *
 * Here expr is no longer a terminal. It is replaced by the (left-recursion free) expression
 * grammar from Exercise 1, and the assignment statement from Exercise 1 is kept as well:
 *     program -> stmt program | e
 *     stmt    -> id = expr ;            (chosen when the next two tokens are id and '=')
 *              | expr ;
 *              | if ( expr ) stmt
 *              | for ( optexpr ; optexpr ; optexpr ) stmt
 *              | other
 *     optexpr -> expr | e
 *     expr    -> term expr'
 *     expr'   -> + term expr' | e
 *     term    -> factor term'
 *     term'   -> * factor term' | e
 *     factor  -> id | num | ( expr )
 *
 * "if", "for" and "other" are keywords. The parser prints the derivation (the procedures it
 * enters, indented by depth) so you can see how the input was recognised, and prints
 * "syntax error" if the input is invalid.
 *
 * Build & run:
 *     g++ -std=c++17 q2_stmt_parser.cpp -o q2_stmt_parser
 *     ./q2_stmt_parser tests/q2_valid.txt
 *     ./q2_stmt_parser tests/q2_invalid.txt
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

enum TokenType { ID, NUM, IF, FOR, OTHER, ASSIGN, PLUS, MUL, LPAREN, RPAREN, SEMI, END, INVALID };

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
        while (pos < src.size()) {
            if (src[pos] == '\n') { line++; pos++; }
            else if (isspace((unsigned char)src[pos])) pos++;
            else if (src.compare(pos, 2, "//") == 0) {
                while (pos < src.size() && src[pos] != '\n') pos++;
            } else break;
        }
        if (pos >= src.size()) return {END, "end of input", line};

        char c = src[pos];
        if (isalpha((unsigned char)c) || c == '_') {
            size_t start = pos;
            while (pos < src.size() && (isalnum((unsigned char)src[pos]) || src[pos] == '_')) pos++;
            string word = src.substr(start, pos - start);
            if (word == "if")    return {IF, word, line};
            if (word == "for")   return {FOR, word, line};
            if (word == "other") return {OTHER, word, line};
            return {ID, word, line};
        }
        if (isdigit((unsigned char)c)) {
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
    Token look;      // current lookahead
    Token peekTok;   // the token after look (only needed to tell "id = expr;" from "expr;")
    int depth = 0;

    void advance() { look = peekTok; peekTok = lex.next(); }

    void trace(const string &rule) { cout << string(depth * 2, ' ') << rule << "\n"; }

    [[noreturn]] void error(const string &expected) {
        ostringstream msg;
        msg << "line " << look.line << ": expected " << expected << " but found '" << look.lexeme << "'";
        throw SyntaxError(msg.str());
    }

    void match(TokenType t, const string &what) {
        if (look.type != t) error("'" + what + "'");
        advance();
    }

    // RAII helper so that the trace is indented by recursion depth
    struct Enter {
        Parser &p;
        Enter(Parser &p, const string &rule) : p(p) { p.trace(rule); p.depth++; }
        ~Enter() { p.depth--; }
    };

    // ---- statements (from the lab example) ----

    void stmt() {
        switch (look.type) {
            case IF: {
                Enter e(*this, "stmt -> if ( expr ) stmt");
                match(IF, "if"); match(LPAREN, "("); expr(); match(RPAREN, ")"); stmt();
                break;
            }
            case FOR: {
                Enter e(*this, "stmt -> for ( optexpr ; optexpr ; optexpr ) stmt");
                match(FOR, "for"); match(LPAREN, "(");
                optexpr(); match(SEMI, ";");
                optexpr(); match(SEMI, ";");
                optexpr(); match(RPAREN, ")");
                stmt();
                break;
            }
            case OTHER: {
                Enter e(*this, "stmt -> other");
                match(OTHER, "other");
                break;
            }
            default:
                if (look.type == ID && peekTok.type == ASSIGN) {
                    Enter e(*this, "stmt -> id = expr ;   [id = " + look.lexeme + "]");
                    match(ID, "id"); match(ASSIGN, "="); expr(); match(SEMI, ";");
                } else if (look.type == ID || look.type == NUM || look.type == LPAREN) {
                    Enter e(*this, "stmt -> expr ;");
                    expr(); match(SEMI, ";");
                } else {
                    error("a statement");
                }
        }
    }

    void optexpr() {
        if (look.type == ID || look.type == NUM || look.type == LPAREN) {
            Enter e(*this, "optexpr -> expr");
            expr();
        } else {
            trace("optexpr -> e");
        }
    }

    // ---- expressions (from Exercise 1), replacing the old expr terminal ----

    void expr() {
        Enter e(*this, "expr -> term expr'");
        term(); exprPrime();
    }

    void exprPrime() {
        if (look.type == PLUS) {
            Enter e(*this, "expr' -> + term expr'");
            match(PLUS, "+"); term(); exprPrime();
        } else {
            trace("expr' -> e");
        }
    }

    void term() {
        Enter e(*this, "term -> factor term'");
        factor(); termPrime();
    }

    void termPrime() {
        if (look.type == MUL) {
            Enter e(*this, "term' -> * factor term'");
            match(MUL, "*"); factor(); termPrime();
        } else {
            trace("term' -> e");
        }
    }

    void factor() {
        if (look.type == ID)       { trace("factor -> id   [" + look.lexeme + "]"); advance(); }
        else if (look.type == NUM) { trace("factor -> num  [" + look.lexeme + "]"); advance(); }
        else if (look.type == LPAREN) {
            Enter e(*this, "factor -> ( expr )");
            match(LPAREN, "("); expr(); match(RPAREN, ")");
        } else {
            error("id, num or '('");
        }
    }

public:
    explicit Parser(Lexer &l) : lex(l) {
        look = lex.next();
        peekTok = lex.next();
    }

    // program -> stmt program | e
    bool parse() {
        try {
            while (look.type != END) stmt();
            cout << "\nInput is syntactically valid.\n";
            return true;
        } catch (const SyntaxError &e) {
            cout << "\nsyntax error (" << e.what() << ")\n";
            return false;
        }
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
    return parser.parse() ? 0 : 1;
}
