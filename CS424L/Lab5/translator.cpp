/*
 * CS424L - Compiler Construction Lab | Lab 5 | Exercise 1
 *
 * Question:
 * The example above only has if statements and assignments of the form id=id or id=num. Add
 * support for while and do-while loops to the example. To do this study the if_stmt class in the
 * example code. Your program should be able to translate the following code.
 *     {
 *     a =1;
 *     if ( a ) {
 *     b = a ;
 *     while ( b ) {
 *     a =2;
 *     b = a ;
 *     }
 *     do{
 *     c = b ;
 *     } while ( b ) ;
 *     }
 *     d = a ;
 *     }
 *
 * ---------------------------------------------------------------------------
 * Grammar:
 *     program -> block
 *     block   -> { stmts }
 *     stmts   -> stmt stmts | e
 *     stmt    -> id = id ;                       (assign_stmt)
 *              | id = num ;                      (assign_stmt)
 *              | if ( id ) stmt                  (if_stmt, from the example)
 *              | while ( id ) stmt               (while_stmt, new)
 *              | do stmt while ( id ) ;          (do_while_stmt, new)
 *              | block                           (block_stmt)
 *
 * The parser builds a syntax tree of stmt objects. Each class has a gen() method that emits
 * three-address code, exactly like if_stmt in the example:
 *
 *   if_stmt       (if (E) S)          while_stmt   (while (E) S)       do_while_stmt (do S while (E);)
 *       ifFalse E goto after          begin:                           begin:
 *       S                                 ifFalse E goto after             S
 *   after:                                S                                if E goto begin
 *                                         goto begin
 *                                     after:
 *
 * while_stmt is if_stmt plus a jump back to a label placed before the test. do_while_stmt
 * runs the body first, and its test jumps back to the top when the condition is true.
 *
 * Build & run:
 *     g++ -std=c++17 translator.cpp -o translator
 *     ./translator tests/lab_example.txt
 * ---------------------------------------------------------------------------
 */

#include <cctype>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

// ----------------------------- Lexical analyzer -----------------------------

enum TokenType { ID, NUM, IF, WHILE, DO, ASSIGN, LPAREN, RPAREN, LBRACE, RBRACE, SEMI, END, INVALID };

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
            if (word == "while") return {WHILE, word, line};
            if (word == "do")    return {DO, word, line};
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
            case '(': return {LPAREN, "(", line};
            case ')': return {RPAREN, ")", line};
            case '{': return {LBRACE, "{", line};
            case '}': return {RBRACE, "}", line};
            case ';': return {SEMI, ";", line};
            default:  return {INVALID, string(1, c), line};
        }
    }
};

// --------------------------- Code generation helpers ------------------------

int labelCount = 0;
string newLabel() { return "L" + to_string(++labelCount); }

void emit(const string &code) { cout << "        " << code << "\n"; }
void emitLabel(const string &label) { cout << label << ":\n"; }

// ------------------------------ Syntax tree ---------------------------------

struct stmt {
    virtual ~stmt() = default;
    virtual void gen() = 0;
};
using StmtPtr = unique_ptr<stmt>;

// id = id ;   or   id = num ;
struct assign_stmt : stmt {
    string target, value;
    assign_stmt(string t, string v) : target(std::move(t)), value(std::move(v)) {}
    void gen() override { emit(target + " = " + value); }
};

// { stmts }
struct block_stmt : stmt {
    vector<StmtPtr> body;
    void gen() override {
        for (auto &s : body) s->gen();
    }
};

// if ( id ) stmt          -- the class from the example
struct if_stmt : stmt {
    string cond;
    StmtPtr body;
    if_stmt(string c, StmtPtr b) : cond(std::move(c)), body(std::move(b)) {}
    void gen() override {
        string after = newLabel();
        emit("ifFalse " + cond + " goto " + after);
        body->gen();
        emitLabel(after);
    }
};

// while ( id ) stmt       -- new
struct while_stmt : stmt {
    string cond;
    StmtPtr body;
    while_stmt(string c, StmtPtr b) : cond(std::move(c)), body(std::move(b)) {}
    void gen() override {
        string begin = newLabel();
        string after = newLabel();
        emitLabel(begin);
        emit("ifFalse " + cond + " goto " + after);   // leave the loop when cond is false
        body->gen();
        emit("goto " + begin);                         // jump back and test again
        emitLabel(after);
    }
};

// do stmt while ( id ) ;  -- new
struct do_while_stmt : stmt {
    string cond;
    StmtPtr body;
    do_while_stmt(StmtPtr b, string c) : cond(std::move(c)), body(std::move(b)) {}
    void gen() override {
        string begin = newLabel();
        emitLabel(begin);
        body->gen();                                   // body always runs at least once
        emit("if " + cond + " goto " + begin);         // repeat while cond is true
    }
};

// ------------------------------- Parser -------------------------------------

struct SyntaxError : runtime_error {
    using runtime_error::runtime_error;
};

class Parser {
    Lexer &lex;
    Token look;

    [[noreturn]] void error(const string &expected) {
        throw SyntaxError("line " + to_string(look.line) + ": expected " + expected +
                          " but found '" + look.lexeme + "'");
    }

    string match(TokenType t, const string &what) {
        if (look.type != t) error("'" + what + "'");
        string lexeme = look.lexeme;
        look = lex.next();
        return lexeme;
    }

    // block -> { stmts }
    unique_ptr<block_stmt> block() {
        auto b = make_unique<block_stmt>();
        match(LBRACE, "{");
        while (look.type != RBRACE && look.type != END) b->body.push_back(statement());
        match(RBRACE, "}");
        return b;
    }

    // condition inside ( ) is an id, as in the example
    string condition() {
        match(LPAREN, "(");
        string c = match(ID, "identifier");
        match(RPAREN, ")");
        return c;
    }

    StmtPtr statement() {
        switch (look.type) {
            case ID: {                                       // id = id ; | id = num ;
                string target = match(ID, "identifier");
                match(ASSIGN, "=");
                string value;
                if (look.type == ID || look.type == NUM) { value = look.lexeme; look = lex.next(); }
                else error("an identifier or number");
                match(SEMI, ";");
                return make_unique<assign_stmt>(target, value);
            }
            case IF: {                                       // if ( id ) stmt
                match(IF, "if");
                string c = condition();
                return make_unique<if_stmt>(c, statement());
            }
            case WHILE: {                                    // while ( id ) stmt
                match(WHILE, "while");
                string c = condition();
                return make_unique<while_stmt>(c, statement());
            }
            case DO: {                                       // do stmt while ( id ) ;
                match(DO, "do");
                StmtPtr body = statement();
                match(WHILE, "while");
                string c = condition();
                match(SEMI, ";");
                return make_unique<do_while_stmt>(std::move(body), c);
            }
            case LBRACE:
                return block();
            default:
                error("a statement");
        }
    }

public:
    explicit Parser(Lexer &l) : lex(l) { look = lex.next(); }

    // program -> block
    StmtPtr program() {
        StmtPtr p = block();
        if (look.type != END) error("end of input");
        return p;
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

    try {
        Lexer lexer(buffer.str());
        Parser parser(lexer);
        StmtPtr prog = parser.program();
        cout << "Three-address code:\n\n";
        prog->gen();
    } catch (const SyntaxError &e) {
        cout << "syntax error (" << e.what() << ")\n";
        return 1;
    }
    return 0;
}
