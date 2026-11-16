/*
 * CS424L - Compiler Construction Lab | Lab 4 | Exercise 1
 *
 * Question:
 * 1. Add support for declarations and scopes to the parser for assignment statements from the
 * previous lab. Assignment statements are of the form id= expr;. Each variable must be
 * declared before it can be used. To do this write a grammar. Convert the grammar into a
 * syntax directed translation scheme. Then implement the SDT inside a predictive parser.
 * For example, a valid input can be of the form:
 *     {
 *     int x ;
 *     int y ;
 *     x = 2 + 3*4;
 *     {
 *     int x ;
 *     x = 5+10;
 *     }
 *     y = 4 + x ;
 *     }
 * Another example:
 *     {
 *     int x ;
 *     int y ;
 *     bool y ;
 *     x = 2 + 3*4;
 *     y = 4 + x ;
 *     }
 * This should show a multiple declarations error, since y has been declared twice.
 *
 * ---------------------------------------------------------------------------
 * 1) Grammar (left-recursion free, so it is LL(1)):
 *
 *     program -> block
 *     block   -> { decls stmts }
 *     decls   -> decl decls | e
 *     decl    -> type id ;
 *     type    -> int | bool
 *     stmts   -> stmt stmts | e
 *     stmt    -> id = expr ;
 *              | block
 *     expr    -> term expr'
 *     expr'   -> + term expr' | e
 *     term    -> factor term'
 *     term'   -> * factor term' | e
 *     factor  -> id | num | ( expr )
 *
 * 2) Syntax directed translation scheme (actions in { braces } run at that point of the body).
 *    "top" is the symbol table of the innermost scope; each table points to its enclosing one.
 *
 *     program -> { top = null; }
 *                block
 *     block   -> '{'  { saved = top; top = new Env(top); }      // open a new scope
 *                decls stmts
 *                '}'  { top = saved; }                          // close the scope
 *     decl    -> type id ;
 *                     { if top.containsLocally(id.lexeme)
 *                           error("multiple declarations of " + id.lexeme);
 *                       else top.put(id.lexeme, type.t); }
 *     type    -> int  { type.t = "int"; }
 *              | bool { type.t = "bool"; }
 *     stmt    -> id   { s = top.get(id.lexeme);
 *                       if s == null error(id.lexeme + " is not declared"); }
 *                = expr ;  { print(id.lexeme@scope " = " expr.postfix); }
 *     factor  -> id   { if top.get(id.lexeme) == null error(id.lexeme + " is not declared");
 *                       factor.postfix = id.lexeme@scope; }
 *              | num  { factor.postfix = num.value; }
 *              | ( expr ) { factor.postfix = expr.postfix; }
 *     (expr', term' append "+" / "*" to the postfix string after their right operand.)
 *
 *    top.get(name) searches the current scope first and then each enclosing scope, so an
 *    inner "int x" hides the outer x (as in the first example). containsLocally only looks
 *    at the current scope, so the same name may be declared again in an inner block, but
 *    not twice in the same block (the second example).
 *
 * 3) The SDT is implemented below as a recursive-descent predictive parser: one function per
 *    non-terminal, with the semantic actions placed at the same positions as in the scheme.
 *    Every variable in the output is tagged with the scope depth it resolves to, e.g. x@2.
 *
 * Build & run:
 *     g++ -std=c++17 scope_parser.cpp -o scope_parser
 *     ./scope_parser tests/valid.txt
 *     ./scope_parser tests/multiple_decl.txt
 *     ./scope_parser tests/undeclared.txt
 * ---------------------------------------------------------------------------
 */

#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace std;

// ----------------------------- Lexical analyzer -----------------------------

enum TokenType { ID, NUM, INT, BOOL, ASSIGN, PLUS, MUL, LPAREN, RPAREN, LBRACE, RBRACE, SEMI, END, INVALID };

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
            if (word == "int")  return {INT, word, line};
            if (word == "bool") return {BOOL, word, line};
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
            case '{': return {LBRACE, "{", line};
            case '}': return {RBRACE, "}", line};
            case ';': return {SEMI, ";", line};
            default:  return {INVALID, string(1, c), line};
        }
    }
};

// ------------------------------ Symbol tables -------------------------------

// One Env per block; prev links to the enclosing block (a chain of symbol tables).
class Env {
    map<string, string> table;   // name -> type

public:
    shared_ptr<Env> prev;
    int depth;

    Env(shared_ptr<Env> p) : prev(std::move(p)), depth(prev ? prev->depth + 1 : 1) {}

    bool containsLocally(const string &name) const { return table.count(name) > 0; }

    void put(const string &name, const string &type) { table[name] = type; }

    // Search this scope, then the enclosing ones. Returns the Env that declares name, or null.
    const Env *find(const string &name) const {
        for (const Env *e = this; e; e = e->prev.get())
            if (e->containsLocally(name)) return e;
        return nullptr;
    }

    string typeOf(const string &name) const { return table.at(name); }
};

// --------------------------- Parser + SDT actions ---------------------------

struct CompileError : runtime_error {
    using runtime_error::runtime_error;
};

class Parser {
    Lexer &lex;
    Token look;
    shared_ptr<Env> top;   // innermost scope

    [[noreturn]] void error(const string &msg) {
        throw CompileError("line " + to_string(look.line) + ": " + msg);
    }

    void match(TokenType t, const string &what) {
        if (look.type != t) error("syntax error: expected '" + what + "' but found '" + look.lexeme + "'");
        look = lex.next();
    }

    string indent() const { return string((top ? top->depth : 0) * 2, ' '); }

    // Semantic action shared by stmt and factor: the variable must already be declared.
    string resolve(const string &name) {
        const Env *e = top->find(name);
        if (!e) error("semantic error: variable '" + name + "' is used before it is declared");
        return name + "@" + to_string(e->depth);
    }

    // block -> { decls stmts }
    void block() {
        match(LBRACE, "{");
        shared_ptr<Env> saved = top;            // { saved = top; top = new Env(top); }
        top = make_shared<Env>(top);
        cout << indent() << "enter scope " << top->depth << "\n";

        decls();
        stmts();

        cout << indent() << "exit scope " << top->depth << "\n";
        match(RBRACE, "}");
        top = saved;                            // { top = saved; }
    }

    // decls -> decl decls | e
    void decls() {
        while (look.type == INT || look.type == BOOL) decl();
    }

    // decl -> type id ;
    void decl() {
        string t = type();
        string name = look.lexeme;
        int line = look.line;
        match(ID, "identifier");
        match(SEMI, ";");
        if (top->containsLocally(name))        // multiple declarations in the same scope
            throw CompileError("line " + to_string(line) + ": multiple declarations error: '" + name +
                               "' is already declared as " + top->typeOf(name) + " in this scope");
        top->put(name, t);
        cout << indent() << "declare " << t << " " << name << "@" << top->depth << "\n";
    }

    // type -> int | bool
    string type() {
        if (look.type == INT)  { match(INT, "int");   return "int"; }
        if (look.type == BOOL) { match(BOOL, "bool"); return "bool"; }
        error("syntax error: expected a type");
    }

    // stmts -> stmt stmts | e      (FOLLOW(stmts) = { '}' })
    void stmts() {
        while (look.type == ID || look.type == LBRACE) stmt();
        if (look.type == INT || look.type == BOOL)
            error("syntax error: declarations must come before statements in a block");
    }

    // stmt -> id = expr ; | block
    void stmt() {
        if (look.type == LBRACE) { block(); return; }

        string target = resolve(look.lexeme);  // id must be declared
        match(ID, "identifier");
        match(ASSIGN, "=");
        string rhs = expr();
        match(SEMI, ";");
        cout << indent() << target << " = " << rhs << "   (postfix)\n";
    }

    // expr -> term expr'      expr' -> + term expr' | e
    string expr() {
        string s = term();
        while (look.type == PLUS) {
            match(PLUS, "+");
            s += " " + term() + " +";
        }
        return s;
    }

    // term -> factor term'    term' -> * factor term' | e
    string term() {
        string s = factor();
        while (look.type == MUL) {
            match(MUL, "*");
            s += " " + factor() + " *";
        }
        return s;
    }

    // factor -> id | num | ( expr )
    string factor() {
        if (look.type == ID) {
            string s = resolve(look.lexeme);
            match(ID, "identifier");
            return s;
        }
        if (look.type == NUM) {
            string s = look.lexeme;
            match(NUM, "number");
            return s;
        }
        if (look.type == LPAREN) {
            match(LPAREN, "(");
            string s = expr();
            match(RPAREN, ")");
            return s;
        }
        error("syntax error: expected id, num or '(' but found '" + look.lexeme + "'");
    }

public:
    explicit Parser(Lexer &l) : lex(l) { look = lex.next(); }

    // program -> block
    bool parse() {
        try {
            block();
            if (look.type != END) error("syntax error: unexpected '" + look.lexeme + "' after the program");
            cout << "\nProgram is valid.\n";
            return true;
        } catch (const CompileError &e) {
            cout << "\nError: " << e.what() << "\n";
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
