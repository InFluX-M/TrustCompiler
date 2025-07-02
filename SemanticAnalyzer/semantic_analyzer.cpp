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

semantic_type exp_t_to_semantic_type(exp_type t) {
    switch (t) {
        case TYPE_INT: return INT;
        case TYPE_BOOL: return BOOL;
        case TYPE_ARRAY: return ARRAY;
        case TYPE_TUPLE: return TUPLE;
        default: return VOID;
    }
}

void SemanticAnalyzer::dfs(Node<Symbol>* node) {
    std::deque<Node<Symbol>*> children = node->get_children();
    Symbol &symbol = node->get_data();
    std::string head_name = symbol.get_name();
    int line_number = symbol.get_line_number();

    if (head_name == "var_declaration") {
        // handle <pattern>
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
            x.set_mut(children[1]->get_children()[0]->get_data().get_name() == "eps" ? false : true);
            x.set_def_area(def_area);
            
            symbol_table[current_func][names[i]] = x;
        }

        for (auto child : children) {
            dfs(child);
        }

        // handle <type_opt>
        if (children[3]->get_children()[0]->get_data().get_name() != "eps") {
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
        }

        // handle <assign_opt>
        if (children[4]->get_children()[0]->get_data().get_name() != "eps") {
            if (children_size == 3) {
                std::vector<semantic_type> params_type = children[4]->get_children()[1]->get_data().get_params_type();
                if (params_type.size() != names.size()) {
                    std::cerr << RED 
                            << "Semantic Error [Line " << line_number << "]: "
                            << "Mismatch in tuple assignment for identifier '" << symbol.get_content() << "'.\n"
                            << "  - Assigned " << params_type.size() << " value(s) but expected " << names.size() << ".\n"
                            << "  - Ensure the number of assigned values matches the number of variables in the tuple.\n"
                            << WHITE << std::endl;
                    std::cerr << "----------------------------------------------------------------" << std::endl;
                    num_errors++;
                } else {
                    int idx = 0;
                    for (std::string name : names) {
                        if (params_type[idx] == VOID) {
                            std::cerr << RED 
                                    << "Semantic Error [Line " << line_number << "]: "
                                    << "Unable to infer type for variable '" << name << "' from the assigned expression.\n"
                                    << "  - The expression has an unsupported or unknown type.\n"
                                    << "  - Ensure the expression is valid and has a type like int, bool, array, or tuple.\n"
                                    << WHITE << std::endl;
                            std::cerr << "----------------------------------------------------------------" << std::endl;
                            num_errors++;
                        } else {
                            symbol_table[current_func][name].set_stype(params_type[idx++]);
                        }
                    }
                }
            } else {
                exp_type exp_t = children[4]->get_children()[1]->get_data().get_exp_type();
                if (exp_t == TYPE_INT) {
                    symbol_table[current_func][names[0]].set_stype(INT);
                } else if (exp_t == TYPE_BOOL) {
                    symbol_table[current_func][names[0]].set_stype(BOOL);
                } else if (exp_t == TYPE_ARRAY) {
                    symbol_table[current_func][names[0]].set_stype(ARRAY);
                } else if (exp_t == TYPE_TUPLE) {
                    symbol_table[current_func][names[0]].set_stype(TUPLE);
                    for (semantic_type stype : children[4]->get_children()[1]->get_data().get_params_type()) {
                        symbol_table[current_func][names[0]].add_to_parameters({"", stype});
                    }
                } else {
                    std::cerr << RED 
                            << "Semantic Error [Line " << line_number << "]: "
                            << "Unable to infer type for variable '" << names[0] << "' from the assigned expression.\n"
                            << "  - The expression has an unsupported or unknown type.\n"
                            << "  - Ensure the expression is valid and has a type like int, bool, array, or tuple.\n"
                            << WHITE << std::endl;
                    std::cerr << "----------------------------------------------------------------" << std::endl;
                    num_errors++;
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
    else if (head_name == "exp") {
        symbol.set_exp_type(children[0]->get_data().get_exp_type());
        for (semantic_type stype : children[0]->get_data().get_params_type()) {
                symbol.add_to_params_type(stype);
        }

    } else if (head_name == "log_exp" or head_name == "rel_exp" or head_name == "eq_exp" or head_name == "cmp_exp") {
        if (children[1]->get_children()[0]->get_data().get_name() != "eps") {
            symbol.set_exp_type(TYPE_BOOL);
        } else {
            symbol.set_exp_type(children[0]->get_data().get_exp_type());
            for (semantic_type stype : children[0]->get_data().get_params_type()) {
                symbol.add_to_params_type(stype);
            }
        }
    } else if (head_name == "arith_exp" or head_name == "arith_term") {
        if (children[1]->get_children()[0]->get_data().get_name() != "eps") {
            symbol.set_exp_type(TYPE_INT);
        } else {
            symbol.set_exp_type(children[0]->get_data().get_exp_type());
            for (semantic_type stype : children[0]->get_data().get_params_type()) {
                symbol.add_to_params_type(stype);
            }
        }
    } else if (head_name == "arith_factor") {
        if (children[0]->get_data().get_name() == "T_Hexadecimal" or children[0]->get_data().get_name() == "T_Decimal") {
            symbol.set_exp_type(TYPE_INT);
        } else if (children[0]->get_data().get_name() == "T_String") {
            symbol.set_exp_type(TYPE_STRING);
        } else if (children[0]->get_data().get_name() == "T_True" or children[0]->get_data().get_name() == "T_False" or children[0]->get_data().get_name() == "T_LOp_NOT") {
            symbol.set_exp_type(TYPE_BOOL);
        } else if (children[0]->get_data().get_name() == "T_Id") {
            if (children[1]->get_children()[0]->get_data().get_name() == "T_LP") {
                // function calling

            } else if (children[1]->get_children()[0]->get_data().get_name() == "T_LB") {
                // array indexing
                semantic_type arr_type = symbol_table[current_func][children[0]->get_data().get_content()].get_arr_type();
                if (arr_type == INT) {
                    symbol.set_exp_type(TYPE_INT);
                } else if (arr_type == BOOL) {
                    symbol.set_exp_type(TYPE_BOOL);
                } else if (arr_type == TUPLE) {
                    symbol.set_exp_type(TYPE_TUPLE);
                } else if (arr_type == ARRAY) {
                    symbol.set_exp_type(TYPE_ARRAY);
                } else {
                    std::cerr << RED 
                            << "Semantic Error [Line " << line_number << "]: "
                            << "Array type for identifier '" << children[0]->get_data().get_content() << "' is not defined.\n"
                            << "  - Ensure the array is declared with a valid type.\n"
                            << WHITE << std::endl;
                    std::cerr << "----------------------------------------------------------------" << std::endl;
                    num_errors++;
                }
                
            } else {
                // plain variable
                semantic_type arr_type = symbol_table[current_func][children[0]->get_data().get_content()].get_stype();
                if (arr_type == INT) {
                    symbol.set_exp_type(TYPE_INT);
                } else if (arr_type == BOOL) {
                    symbol.set_exp_type(TYPE_BOOL);
                } else if (arr_type == TUPLE) {
                    symbol.set_exp_type(TYPE_TUPLE);
                } else if (arr_type == ARRAY) {
                    symbol.set_exp_type(TYPE_ARRAY);
                } else if (arr_type == VOID) {
                    std::cerr << RED 
                            << "Semantic Error [Line " << line_number << "]: "
                            << "Variable '" << children[0]->get_data().get_content() << "' is not initialized.\n"
                            << "  - Ensure the variable is assigned a value before use.\n"
                            << WHITE << std::endl;
                    std::cerr << "----------------------------------------------------------------" << std::endl;
                    num_errors++;
                }
            }
        } else if (children[0]->get_data().get_name() == "T_LP") {
            if (children[1]->get_children()[1]->get_children()[0]->get_data().get_name() == "T_RP") {
                // parentheses
                symbol.set_exp_type(children[1]->get_children()[1]->get_children()[0]->get_data().get_exp_type());
            } else if (children[1]->get_children()[1]->get_children()[0]->get_data().get_name() == "T_Comma") {
                // tuple
                symbol.set_exp_type(TYPE_TUPLE);
                std::vector<semantic_type> params_type;
                params_type.push_back(exp_t_to_semantic_type(children[1]->get_children()[0]->get_data().get_exp_type()));
                auto tmp_node = children[1]->get_children()[1]->get_children()[1];

                if (tmp_node->get_data().get_name() != "eps") {
                    params_type.push_back(exp_t_to_semantic_type(tmp_node->get_children()[0]->get_data().get_exp_type()));

                    tmp_node = tmp_node->get_children()[1];
                    while (tmp_node->get_children()[0]->get_data().get_name() != "eps") {
                        params_type.push_back(exp_t_to_semantic_type(tmp_node->get_children()[1]->get_data().get_exp_type()));
                        tmp_node = tmp_node->get_children()[2];
                    }
                }

                for (semantic_type stype : params_type) {
                    symbol.add_to_params_type(stype);
                }
            }
        } else if (children[0]->get_data().get_name() == "T_LB") {
            // array literal
            symbol.set_exp_type(TYPE_ARRAY);
        }
    } else if (head_name == "stmt_after_id") {
        if (children.size() != 3) {
            // assign new value to a var or array member
            std::string name = node->get_parent()->get_children()[0]->get_data().get_content();

            if (symbol_table[current_func][name].get_stype() == VOID) {
                semantic_type stp;
                if (children.size() == 2) {
                    stp = exp_t_to_semantic_type(children[1]->get_data().get_exp_type());
                } else {
                    stp = exp_t_to_semantic_type(children[4]->get_data().get_exp_type());
                }

                if (stp == VOID) {
                    std::cerr << RED 
                            << "Semantic Error [Line " << line_number << "]: "
                            << "Unable to infer type for variable '" << name << "' from the assigned expression.\n"
                            << "  - The expression has an unsupported or unknown type.\n"
                            << "  - Ensure the expression is valid and has a type like int, bool, array, or tuple.\n"
                            << WHITE << std::endl;
                    std::cerr << "----------------------------------------------------------------" << std::endl;
                    num_errors++;
                    return;
                } else {
                    symbol_table[current_func][name].set_stype(stp);
                }
            } else if (!symbol_table[current_func][name].get_mut()) {
                std::cerr << RED 
                        << "Semantic Error [Line " << line_number << "]: "
                        << "Cannot assign to immutable variable '" << name << "'.\n"
                        << "  - This variable was not declared as mutable (e.g., 'let mut " << name << "').\n"
                        << "  - To allow mutation, declare the variable with the 'mut' keyword.\n"
                        << WHITE << std::endl;
                std::cerr << "----------------------------------------------------------------" << std::endl;
                num_errors++;
            }
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