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
    private:
        std::string out_address;
        std::ofstream out;
        std::vector<Token> tokens;
        std::vector<Rule> rules;
        std::map<Symbol, std::vector<Rule>> self_rules;
        std::set<Symbol> variables, terminals;
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
                    if (rule_parts[j][0] == '<') {
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
};

