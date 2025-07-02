#include "semantic_analyzer.h"

std::vector<std::string> split(std::string s, std::vector<char> chs) {
    int n = s.size();
    std::vector<std::string> sp;

    std::string tmp = "";
    for (int i = 0; i < n; i++) {
        bool f = 0;
        for (char ch : chs) {
            if (s[i] == ch) {
                f = 1;
                break;
            }
        }
        if (!f) {
            tmp += s[i];
        }
        else if (tmp != "") {
            sp.push_back(tmp);
            tmp = "";
        }
    }
    if (tmp != "") {
        sp.push_back(tmp);
    }
    return sp;
}

std::string eval(std::string exp, std::vector<char> chs) {
    if (exp == "") {
        return exp;
    }

    int64_t val = 1;
    if (exp[0] == '-') {
        exp = exp.substr(1, exp.size() - 1);
        val = -1;
    }
    std::vector<std::string> sp = split(exp, chs);
    int len = exp.size(), j = 1;
    int64_t result = stoi(sp[0]) * val;
    if (chs.size() == 2) {
        for (int i = 0; i < len; i++) {
            if (exp[i] == '+') {
                result += stoi(sp[j++]);
            }
            else if (exp[i] == '-') {
                result -= stoi(sp[j++]);
            }
        }
    }
    else if (chs.size() == 3) {
        for (int i = 0; i < len; i++) {
            if (exp[i] == '*') {
                result *= stoi(sp[j++]);
            }
            else if (exp[i] == '/') {
                if (stoi(sp[j]) != 0) {
                    result /= stoi(sp[j]);
                }
                j++;
            }
            else if (exp[i] == '%') {
                if (stoi(sp[j]) != 0) {
                    result %= stoi(sp[j]);
                }
                j++;
            }
        }
    }

    return std::to_string(result);
}

void SemanticAnalyzer::dfs(Node<Symbol>* node) {
    std::deque<Node<Symbol>*> children = node->get_children();
    Symbol &symbol = node->get_data();
    std::string head_name = symbol.get_name();

    for (auto child : children) {
        dfs(child);
    }

    if (head_name == "T_Int") {
        symbol.set_stype(INT);
    } else if (head_name == "T_Bool") {
        symbol.set_stype(BOOL);
    } else if (head_name == "T_True" || head_name == "T_False") {
        symbol.set_stype(BOOL);
    } else if (head_name == "var_declaration") {
        SymbolTableEntry entry(VAR);
        int children_size = children[3]->get_children()[1]->get_children().size();
        
        if (children[3]->get_children().empty()) {
            entry.set_stype(VOID);
        } else if (children_size == 5) {
            entry.set_stype(ARRAY);
            entry.set_arr_len(stoi(children[3]->get_children()[1]->get_children()[3]->get_data().get_content()));
            entry.set_arr_type((children[3]->get_children()[1]->get_children()[1]->get_children()[0]->get_data().get_name() == "T_Int") ? INT : BOOL);
        } else if (children_size == 3) {
            entry.set_stype(TUPLE);
            std::vector<std::pair<std::string, semantic_type>> params_type;
            params_type.push_back({"", children[3]->get_children()[1]->get_children()[1]->get_children()[0]->get_children()[0]->get_data().get_stype()});
            auto tmp_child = children[3]->get_children()[1]->get_children()[1]->get_children()[1];
            while (tmp_child->get_children()[0]->get_data().get_name() != "eps") {
                params_type.push_back({"", tmp_child->get_children()[1]->get_children()[0]->get_data().get_stype()});
                tmp_child = tmp_child->get_children()[2];
            }

            int idx = 0;
            tmp_child = children[2];
            if (tmp_child->get_children().size() == 1) {
                params_type[idx].first = tmp_child->get_children()[0]->get_data().get_content();
            } else {
                params_type[idx].first = tmp_child->get_children()[1]->get_children()[0]->get_data().get_content();
                tmp_child = tmp_child->get_children()[1]->get_children()[1];
                while (tmp_child->get_children()[0]->get_data().get_name() != "eps") {
                    params_type[idx].first = tmp_child->get_children()[1]->get_data().get_content();
                    tmp_child = tmp_child->get_children()[2];
                }
            }

            for (std::pair<std::string, semantic_type> &param : params_type) {
                entry.add_to_parameters(param);
            }
        } else {
            entry.set_stype(children[3]->get_children()[1]->get_children()[0]->get_data().get_stype());
        }

        entry.set_mut(children[1]->get_children().empty() ? false : true);
        entry.set_def_area(def_area);

        symbol_table[current_func].push_back(entry);
    }

}

SemanticAnalyzer::SemanticAnalyzer(Tree<Symbol> _parse_tree, std::string output_file_name) {
    parse_tree = _parse_tree;
    out_address = output_file_name;
    def_area = 0;
    current_func = "";
    num_errors = 0;
}