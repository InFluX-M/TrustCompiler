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
    int line_number = symbol.get_line_number();

    if (head_name == "var_declaration") {
        int children_size = children[2]->get_children().size();
        
        std::vector<std::string> names;
        if (children_size == 3) {
            names.push_back(children[2]->get_children()[1]->get_children()[0]->get_data().get_content());
            auto tmp_child = children[2]->get_children()[1]->get_children()[1];
            while (tmp_child->get_children()[0]->get_data().get_name() != "eps") {
                names.push_back(tmp_child->get_children()[1]->get_data().get_content());
                tmp_child = tmp_child->get_children()[2];
            }
        } else {
            names.push_back(children[2]->get_children()[0]->get_data().get_content());
        }

        for (int i = 0; i < names.size(); i++) {
            SymbolTableEntry x(VAR);
            x.set_name(names[i]);
            x.set_mut(children[1]->get_children().empty() ? false : true);
            x.set_def_area(def_area);
            
            symbol_table[current_func][names[i]] = x;
        }

        for (auto child : children) {
            dfs(child);
        }

        if (children_size == 3) {
            // We have (x, y, z, ...)
            
            if (children[3]->get_children().empty()) {
                for (std::string name : names) {
                    symbol_table[current_func][name].set_stype(VOID);
                }
            } else {
                std::vector<semantic_type> params_type;
                params_type.push_back(children[3]->get_children()[1]->get_children()[1]->get_children()[0]->get_children()[0]->get_data().get_stype());
                auto tmp_child = children[3]->get_children()[1]->get_children()[1]->get_children()[1];
                while (tmp_child->get_children()[0]->get_data().get_name() != "eps") {
                    params_type.push_back(tmp_child->get_children()[1]->get_children()[0]->get_data().get_stype());
                    tmp_child = tmp_child->get_children()[2];
                }

                if (params_type.size() == names.size()) {
                    int idx = 0;
                    for (std::string name : names) {
                        symbol_table[current_func][name].set_stype(params_type[idx++]);
                    }
                }
                else if (params_type.size() != names.size()) {
                    std::cerr << RED 
                            << "Semantic Error [Line " << line_number << "]: "
                            << "Mismatch in tuple declaration for identifier '" << symbol.get_content() << "'.\n"
                            << "  - Declared " << names.size() << " variable(s) but provided " << params_type.size() << " type(s).\n"
                            << "  - Ensure the number of variables matches the number of types in the tuple.\n"
                            << WHITE << std::endl;
                    std::cerr << "----------------------------------------------------------------" << std::endl;
                    num_errors++;
                } 
            }
        } else {
            // We have x

            std::vector<semantic_type> params_type;
            
            if (children[3]->get_children().size() == 1 and children[3]->get_children()[0]->get_data().get_name() == "eps") {
                symbol_table[current_func][names[0]].set_stype(VOID);
            } else {
                int children_size_type = children[3]->get_children()[1]->get_children().size();
                if (children_size_type == 5) {
                    symbol_table[current_func][names[0]].set_stype(ARRAY);
                    symbol_table[current_func][names[0]].set_arr_len(stoi(children[3]->get_children()[1]->get_children()[3]->get_data().get_content()));
                    symbol_table[current_func][names[0]].set_arr_type((children[3]->get_children()[1]->get_children()[1]->get_children()[0]->get_data().get_name() == "T_Int") ? INT : BOOL);
                } else if (children_size_type == 3) {
                    symbol_table[current_func][names[0]].set_stype(TUPLE);
                    std::vector<std::pair<std::string, semantic_type>> params_type;
                    params_type.push_back({"", children[3]->get_children()[1]->get_children()[1]->get_children()[0]->get_children()[0]->get_data().get_stype()});
                    auto tmp_child = children[3]->get_children()[1]->get_children()[1]->get_children()[1];
                    while (tmp_child->get_children()[0]->get_data().get_name() != "eps") {
                        params_type.push_back({"", tmp_child->get_children()[1]->get_children()[0]->get_data().get_stype()});
                        tmp_child = tmp_child->get_children()[2];
                    }

                    for (std::pair<std::string, semantic_type> &param : params_type) {
                        symbol_table[current_func][names[0]].add_to_parameters(param);
                    }
                } else {
                    symbol_table[current_func][names[0]].set_stype(children[3]->get_children()[1]->get_children()[0]->get_data().get_stype());
                }
            }
        }
    } else {
        for (auto child : children) {
            dfs(child);
        }
    }



    if (head_name == "T_Int") {
        symbol.set_stype(INT);
    } else if (head_name == "T_Bool") {
        symbol.set_stype(BOOL);
    } else if (head_name == "T_True" || head_name == "T_False") {
        symbol.set_stype(BOOL);
    } else if (head_name == "T_Id" and node->get_parent()->get_data().get_name() != "pattern" and 
               node->get_parent()->get_data().get_name() != "func" and node->get_parent()->get_data().get_name() != "id_ls" 
               and node->get_parent()->get_data().get_name() != "id_ls_tail") {
        bool find = false;
        for (const auto& entry : symbol_table[current_func]) {
            if (entry.first == symbol.get_content()) {
                find = true;
            }
        }

        if (!find) {
            std::cerr << RED 
                    << "Semantic Error [Line " << line_number << "]: "
                    << "Use of undeclared identifier '" << symbol.get_content() << "'.\n"
                    << "  - The identifier must be declared before it is used.\n"
                    << "  - Check for missing declarations or typos in the name.\n"
                    << WHITE << std::endl;
            std::cerr << "----------------------------------------------------------------" << std::endl;
            num_errors++;
        }
    }

}

SemanticAnalyzer::SemanticAnalyzer(Tree<Symbol> _parse_tree, std::string output_file_name) {
    parse_tree = _parse_tree;
    out_address = output_file_name;
    def_area = 0;
    current_func = "";
    num_errors = 0;
}