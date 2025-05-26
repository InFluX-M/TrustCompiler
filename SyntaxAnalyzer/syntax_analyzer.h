#ifndef SYNTAX_ANALYZER_H
#define SYNTAX_ANALYZER_H

#include "../utils.h"

#define GRAMMAR_PATH "Test/Grammar.txt"
#define TABLE_PATH "Output/table.txt"
#define START_VAR "program"

enum rule_type {
    VALID,
    SYNCH,
    EMPTY
};

const Symbol eps = Symbol("eps", TERMINAL);

class Rule {
private:
    Symbol head;
    std::vector<Symbol> body;
    rule_type type;

public:
    Rule();
    explicit Rule(rule_type _type);
    void set_head(Symbol _head);
    Symbol &get_head();
    void set_type(rule_type _type);
    rule_type get_type();
    void add_to_body(const Symbol &var);
    std::vector<Symbol> &get_body();
    std::string toString();
    friend std::ostream &operator<<(std::ostream &out, Rule &rule);
};


class SyntaxAnalyzer {
public:
    std::string out_address;
    std::ofstream out;
    std::vector<Token> tokens;
    std::vector<Rule> rules;
    std::map<Symbol, std::vector<Rule>> self_rules;
    std::set<Symbol> variables, terminals;
    std::map<Symbol, std::set<Symbol>> firsts, follows;
    std::map<Symbol, bool> first_done;
    std::map<Symbol, std::set<Symbol>> graph;
    std::map<std::pair<Symbol, Symbol>, Rule> table;
    std::map<token_type, std::string> match;
    Tree<Symbol> tree;
    bool has_par[200]{};
    int num_errors;

    SyntaxAnalyzer(std::vector<Token> _tokens, std::string output_file);

    SyntaxAnalyzer();

    void extract(std::string line);

    void calc_firsts();

    void calc_first(const Symbol& var);

    void print_firsts();

    void print_first(const Symbol& var);

    void calc_follows();

    void calc_follow(const Symbol &var);

    void relaxation();

    void print_follows();

    void print_follow(const Symbol &var);

    void set_matches();

    void make_table();

    void write_table();

    void read_table();

    void write_tree(Node<Symbol> *node, int num = 0, bool last = false);

    void update_grammar();

    void make_tree(bool update = true);

    void write();

    Tree<Symbol> get_tree() const;
};

#endif // SYNTAX_ANALYZER_H