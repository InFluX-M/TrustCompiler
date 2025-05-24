#include "syntax_analyzer.h"
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <vector>
#include <string>


class Rule {
    private:
        Symbol head;
        std::vector<Symbol> body;
        rule_type type;

    public:
        Rule() {
            type = EMPTY;
        }
        Rule(rule_type _type) {
            type = _type;
        }

        void set_head(Symbol _head) {
            head = _head;
        }
        Symbol& get_head() {
            return head;
        }
        void set_type(rule_type _type) {
            type = _type;
        }
        rule_type get_type() {
            return type;
        }
        void add_to_body(Symbol var) {
            body.push_back(var);
        }
        std::vector<Symbol>& get_body() {
            return body;
        }

        std::string toString() {
            std::string res = "";
            res += head.toString() + " -> ";
            for (auto &part : body) {
                res += part.toString() + " ";
            }
            if (type == SYNCH) {
                res = "SYNCH";
            }
            if (type == EMPTY) {
                res = "EMPTY";
            }
            return res;
        }
        friend std::ostream& operator << (std::ostream &out, Rule &rule) {
            return out << rule.toString();
        }
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
        //std::map<std::pair<Symbol, Symbol>, Rule> table;
        std::map<token_type, std::string> match;
        int num_errors;

        void extract(std::string line) {
            line = strip(line);
            std::string head_str = "", body_str = "";
            int len = (int)line.size();
            for (int i = 0; i < len; i++) {
                if (line[i] == '<' or line[i] == '>') {
                    continue;
                }
                if (line[i] != ' ') {
                    head_str += line[i];
                }
                else {
                    while (line[i] == ' ' || line[i] == '-' || line[i] == '>') {
                        i++;
                    }
                    while (i < len) {
                        body_str += line[i];
                        i++;
                    }
                }
            }

            Symbol head = Symbol(head_str, VARIABLE);
            variables.insert(head);

            std::vector<std::string> rules_str = split(body_str, '@');
            for (auto &rule : rules_str) {
                rule = strip(rule);
            }
            int number_rules = (int)rules_str.size();
            if (number_rules < 1) {
                std::cerr << RED << "Grammar Error: Invalid grammar in file" << WHITE << std::endl;
                exit(GRAMMAR_ERROR);
            }

            for (int i = 0; i < number_rules; i++) {
                Rule rule;
                rule.set_head(head);
                rule.set_type(VALID);
                std::vector<std::string> rule_parts = split(rules_str[i]);
                for (int j = 0; j < (int)rule_parts.size(); j++) {
                    Symbol tmp;
                    if (rule_parts[j] == "ε") {
                        tmp.set_name("eps");
                        tmp.set_type(TERMINAL);
                        terminals.insert(tmp);
                    }
                    else if (rule_parts[j][0] == '<') {
                        tmp.set_name(rule_parts[j].substr(1, rule_parts[j].size() - 2));
                        tmp.set_type(VARIABLE);
                        variables.insert(tmp);
                    } 
                    else {
                        tmp.set_name(rule_parts[j]);
                        tmp.set_type(TERMINAL);
                        terminals.insert(tmp);
                    }
                    rule.add_to_body(tmp);
                }
                rules.push_back(rule);
                self_rules[rule.get_head()].push_back(rule);
            }
        }

        void calc_firsts() {
            for (auto term : terminals) {
                calc_first(term);
            }
            for (auto var : variables) {
                calc_first(var);
            }
        }

        void calc_first(Symbol var) {
            if (var.get_type() == TERMINAL) {
                firsts[var].insert(var);
                first_done[var] = true;
                return;
            }

            for (auto &rule : self_rules[var]) {
                bool exist_eps = false;
                for (auto &part_body : rule.get_body()) {
                    if (first_done[part_body] == false) {
                        calc_first(part_body);
                    }
                    exist_eps = false;
                    for (auto first : firsts[part_body]) {
                        if (first == eps) {
                            exist_eps = true;
                        }
                        else {
                            firsts[var].insert(first);
                        }
                    }
                    if (!exist_eps) {
                        break;
                    }
                }
                if (exist_eps) {
                    firsts[var].insert(eps);
                }
            }
            first_done[var] = true;
        }

        void print_firsts() {
            for (auto term : terminals) {
                print_first(term);
            }
            for (auto var : variables) {
                print_first(var);
            }
        }

        void print_first(Symbol var) {
            std::cout << "First[" + var.get_name() + "]:";
            for (auto first : firsts[var]) {
                std::cout << " " << first;
            }
            std::cout << std::endl;
        }

        void calc_follows() {
            for (auto var : variables) {
                if (var.get_name() == START_VAR) {
                    follows[var].insert(Symbol("$", TERMINAL));
                }
                calc_follow(var);
            }
            relaxation();
        }

        void calc_follow(Symbol var) {
            for (auto &rule : rules) {
                std::vector<Symbol> &body = rule.get_body();
                int body_len = (int)body.size();

                for (int i = 0; i < body_len; i++) {
                    if (body[i] == var) {
                        bool all_eps = true;
                        for (int j = i + 1; j < body_len; j++) {
                            bool has_eps = false;
                            for (auto first : firsts[body[j]]) {
                                if (first == eps) {
                                    has_eps = true;
                                }
                                else {
                                    follows[var].insert(first);
                                }
                            }
                            if (!has_eps) {
                                all_eps = false;
                                break;
                            }
                        }
                        if (all_eps && rule.get_head() != var) {
                            for (auto follow : follows[rule.get_head()]) {
                                follows[var].insert(follow);
                            }
                            graph[rule.get_head()].insert(var);
                        }
                    }
                }
            }
        }

        void relaxation() {
            std::set<Symbol> set;
            for (auto var : variables) {
                set.insert(var);
            }

            while (!set.empty()) {
                Symbol var = *set.begin();
                set.erase(set.begin());

                for (auto v : graph[var]) {
                    int old_size = (int)follows[v].size();
                    for (auto follow : follows[var]) {
                        follows[v].insert(follow);
                    }
                    int new_size = (int)follows[v].size();
                    if (new_size > old_size) {
                        set.insert(v);
                    }
                }
            }
        }

        void print_follows() {
            for (auto var : variables) {
                print_follow(var);
            }
        }

        void print_follow(Symbol var) {
            std::cout << "Follow[" + var.get_name() + "]:";
            for (auto follow : follows[var]) {
                std::cout << " " << follow;
            }
            std::cout << std::endl;
        }

        void set_matches() {
            match[T_Bool] = "bool";
            match[T_Break] = "break";
            match[T_Continue] = "continue";
            match[T_Else] = "else";
            match[T_False] = "false";
            match[T_Fn] = "fn";
            match[T_Int] = "i32";
            match[T_If] = "if";
            match[T_Let] = "let";
            match[T_Loop] = "loop";
            match[T_Mut] = "mut";
            match[T_Print] = "println!";
            match[T_Return] = "return";
            match[T_True] = "true";

            match[T_AOp_AD] = "+";
            match[T_AOp_MN] = "-";
            match[T_AOp_ML] = "*";
            match[T_AOp_DV] = "/";
            match[T_AOp_RM] = "%";

            match[T_ROp_L] = "<";
            match[T_ROp_G] = ">";
            match[T_ROp_LE] = "<=";
            match[T_ROp_GE] = ">=";
            match[T_ROp_NE] = "!=";
            match[T_ROp_E] = "==";

            match[T_LOp_AND] = "&&";
            match[T_LOp_OR] = "||";
            match[T_LOp_NOT] = "!";

            match[T_Assign] = "=";
            match[T_LP] = "(";
            match[T_RP] = ")";
            match[T_LC] = "{";
            match[T_RC] = "}";
            match[T_LB] = "[";
            match[T_RB] = "]";
            match[T_Semicolon] = ";";
            match[T_Comma] = ",";
            match[T_Colon] = ":";
            match[T_Arrow] = "->";

            match[T_Id] = "id";
            match[T_Decimal] = "decimal";
            match[T_Hexadecimal] = "hexadecimal";
            match[T_String] = "string";

            match[Invalid] = "invalid";
            match[Eof] = "$";
        }

    public:
        SyntaxAnalyzer() {
            num_errors = 0;
        }
};

int main() {
    SyntaxAnalyzer syntax_analyzer;
    std::ifstream f("Grammar.txt");  
    std::string line;
    while (std::getline(f, line)) {
        syntax_analyzer.extract(line);
    }
    syntax_analyzer.calc_firsts();
    syntax_analyzer.print_firsts();
    syntax_analyzer.calc_follows();
    syntax_analyzer.print_follows();
}