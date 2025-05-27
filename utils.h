#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <map>
#include <set>
#include <utility>
#include <stack>

#define SUCCESS 0
#define FAILURE 1
#define FILE_ERROR 2
#define GRAMMAR_ERROR 3

#define ENDL '\n'
#define SPACE ' '
#define TAB '\t'

#define COLORED_ERRORS true

const std::string WHITE = COLORED_ERRORS ? "\033[0;m" : "";
const std::string RED = COLORED_ERRORS ? "\033[0;31m" : "";
const std::string GREEN = COLORED_ERRORS ? "\033[0;32m" : "";
const std::string YELLOW = COLORED_ERRORS ? "\033[0;33m" : "";

enum token_type {
    T_Bool,
    T_Break,
    T_Continue,
    T_Else,
    T_False,
    T_Fn,
    T_Int,
    T_If,
    T_Let,
    T_Loop,
    T_Mut,
    T_Return,
    T_Print,
    T_True,

    T_AOp_Trust,
    T_AOp_MN,
    T_AOp_ML,
    T_AOp_DV,
    T_AOp_RM,

    T_ROp_L,
    T_ROp_G,
    T_ROp_LE,
    T_ROp_GE,
    T_ROp_NE,
    T_ROp_E,

    T_LOp_AND,
    T_LOp_OR,
    T_LOp_NOT,

    T_Assign,
    T_LP,
    T_RP,
    T_LC,
    T_RC,
    T_LB,
    T_RB,

    T_Semicolon,
    T_Comma,
    T_Colon,
    T_Arrow,
    T_Id,
    T_Decimal,
    T_Hexadecimal,
    T_String,
    T_Comment,
    T_Whitespace,

    Invalid,
    Eof
};

enum symbol_type {
    TERMINAL,
    VARIABLE
};

const std::string type_to_string[] = {
        "T_Bool",
        "T_Break",
        "T_Continue",
        "T_Else",
        "T_False",
        "T_Fn",
        "T_Int",
        "T_If",
        "T_Let",
        "T_Loop",
        "T_Mut",
        "T_Return",
        "T_Print",
        "T_True",

        "T_AOp_Trust",
        "T_AOp_MN",
        "T_AOp_ML",
        "T_AOp_DV",
        "T_AOp_RM",

        "T_ROp_L",
        "T_ROp_G",
        "T_ROp_LE",
        "T_ROp_GE",
        "T_ROp_NE",
        "T_ROp_E",

        "T_LOp_AND",
        "T_LOp_OR",
        "T_LOp_NOT",

        "T_Assign",
        "T_LP",
        "T_RP",
        "T_LC",
        "T_RC",
        "T_LB",
        "T_RB",

        "T_Semicolon",
        "T_Comma",
        "T_Colon",
        "T_Arrow",
        "T_Id",
        "T_Decimal",
        "T_Hexadecimal",
        "T_String",
        "T_Comment",
        "T_Whitespace",

        "Invalid",
        "Eof"
};

inline std::vector<std::string> split(const std::string &s, char ch = ' ') {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ch)) {
        if (!token.empty()) result.push_back(token);
    }
    return result;
}

inline std::string strip(const std::string &s, char ch = ' ') {
    size_t start = s.find_first_not_of(ch);
    size_t end = s.find_last_not_of(ch);
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

class Token {
private:
    token_type type;
    int line_number;
    std::string content;

public:
    Token(token_type _type, int _line_number = -1, std::string _content = "") {
        type = _type;
        line_number = _line_number;
        content = _content;
    }

    token_type get_type() {
        return type;
    }

    int get_line_number() {
        return line_number;
    }

    std::string get_content() {
        return content;
    }


    void set_type(token_type _type) {
        type = _type;
    }

    void set_line_number(int _line_number) {
        line_number = _line_number;
    }

    void set_content(std::string _content) {
        content = _content;
    }


    std::string toString() const {
        if (content == "") {
            return "< type: " + type_to_string[type] + ", line: " + std::to_string(line_number) + " >";
        }
        return "< type: " + type_to_string[type] + ", line: " + std::to_string(line_number) + ", content: " + content +
               " >";
    }

    friend std::ostream &operator<<(std::ostream &out, const Token &token) {
        return out << token.toString();
    }
};

class Symbol {
private:
    std::string name;
    symbol_type type;
    int line_number;
    std::string content;

public:
    Symbol() {}

    Symbol(std::string _name, symbol_type _type) {
        name = std::move(_name);
        type = _type;
        line_number = -1;
        content = "";
    }

    void set_name(std::string _name) {
        name = std::move(_name);
    }

    std::string get_name() const {
        return name;
    }

    void set_type(symbol_type _type) {
        type = _type;
    }

    symbol_type get_type() const {
        return type;
    }

    void set_line_number(int _line_number) {
        line_number = _line_number;
    }

    int get_line_number() {
        return line_number;
    }

    void set_content(std::string _content) {
        content = _content;
    }

    std::string get_content() {
        return content;
    }

    bool operator==(const Symbol &other) const {
        return name == other.get_name() && type == other.get_type();
    }

    bool operator!=(const Symbol &other) const {
        return name != other.get_name() || type != other.get_type();
    }

    bool operator<(const Symbol &other) const {
        return name < other.get_name();
    }

    bool operator>(const Symbol &other) const {
        return name > other.get_name();
    }

    std::string toString() {
        std::string res;
        if (type == TERMINAL) {
            res = name;
        } else {
            res = "<" + name + ">";
        }
        return res;
    }

    friend std::ostream &operator<<(std::ostream &out, Symbol &var) {
        return out << var.toString();
    }
};

template<typename T>
class Node {
private:
    T data;
    Node<T> *parent;
    std::deque<Node<T> *> children;

public:
    Node() {
        data = T();
        parent = nullptr;
    }

    Node(T _data, Node<T> *_parent = nullptr) {
        data = _data;
        parent = _parent;
    }

    void set_date(T _data) {
        data = _data;
    }

    T &get_data() {
        return data;
    }

    void set_parent(Node<T> *_parent) {
        parent = _parent;
    }

    Node<T> *get_parent() {
        return parent;
    }

    void push_back_children(Node<T> *node) {
        children.push_back(node);
    }

    void push_front_children(Node<T> *node) {
        children.push_front(node);
    }

    std::deque<Node<T> *> get_children() {
        return children;
    }
};

template<typename T>
class Tree {
private:
    Node<T> *root;

    void _print_tree(Node<T> *node, bool *has_par, int num = 0, bool last = false) {
        T var = node->get_data();

        for (int i = 0; i < num * TAB - TAB; i++) {
            if (has_par[i]) {
                std::cout << "│";
            } else {
                std::cout << " ";
            }
        }
        if (num) {
            if (last) {
                std::cout << "└── ";
            } else {
                std::cout << "├── ";
            }
        }
        std::cout << var << std::endl;
        if (node->get_data().get_content() != "") {
            for (int i = 0; i < num * TAB; i++) {
                if (has_par[i]) {
                    std::cout << "│";
                } else {
                    std::cout << " ";
                }
            }
            std::cout << "└── '" << node->get_data().get_content() << "'" << std::endl;
        }

        has_par[num * TAB] = true;
        std::deque<Node<T> *> children = node->get_children();
        for (auto child: children) {
            bool end = false;
            if (child == children.back()) {
                has_par[num * TAB] = false;
                end = true;
            }
            _print_tree(child, has_par, num + 1, end);
        }
    }

public:
    Tree() {
        root = nullptr;
    }

    Tree(Node<T> *_root) {
        root = _root;
    }

    void set_root(Node<T> *_root) {
        root = _root;
    }

    Node<T> *get_root() {
        return root;
    }

    void print_tree() {
        bool has_par[200];
        std::fill(has_par, has_par + 200, false);
        _print_tree(root, has_par);
    }
};

#endif // UTILS_H