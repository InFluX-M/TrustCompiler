#include "semantic_analyzer.h"

std::vector<std::string> split(std::string s, std::vector<char> chs) {
    int n = s.size();
    std::vector<std::string> sp;

    std::string tmp;
    for (int i = 0; i < n; i++) {
        bool f = false;
        for (char ch: chs) {
            if (s[i] == ch) {
                f = true;
                break;
            }
        }
        if (!f) {
            tmp += s[i];
        } else if (!tmp.empty()) {
            sp.push_back(tmp);
            tmp = "";
        }
    }
    if (!tmp.empty()) {
        sp.push_back(tmp);
    }
    return sp;
}

std::string eval(std::string exp, std::vector<char> chs) {
    if (exp.empty()) {
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
            } else if (exp[i] == '-') {
                result -= stoi(sp[j++]);
            }
        }
    } else if (chs.size() == 3) {
        for (int i = 0; i < len; i++) {
            if (exp[i] == '*') {
                result *= stoi(sp[j++]);
            } else if (exp[i] == '/') {
                if (stoi(sp[j]) != 0) {
                    result /= stoi(sp[j]);
                }
                j++;
            } else if (exp[i] == '%') {
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
        case TYPE_INT:
            return INT;
        case TYPE_BOOL:
            return BOOL;
        case TYPE_ARRAY:
            return ARRAY;
        case TYPE_TUPLE:
            return TUPLE;
        case TYPE_VOID:
            return VOID;
        default:
            return UNK;
    }
}

std::string exp_t_to_string(exp_type t) {
    switch (t) {
        case TYPE_INT:
            return "INT";
        case TYPE_BOOL:
            return "BOOL";
        case TYPE_ARRAY:
            return "ARRAY";
        case TYPE_TUPLE:
            return "TUPLE";
        case TYPE_VOID:
            return "VOID";
        default:
            return "UNK";
    }
}

void SemanticAnalyzer::dfs(Node<Symbol> *node) {
    std::deque<Node<Symbol> *> children = node->get_children();
    Symbol &symbol = node->get_data();
    std::string head_name = symbol.get_name();
    int line_number = symbol.get_line_number();

    bool is_new_scope = false;

    if (head_name == "func" || head_name == "loop_stmt" || head_name == "if_stmt" || head_name == "else_alternative") {
        is_new_scope = true;
        def_area++;
    }

    if (head_name == "func") {
        std::string name = children[1]->get_data().get_content();
        current_func = name;
        if (symbol_table[""].count(name)) {
            std::cerr << RED << "Semantic Error [Line " << line_number << "]: "
                      << "Redeclaration of function '" << name << "'. Functions must have unique names globally.\n"
                      << WHITE << std::endl;
            std::cerr << "----------------------------------------------------------------" << std::endl;
            num_errors++;
        }
        symbol_table[""][name] = SymbolTableEntry(FUNC);
        symbol_table[name];
        Node<Symbol> *args_node = children[3];
        if (args_node->get_children()[0]->get_data().get_name() != "eps") {
            Node<Symbol> *arg_node = args_node->get_children()[0];
            std::string arg_name = arg_node->get_children()[0]->get_data().get_content();
            SymbolTableEntry arg_entry(VAR);
            arg_entry.set_name(arg_name);
            arg_entry.set_def_area(def_area);
            arg_entry.set_mut(false);
            if (arg_node->get_children()[1]->get_children()[0]->get_data().get_name() != "eps") {
                dfs(arg_node->get_children()[1]->get_children()[1]);
                arg_entry.set_stype(arg_node->get_children()[1]->get_children()[1]->get_data().get_stype());
            } else {
                arg_entry.set_stype(UNK);
            }
            symbol_table[current_func][arg_name] = arg_entry;
            symbol_table[""][current_func].add_to_parameters({arg_name, arg_entry.get_stype()});
            Node<Symbol> *args_tail_node = args_node->get_children()[1];
            while (args_tail_node->get_children()[0]->get_data().get_name() != "eps") {
                arg_node = args_tail_node->get_children()[1];
                arg_name = arg_node->get_children()[0]->get_data().get_content();
                SymbolTableEntry next_arg_entry(VAR);
                next_arg_entry.set_name(arg_name);
                next_arg_entry.set_def_area(def_area);
                next_arg_entry.set_mut(false);
                if (arg_node->get_children()[1]->get_children()[0]->get_data().get_name() != "eps") {
                    dfs(arg_node->get_children()[1]->get_children()[1]);
                    next_arg_entry.set_stype(arg_node->get_children()[1]->get_children()[1]->get_data().get_stype());
                } else {
                    next_arg_entry.set_stype(UNK);
                }
                symbol_table[current_func][arg_name] = next_arg_entry;
                symbol_table[""][current_func].add_to_parameters({arg_name, next_arg_entry.get_stype()});
                args_tail_node = args_tail_node->get_children()[2];
            }
        }
    }

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
        for (const auto &name: names) {
            if (symbol_table[current_func].count(name) && symbol_table[current_func][name].get_def_area() == def_area) {
                std::cerr << RED << "Semantic Error [Line " << line_number << "]: "
                          << "Identifier '" << name << "' is already defined in this scope." << WHITE << std::endl;
                std::cerr << "----------------------------------------------------------------" << std::endl;
                num_errors++;
            } else {
                SymbolTableEntry x(VAR);
                x.set_name(name);
                x.set_mut(children[1]->get_children()[0]->get_data().get_name() != "eps");
                x.set_def_area(def_area);
                symbol_table[current_func][name] = x;
            }
        }
    }

    for (auto child: children) {
        dfs(child);
    }

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
        if (children[3]->get_children()[0]->get_data().get_name() != "eps") {
            if (children_size == 3) {
                // We have (x, y, z, ...)

                if (children[3]->get_children().empty()) {
                    for (std::string name: names) {
                        symbol_table[current_func][name].set_stype(UNK);
                    }
                } else {
                    std::vector<semantic_type> tuple_type;
                    tuple_type.push_back(
                            children[3]->get_children()[1]->get_children()[1]->get_children()[0]->get_children()[0]->get_data().get_stype());
                    auto tmp_child = children[3]->get_children()[1]->get_children()[1]->get_children()[1];
                    while (tmp_child->get_children()[0]->get_data().get_name() != "eps") {
                        tuple_type.push_back(tmp_child->get_children()[1]->get_children()[0]->get_data().get_stype());
                        tmp_child = tmp_child->get_children()[2];
                    }

                    if (tuple_type.size() == names.size()) {
                        int idx = 0;
                        for (std::string name: names) {
                            symbol_table[current_func][name].set_stype(tuple_type[idx++]);
                        }
                    } else if (tuple_type.size() != names.size()) {
                        std::cerr << RED << "Semantic Error [Line " << line_number << "]: "
                                  << "Mismatch in tuple declaration for identifier '" << symbol.get_content() << "'.\n"
                                  << "  - Declared " << names.size() << " variable(s) but provided "
                                  << tuple_type.size() << " type(s).\n"
                                  << "  - Ensure the number of variables matches the number of types in the tuple.\n"
                                  << WHITE << std::endl;
                        std::cerr << "----------------------------------------------------------------" << std::endl;
                        num_errors++;
                    }
                }
            } else {
                // We have x

                if (children[3]->get_children().size() == 1 and
                    children[3]->get_children()[0]->get_data().get_name() == "eps") {
                    symbol_table[current_func][names[0]].set_stype(UNK);
                } else {
                    int children_size_type = children[3]->get_children()[1]->get_children().size();
                    if (children_size_type == 5) {
                        symbol_table[current_func][names[0]].set_stype(ARRAY);
                        symbol_table[current_func][names[0]].set_arr_len(
                                stoi(children[3]->get_children()[1]->get_children()[3]->get_children()[0]->get_data().get_content()));
                        symbol_table[current_func][names[0]].set_arr_type(
                                (children[3]->get_children()[1]->get_children()[1]->get_children()[0]->get_data().get_name() ==
                                 "T_Int") ? INT : BOOL);
                    } else if (children_size_type == 3) {
                        symbol_table[current_func][names[0]].set_stype(TUPLE);
                        std::vector<semantic_type> tuple_type;
                        tuple_type.push_back(
                                children[3]->get_children()[1]->get_children()[1]->get_children()[0]->get_children()[0]->get_data().get_stype());
                        auto tmp_child = children[3]->get_children()[1]->get_children()[1]->get_children()[1];
                        while (tmp_child->get_children()[0]->get_data().get_name() != "eps") {
                            tuple_type.push_back(
                                    tmp_child->get_children()[1]->get_children()[0]->get_data().get_stype());
                            tmp_child = tmp_child->get_children()[2];
                        }

                        for (semantic_type type: tuple_type) {
                            symbol_table[current_func][names[0]].add_to_tuple_types(type);
                        }
                    } else {
                        symbol_table[current_func][names[0]].set_stype(
                                children[3]->get_children()[1]->get_children()[0]->get_data().get_stype());
                    }
                }
            }
        }

        // handle <assign_opt>
        if (children[4]->get_children()[0]->get_data().get_name() != "eps") {
            if (children_size == 3) {
                std::vector<semantic_type> tuple_type = children[4]->get_children()[1]->get_data().get_tuple_types();
                if (tuple_type.size() != names.size()) {
                    std::cerr << RED
                              << "Semantic Error [Line " << line_number << "]: "
                              << "Mismatch in tuple assignment for identifier '" << symbol.get_content() << "'.\n"
                              << "  - Assigned " << tuple_type.size() << " value(s) but expected " << names.size()
                              << ".\n"
                              << "  - Ensure the number of assigned values matches the number of variables in the tuple.\n"
                              << WHITE << std::endl;
                    std::cerr << "----------------------------------------------------------------" << std::endl;
                    num_errors++;
                } else {
                    int idx = 0;
                    for (std::string name: names) {
                        if (tuple_type[idx] == UNK) {
                            std::cerr << RED << "Semantic Error [Line " << line_number << "]: "
                                      << "Unable to infer type for variable '" << name
                                      << "' from the assigned expression.\n"
                                      << "  - The expression has an unsupported or unknown type.\n"
                                      << "  - Ensure the expression is valid and has a type like int, bool, array, or tuple.\n"
                                      << WHITE << std::endl;
                            std::cerr << "----------------------------------------------------------------"
                                      << std::endl;
                            num_errors++;
                        } else {
                            symbol_table[current_func][name].set_stype(tuple_type[idx++]);
                        }
                    }
                }
            } else {
                exp_type exp_t = children[4]->get_children()[1]->get_data().get_exp_type();
                semantic_type s_type = exp_t_to_semantic_type(exp_t);
                if (s_type == UNK) {
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: "
                              << "Unable to infer type for variable '" << names[0]
                              << "' from the assigned expression.\n"
                              << "  - The expression has an unsupported or unknown type.\n"
                              << "  - Ensure the expression is valid and has a type like int, bool, array, or tuple.\n"
                              << WHITE << std::endl;
                    std::cerr << "----------------------------------------------------------------" << std::endl;
                    num_errors++;
                } else {
                    symbol_table[current_func][names[0]].set_stype(s_type);
                    if (s_type == TUPLE) {
                        symbol_table[current_func][names[0]].add_to_tuple_types(
                                children[4]->get_children()[1]->get_data().get_tuple_types());
                    }
                }
            }
        }

        // value
        Node<Symbol> *assign_opt_node = children[4];
        if (assign_opt_node->get_children()[0]->get_data().get_name() != "eps") {
            if (names.size() == 1) {
                std::string exp_val = assign_opt_node->get_children()[1]->get_data().get_val();
                if (!exp_val.empty()) symbol_table[current_func][names[0]].set_val(exp_val);

            }
        }
    } else if (head_name == "func") {
        SymbolTableEntry &entry = symbol_table[""][current_func];
        if (children[5]->get_children()[0]->get_data().get_name() != "eps") {
            entry.set_stype(children[5]->get_children()[1]->get_data().get_stype());
        }
        Node<Symbol> *return_stmt_node = children[8];
        if (return_stmt_node->get_children()[0]->get_data().get_name() == "eps") {
            if (entry.get_stype() == UNK) entry.set_stype(VOID);
            else if (entry.get_stype() != VOID) {
                std::cerr << RED << "Semantic Error [Line " << line_number << "]: " << "Function '" << current_func
                          << "' declared to return type '" << semantic_type_to_string[entry.get_stype()]
                          << "' but has no return statement.\n"
                          << "  - Ensure the function returns a value of the declared type or change the return type to 'void'.\n"
                          << WHITE << std::endl;
                std::cerr << "----------------------------------------------------------------" << std::endl;
                num_errors++;
            }
        } else {
            semantic_type stp = exp_t_to_semantic_type(
                    return_stmt_node->get_children()[1]->get_data().get_exp_type());
            if (entry.get_stype() == UNK) {
                if (stp == UNK) {
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: "
                              << "Unable to infer return type for function '" << current_func
                              << "' from the return expression.\n"
                              << "  - The expression has an unsupported or unknown type.\n"
                              << "  - Ensure the expression is valid and has a type like int, bool, array, or tuple.\n"
                              << WHITE << std::endl;
                    std::cerr << "----------------------------------------------------------------" << std::endl;
                    num_errors++;
                } else {
                    entry.set_stype(stp);
                }
            } else {
                if (entry.get_stype() != stp) {
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: " << "Function '"
                              << current_func
                              << "' declared to return type '" << semantic_type_to_string[entry.get_stype()]
                              << "' but has a return statement of type '" << semantic_type_to_string[stp] << "'.\n"
                              << "  - Ensure the function returns a value of the declared type.\n" << WHITE
                              << std::endl;
                    std::cerr << "----------------------------------------------------------------" << std::endl;
                    num_errors++;
                }
                if (entry.get_stype() == TUPLE && entry.get_tuple_types().size() !=
                                                  return_stmt_node->get_children()[1]->get_data().get_tuple_types().size()) {
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: " << "Function '"
                              << current_func
                              << "' declared to return a tuple of type '"
                              << semantic_type_to_string[entry.get_stype()]
                              << "' but the return statement has a different number of elements.\n"
                              << "  - Ensure the number of elements in the returned tuple matches the declared type.\n"
                              << WHITE << std::endl;
                    std::cerr << "----------------------------------------------------------------" << std::endl;
                    num_errors++;
                }
            }
        }
    } else if (head_name == "T_Int") {
        symbol.set_stype(INT);
    } else if (head_name == "T_Bool") {
        symbol.set_stype(BOOL);
    } else if (head_name == "T_True" || head_name == "T_False") {
        symbol.set_stype(BOOL);
    } else if (head_name == "T_Id" and node->get_parent()->get_data().get_name() != "pattern" and
               node->get_parent()->get_data().get_name() != "func" and
               node->get_parent()->get_data().get_name() != "id_ls" and
               node->get_parent()->get_data().get_name() != "id_ls_tail" and
               node->get_parent()->get_data().get_name() != "arg") {
        if (!current_func.empty() && !symbol_table[current_func].count(symbol.get_content())) {
            if (!symbol_table[""].count(symbol.get_content())) {
                std::cerr << RED << "Semantic Error [Line " << line_number << "]: "
                          << "Use of undeclared identifier '"
                          << symbol.get_content() << "'.\n"
                          << "  - The identifier must be declared before it is used.\n"
                          << "  - Check for missing declarations or typos in the name.\n" << WHITE << std::endl;
                std::cerr << "----------------------------------------------------------------" << std::endl;
                num_errors++;
            }
        }
    } else if (head_name == "exp") {
        symbol.set_exp_type(children[0]->get_data().get_exp_type());
        symbol.add_to_tuple_types(children[0]->get_data().get_tuple_types());
        symbol.set_val(children[0]->get_data().get_val());
    } else if (head_name == "log_exp" or head_name == "rel_exp" or head_name == "eq_exp" or
               head_name == "cmp_exp") {
        Node<Symbol> *tail_node = children[1];
        if (tail_node->get_children()[0]->get_data().get_name() != "eps") {
            symbol.set_exp_type(TYPE_BOOL);
        } else {
            symbol.set_exp_type(children[0]->get_data().get_exp_type());
            symbol.add_to_tuple_types(children[0]->get_data().get_tuple_types());
            symbol.set_val(children[0]->get_data().get_val());
        }
    } else if (head_name == "arith_exp" or head_name == "arith_term") {
        if (children[1]->get_children()[0]->get_data().get_name() != "eps") {
            symbol.set_exp_type(TYPE_INT);
        } else {
            symbol.set_exp_type(children[0]->get_data().get_exp_type());
            symbol.add_to_tuple_types(children[0]->get_data().get_tuple_types());
        }

        // value
        std::string current_val_str = children[0]->get_data().get_val();

        Node<Symbol> *tail_node = children[1];
        while (tail_node->get_children()[0]->get_data().get_name() != "eps") {
            std::string op_name = tail_node->get_children()[0]->get_data().get_name();
            std::string right_val_str = tail_node->get_children()[1]->get_data().get_val();

            // Only perform calculation if both operands are constant.
            if (!current_val_str.empty() && !right_val_str.empty()) {
                long long left_val = std::stoll(current_val_str);
                long long right_val = std::stoll(right_val_str);
                long long result = 0;

                if (op_name == "T_AOp_Trust") result = left_val + right_val;
                else if (op_name == "T_AOp_MN") result = left_val - right_val;
                else if (op_name == "T_AOp_ML") result = left_val * right_val;
                else if (op_name == "T_AOp_DV") result = (right_val != 0) ? left_val / right_val : 0;
                else if (op_name == "T_AOp_RM") result = (right_val != 0) ? left_val % right_val : 0;

                current_val_str = std::to_string(result);
            } else {
                current_val_str = "";
            }
            tail_node = tail_node->get_children()[2];
        }
        symbol.set_val(current_val_str);
    } else if (head_name == "arith_factor") {
        if (children[0]->get_data().get_name() == "T_Hexadecimal" or
            children[0]->get_data().get_name() == "T_Decimal") {
            symbol.set_exp_type(TYPE_INT);
            symbol.set_val(children[0]->get_data().get_content());
        } else if (children[0]->get_data().get_name() == "T_String") {
            symbol.set_exp_type(TYPE_STRING);
        } else if (children[0]->get_data().get_name() == "T_True" ||
                   children[0]->get_data().get_name() == "T_False") {
            symbol.set_exp_type(TYPE_BOOL);
            if (children[0]->get_data().get_name() == "T_True") {
                symbol.set_val("true");
            } else {
                symbol.set_val("false");
            }
        } else if (children[0]->get_data().get_name() == "T_LOp_NOT") {
            Node<Symbol> *operand_node = children[1];
            exp_type operand_type = operand_node->get_data().get_exp_type();
            if (operand_type != TYPE_BOOL) {
                std::cerr << RED << "Semantic Error [Line " << line_number << "]: "
                          << "Logical NOT operator '!' cannot be applied to a non-boolean type.\n"
                          << "  - Expected operand of type 'bool' but got '" << exp_t_to_string(operand_type)
                          << "'.\n"
                          << WHITE << std::endl;
                std::cerr << "----------------------------------------------------------------" << std::endl;
                num_errors++;
            }
            symbol.set_exp_type(TYPE_BOOL);
            std::string operand_val = operand_node->get_data().get_val();
            if (operand_val == "true") {
                symbol.set_val("false");
            } else if (operand_val == "false") {
                symbol.set_val("true");
            }
        } else if (children[0]->get_data().get_name() == "T_Id") {
            auto fac_id_opt = children[1]->get_children()[0];
            std::string id_name = children[0]->get_data().get_content();

            // value
            if (fac_id_opt->get_data().get_name() == "eps") {
                // It's a variable
                if (symbol_table[current_func].count(id_name)) {
                    symbol.set_val(symbol_table[current_func][id_name].get_val());
                }
            }

            if (fac_id_opt->get_data().get_name() == "T_LP") {
                std::vector<std::pair<std::string, semantic_type>> &expected_params = symbol_table[""][id_name].get_parameters();
                std::vector<semantic_type> provided_arg_types;

                Node<Symbol> *exp_ls_call_node = fac_id_opt->get_parent()->get_children()[1];

                if (exp_ls_call_node->get_children().empty() ||
                    exp_ls_call_node->get_children()[0]->get_data().get_name() == "eps") {
                    // Function called with no arguments
                } else {
                    Node<Symbol> *exp_ls_node = exp_ls_call_node->get_children()[0];
                    if (!exp_ls_node->get_children().empty() &&
                        exp_ls_node->get_children()[0]->get_data().get_name() != "eps") {
                        Node<Symbol> *current_arg_item = exp_ls_node->get_children()[0];
                        Node<Symbol> *current_exp = current_arg_item->get_children()[0];
                        provided_arg_types.push_back(exp_t_to_semantic_type(current_exp->get_data().get_exp_type()));

                        Node<Symbol> *exp_ls_tail_node = exp_ls_node->get_children()[1];
                        while (!exp_ls_tail_node->get_children().empty() &&
                               exp_ls_tail_node->get_children()[0]->get_data().get_name() != "eps") {
                            current_arg_item = exp_ls_tail_node->get_children()[1];
                            current_exp = current_arg_item->get_children()[0];
                            provided_arg_types.push_back(
                                    exp_t_to_semantic_type(current_exp->get_data().get_exp_type()));
                            exp_ls_tail_node = exp_ls_tail_node->get_children()[2];
                        }
                    }
                }

                if (expected_params.size() != provided_arg_types.size()) {
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: "
                              << "Incorrect number of arguments in call to function '" << id_name << "'.\n"
                              << "  - Expected " << expected_params.size() << " argument(s), but got "
                              << provided_arg_types.size() << ".\n"
                              << WHITE << std::endl;
                    std::cerr << "----------------------------------------------------------------" << std::endl;
                    num_errors++;
                } else {
                    for (size_t i = 0; i < expected_params.size(); ++i) {
                        if (expected_params[i].second == UNK) {
                            if (provided_arg_types[i] != UNK) {
                                symbol_table[""][id_name].get_parameters()[i].second = provided_arg_types[i];
                            }
                        } else if (provided_arg_types[i] != UNK &&
                                   expected_params[i].second != provided_arg_types[i]) {
                            std::cerr << RED << "Semantic Error [Line " << line_number << "]: "
                                      << "Type mismatch in arguments of call to function '" << id_name
                                      << "'.\n  - Expected argument " << i + 1 << " to be of type '"
                                      << semantic_type_to_string[expected_params[i].second]
                                      << "' but got type '" << semantic_type_to_string[provided_arg_types[i]] << "'.\n"
                                      << WHITE << std::endl;
                            std::cerr << "----------------------------------------------------------------"
                                      << std::endl;
                            num_errors++;
                        }
                    }
                }

                semantic_type func_return_type = symbol_table[""][id_name].get_stype();
                exp_type call_exp_type;
                switch (func_return_type) {
                    case INT:
                        call_exp_type = TYPE_INT;
                        break;
                    case BOOL:
                        call_exp_type = TYPE_BOOL;
                        break;
                    case ARRAY:
                        call_exp_type = TYPE_ARRAY;
                        break;
                    case TUPLE:
                        call_exp_type = TYPE_TUPLE;
                        break;
                    case VOID:
                        call_exp_type = TYPE_VOID;
                        break;
                    default:
                        call_exp_type = TYPE_UNKNOWN;
                        break;
                }
                symbol.set_exp_type(call_exp_type);
            } else if (fac_id_opt->get_data().get_name() == "T_LB") {
                // Check if the identifier is declared as an array
                if (!symbol_table[current_func].count(id_name) ||
                    symbol_table[current_func][id_name].get_stype() != ARRAY) {
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: Identifier '" << id_name
                              << "' is not an array and cannot be indexed.\n" << WHITE << std::endl;
                    std::cerr << "----------------------------------------------------------------" << std::endl;
                    num_errors++;
                    symbol.set_exp_type(TYPE_UNKNOWN);
                } else {
                    // check if the index expression is of type 'i32' and not negative
                    Node<Symbol> *index_exp_node = children[1]->get_children()[1];
                    if (index_exp_node->get_data().get_exp_type() != TYPE_INT) {
                        std::cerr << RED << "Semantic Error [Line " << line_number << "]: "
                                  << "Array index for '" << id_name << "' must be of type 'i32'.\n"
                                  << "  - The provided index expression is not an integer, it is "
                                  << exp_t_to_string(index_exp_node->get_data().get_exp_type()) << ".\n" << WHITE
                                  << std::endl;
                        std::cerr << "----------------------------------------------------------------"
                                  << std::endl;
                        num_errors++;
                    }

                    std::string index_val_str = index_exp_node->get_data().get_val();
                    if (!index_val_str.empty()) {
                        long long index_val = std::stoll(index_val_str);
                        if (index_val < 0) {
                            std::cerr << RED << "Semantic Error [Line " << line_number << "]: "
                                      << "Array index cannot be negative. Got: " << index_val << " for array '"
                                      << id_name << "'.\n" << WHITE << std::endl;
                            std::cerr << "----------------------------------------------------------------"
                                      << std::endl;
                            num_errors++;
                        }
                    }

                    // type of the array's elements
                    semantic_type arr_elem_type = symbol_table[current_func][id_name].get_arr_type();
                    exp_type call_exp_type;
                    switch (arr_elem_type) {
                        case INT:
                            call_exp_type = TYPE_INT;
                            break;
                        case BOOL:
                            call_exp_type = TYPE_BOOL;
                            break;
                        default:
                            call_exp_type = TYPE_UNKNOWN;
                            break;
                    }
                    symbol.set_exp_type(call_exp_type);
                }
            } else {
                if (symbol_table[current_func].count(id_name)) {
                    semantic_type var_stype = symbol_table[current_func][id_name].get_stype();
                    exp_type call_exp_type;
                    switch (var_stype) {
                        case INT:
                            call_exp_type = TYPE_INT;
                            break;
                        case BOOL:
                            call_exp_type = TYPE_BOOL;
                            break;
                        case ARRAY:
                            call_exp_type = TYPE_ARRAY;
                            break;
                        case TUPLE:
                            call_exp_type = TYPE_TUPLE;
                            break;
                        case VOID:
                            call_exp_type = TYPE_VOID;
                            break;
                        default:
                            call_exp_type = TYPE_UNKNOWN;
                            break;
                    }
                    symbol.set_exp_type(call_exp_type);
                    if (var_stype == TUPLE) {
                        symbol.add_to_tuple_types(symbol_table[current_func][id_name].get_tuple_types());
                    }
                } else {
                    symbol.set_exp_type(TYPE_UNKNOWN);
                }
            }
        } else if (children[0]->get_data().get_name() == "T_LP") {
            // value
            if (children[1]->get_children()[1]->get_children()[0]->get_data().get_name() == "T_RP")
                symbol.set_val(children[1]->get_children()[0]->get_data().get_val());

            if (children[1]->get_children()[1]->get_children()[0]->get_data().get_name() == "T_RP") {
                // parentheses
                symbol.set_exp_type(children[1]->get_children()[0]->get_data().get_exp_type());
            } else if (children[1]->get_children()[1]->get_children()[0]->get_data().get_name() == "T_Comma") {
                // tuple
                symbol.set_exp_type(TYPE_TUPLE);
                std::vector<semantic_type> tuple_type;
                tuple_type.push_back(
                        exp_t_to_semantic_type(children[1]->get_children()[0]->get_data().get_exp_type()));
                auto tmp_node = children[1]->get_children()[1]->get_children()[1];
                if (tmp_node->get_data().get_name() != "eps") {
                    tuple_type.push_back(
                            exp_t_to_semantic_type(tmp_node->get_children()[0]->get_data().get_exp_type()));
                    tmp_node = tmp_node->get_children()[1];
                    while (tmp_node->get_children()[0]->get_data().get_name() != "eps") {
                        tuple_type.push_back(
                                exp_t_to_semantic_type(tmp_node->get_children()[1]->get_data().get_exp_type()));
                        tmp_node = tmp_node->get_children()[2];
                    }
                }
                symbol.add_to_tuple_types(tuple_type);
            }
        } else if (children[0]->get_data().get_name() == "T_LB") {
            // array literal
            symbol.set_exp_type(TYPE_ARRAY);
        }
    } else if (head_name == "stmt_after_id") {
        std::string name = node->get_parent()->get_children()[0]->get_data().get_content();

        // Handles simple assignment: x = 2;
        if (children[0]->get_data().get_name() == "T_Assign") {
            if (symbol_table[current_func].count(name)) {
                if (!symbol_table[current_func][name].get_mut()) {
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: "
                              << "Cannot assign to immutable variable '" << name << "'.\n"
                              << "  - This variable was not declared as mutable (e.g., 'let mut " << name << "').\n"
                              << "  - To allow mutation, declare the variable with the 'mut' keyword.\n" << WHITE
                              << std::endl;
                    std::cerr << "----------------------------------------------------------------" << std::endl;
                    num_errors++;
                }
                semantic_type stp = exp_t_to_semantic_type(children[1]->get_data().get_exp_type());
                if (symbol_table[current_func][name].get_stype() == UNK) {
                    symbol_table[current_func][name].set_stype(stp);
                } else if (stp != UNK && stp != symbol_table[current_func][name].get_stype()) {
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: "
                              << "Type mismatch for variable '" << name << "'.\n" << "  - Expected type '"
                              << semantic_type_to_string[symbol_table[current_func][name].get_stype()]
                              << "' but got type '" << semantic_type_to_string[stp] << "'.\n"
                              << "  - Ensure the assigned value matches the variable's declared type.\n" << WHITE
                              << std::endl;
                    std::cerr << "----------------------------------------------------------------" << std::endl;
                    num_errors++;
                }

                // value
                std::string exp_val = children[1]->get_data().get_val();
                symbol_table[current_func][name].set_val(exp_val);
            }
            // Handles array element assignment: x[y] = 2;
        } else if (children[0]->get_data().get_name() == "T_LB") {
            // Check 1: Is the variable an array and mutable?
            if (!symbol_table[current_func].count(name) || symbol_table[current_func][name].get_stype() != ARRAY) {
                std::cerr << RED << "Semantic Error [Line " << line_number << "]: Identifier '" << name
                          << "' is not an array and cannot be indexed.\n" << WHITE << std::endl;
                std::cerr << "----------------------------------------------------------------" << std::endl;
                num_errors++;
            } else if (!symbol_table[current_func][name].get_mut()) {
                std::cerr << RED << "Semantic Error [Line " << line_number << "]: "
                          << "Cannot assign to an element of immutable array '" << name << "'.\n"
                          << "  - To allow mutation, declare the array with 'mut'.\n" << WHITE << std::endl;
                std::cerr << "----------------------------------------------------------------" << std::endl;
                num_errors++;
            } else {
                // Check 2: Is the index type i32 and not negative?
                Node<Symbol> *index_exp_node = children[1];
                if (index_exp_node->get_data().get_exp_type() != TYPE_INT) {
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: "
                              << "Array index for '" << name << "' must be of type 'i32'.\n"
                              << "  - The provided index has type "
                              << exp_t_to_string(index_exp_node->get_data().get_exp_type()) << ".\n" << WHITE
                              << std::endl;
                    std::cerr << "----------------------------------------------------------------" << std::endl;
                    num_errors++;
                }

                std::string index_val_str = index_exp_node->get_data().get_val();
                if (!index_val_str.empty()) {
                    long long index_val = std::stoll(index_val_str);
                    if (index_val < 0) {
                        std::cerr << RED << "Semantic Error [Line " << line_number << "]: "
                                  << "Array index for assignment cannot be negative. Got: " << index_val
                                  << " for array '" << name << "'.\n" << WHITE << std::endl;
                        std::cerr << "----------------------------------------------------------------"
                                  << std::endl;
                        num_errors++;
                    }
                }

                // Check 3: Does the assigned value's type match the array's element type?
                semantic_type array_element_type = symbol_table[current_func][name].get_arr_type();
                semantic_type assigned_value_type = exp_t_to_semantic_type(children[4]->get_data().get_exp_type());

                if (assigned_value_type != UNK && array_element_type != assigned_value_type) {
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: "
                              << "Type mismatch in assignment to array '" << name << "'.\n"
                              << "  - Array elements have type '" << semantic_type_to_string[array_element_type]
                              << "' but assigned value has type '" << semantic_type_to_string[assigned_value_type]
                              << "'.\n" << WHITE << std::endl;
                    std::cerr << "----------------------------------------------------------------" << std::endl;
                    num_errors++;
                }
            }
        }
    } else if (head_name == "if_stmt") {
        if (children[1]->get_data().get_exp_type() != TYPE_BOOL) {
            std::cerr << RED << "Semantic Error [Line " << line_number << "]: "
                      << "Condition in 'if' statement must be of type 'bool'.\n"
                      << "  - The expression used in the condition is not a boolean expression.\n"
                      << "  - Ensure the condition evaluates to a boolean value (true or false).\n" << WHITE
                      << std::endl;
            std::cerr << "----------------------------------------------------------------" << std::endl;
            num_errors++;
        }
    } else if (head_name == "log_exp_tail" || head_name == "rel_exp_tail") {
        if (children[0]->get_data().get_name() != "eps") {
            semantic_type stp_l = exp_t_to_semantic_type(
                    node->get_parent()->get_children()[0]->get_data().get_exp_type());
            semantic_type stp_r = exp_t_to_semantic_type(children[1]->get_data().get_exp_type());

            // error for logical operate, operand is boolean
            if (stp_l != BOOL) {
                std::cerr << RED
                          << "Semantic Error [Line " << line_number << "]: "
                          << "Left operand of logical operator must be of type 'bool'.\n"
                          << "  - The left operand has type '" << semantic_type_to_string[stp_l]
                          << "' which is not boolean.\n"
                          << "  - Ensure the left operand is a boolean expression (true or false).\n"
                          << WHITE << std::endl;
                std::cerr << "----------------------------------------------------------------" << std::endl;
                num_errors++;
            }

            if (stp_r != BOOL) {
                std::cerr << RED
                          << "Semantic Error [Line " << line_number << "]: "
                          << "Right operand of logical operator must be of type 'bool'.\n"
                          << "  - The right operand has type '" << semantic_type_to_string[stp_r]
                          << "' which is not boolean.\n"
                          << "  - Ensure the right operand is a boolean expression (true or false).\n"
                          << WHITE << std::endl;
                std::cerr << "----------------------------------------------------------------" << std::endl;
                num_errors++;
            }

            // value
            std::string left_val_str = node->get_parent()->get_children()[0]->get_data().get_val();
            std::string right_val_str = children[1]->get_data().get_val();

            if ((left_val_str == "true" || left_val_str == "false") &&
                (right_val_str == "true" || right_val_str == "false")) {

                bool left_bool = (left_val_str == "true");
                bool right_bool = (right_val_str == "true");
                bool result = false;

                if (head_name == "log_exp_tail") {
                    result = left_bool || right_bool;
                } else {
                    result = left_bool && right_bool;
                }

                node->get_parent()->get_data().set_val(result ? "true" : "false");
            }
        }
    } else if (head_name == "eq_exp_tail") {
        if (children[0]->get_data().get_name() != "eps") {
            semantic_type stp_l = exp_t_to_semantic_type(
                    node->get_parent()->get_children()[0]->get_data().get_exp_type());
            semantic_type stp_r = exp_t_to_semantic_type(children[1]->get_data().get_exp_type());

            if (stp_l != stp_r) {
                std::cerr << RED
                          << "Semantic Error [Line " << line_number << "]: "
                          << "Operands of equality operator must have the same type.\n"
                          << "  - Left operand has type '" << semantic_type_to_string[stp_l]
                          << "' and right operand has type '" << semantic_type_to_string[stp_r] << "'.\n"
                          << "  - Ensure both operands are of the same type for comparison.\n"
                          << WHITE << std::endl;
                std::cerr << "----------------------------------------------------------------" << std::endl;
                num_errors++;
            }
        }

        // value
        if (children[0]->get_data().get_name() != "eps") {
            std::string left_val_str = node->get_parent()->get_children()[0]->get_data().get_val();
            std::string right_val_str = children[1]->get_data().get_val();

            if (!left_val_str.empty() && !right_val_str.empty()) {
                std::string op_name = children[0]->get_data().get_name();
                bool result = false;

                if (op_name == "T_ROp_E") result = (left_val_str == right_val_str);
                else if (op_name == "T_ROp_NE") result = (left_val_str != right_val_str);

                node->get_parent()->get_data().set_val(result ? "true" : "false");
            }
        }
    } else if (head_name == "cmp_exp_suf") {
        if (children[0]->get_data().get_name() != "eps") {
            semantic_type stp_l = exp_t_to_semantic_type(
                    node->get_parent()->get_children()[0]->get_data().get_exp_type());
            semantic_type stp_r = exp_t_to_semantic_type(children[1]->get_data().get_exp_type());

            if (stp_l != INT) {
                std::cerr << RED
                          << "Semantic Error [Line " << line_number << "]: "
                          << "Left operand of comparison operator must be of type 'int'.\n"
                          << "  - The left operand has type '" << semantic_type_to_string[stp_l]
                          << "' which is not integer.\n"
                          << "  - Ensure the left operand is an integer expression.\n"
                          << WHITE << std::endl;
                std::cerr << "----------------------------------------------------------------" << std::endl;
                num_errors++;
            }

            if (stp_r != INT) {
                std::cerr << RED
                          << "Semantic Error [Line " << line_number << "]: "
                          << "Right operand of comparison operator must be of type 'int'.\n"
                          << "  - The right operand has type '" << semantic_type_to_string[stp_r]
                          << "' which is not integer.\n"
                          << "  - Ensure the right operand is an integer expression.\n"
                          << WHITE << std::endl;
                std::cerr << "----------------------------------------------------------------" << std::endl;
                num_errors++;
            }

            // value
            std::string left_val_str = node->get_parent()->get_children()[0]->get_data().get_val();
            std::string right_val_str = children[1]->get_data().get_val();

            if (!left_val_str.empty() && !right_val_str.empty()) {
                long long left_val = std::stoll(left_val_str);
                long long right_val = std::stoll(right_val_str);
                std::string op_name = children[0]->get_children()[0]->get_data().get_name();
                bool result = false;

                if (op_name == "T_ROp_L") result = left_val < right_val;
                else if (op_name == "T_ROp_LE") result = left_val <= right_val;
                else if (op_name == "T_ROp_G") result = left_val > right_val;
                else if (op_name == "T_ROp_GE") result = left_val >= right_val;
                node->get_parent()->get_data().set_val(result ? "true" : "false");
            }
        }
    } else if (head_name == "arith_exp_tail" || head_name == "arith_term_tail") {
        if (children[0]->get_data().get_name() != "eps") {
            semantic_type stp_l = exp_t_to_semantic_type(
                    node->get_parent()->get_children()[0]->get_data().get_exp_type());
            semantic_type stp_r = exp_t_to_semantic_type(children[1]->get_data().get_exp_type());

            if (stp_l != INT) {
                std::cerr << RED
                          << "Semantic Error [Line " << line_number << "]: "
                          << "Left operand of arithmetic operator must be of type 'int'.\n"
                          << "  - The left operand has type '" << semantic_type_to_string[stp_l]
                          << "' which is not integer.\n"
                          << "  - Ensure the left operand is an integer expression.\n"
                          << WHITE << std::endl;
                std::cerr << "----------------------------------------------------------------" << std::endl;
                num_errors++;
            }

            if (stp_r != INT) {
                std::cerr << RED
                          << "Semantic Error [Line " << line_number << "]: "
                          << "Right operand of arithmetic operator must be of type 'int'.\n"
                          << "  - The right operand has type '" << semantic_type_to_string[stp_r]
                          << "' which is not integer.\n"
                          << "  - Ensure the right operand is an integer expression.\n"
                          << WHITE << std::endl;
                std::cerr << "----------------------------------------------------------------" << std::endl;
                num_errors++;
            }
        }
    } else if (head_name == "type") {
        if (children[0]->get_data().get_name() == "T_Int") symbol.set_stype(INT);
        else if (children[0]->get_data().get_name() == "T_Bool") symbol.set_stype(BOOL);
        else if (children[0]->get_data().get_name() == "T_LP") symbol.set_stype(TUPLE);
        else if (children[0]->get_data().get_name() == "T_LB") symbol.set_stype(ARRAY);
    } else if (head_name == "opt_type") {
        if (children[0]->get_data().get_name() == "T_Int") symbol.set_stype(INT);
        else if (children[0]->get_data().get_name() == "T_Bool") symbol.set_stype(BOOL);
        else if (children[0]->get_data().get_name() == "T_LP") symbol.set_stype(TUPLE);
        else if (children[0]->get_data().get_name() == "T_LB") symbol.set_stype(ARRAY);
        else if (children[0]->get_data().get_name() == "eps") symbol.set_stype(VOID);
    }

    if (is_new_scope) {
        std::vector<std::string> keys_to_erase;
        if (!current_func.empty()) {
            for (const auto &[key, val]: symbol_table[current_func]) {
                if (val.get_def_area() == def_area) {
                    keys_to_erase.push_back(key);
                }
            }
            for (const auto &key: keys_to_erase) {
                symbol_table[current_func].erase(key);
            }
        }

        if (head_name == "func") {
            current_func = "";
        }
        def_area--;
    }
}

void SemanticAnalyzer::check_for_main_function() {

    if (symbol_table[""].count("main") == 0) {
        std::cerr << RED
                  << "Semantic Error: No 'main' function found.\n"
                  << "  - Every program must have a 'main' function as the entry point.\n"
                  << WHITE << std::endl;
        std::cerr << "----------------------------------------------------------------" << std::endl;
        num_errors++;
    }
}

void SemanticAnalyzer::analyze() {
    if (parse_tree.get_root() != nullptr) {
        dfs(parse_tree.get_root());
    }

    check_for_main_function();

    if (num_errors == 0) {
        std::cout << GREEN << "Semantic analysis completed with no errors." << WHITE << std::endl;

        out.open(out_address);
        if (!out.is_open()) {
            std::cerr << RED << "File Error: Couldn't open semantic output file '" << out_address << "'" << WHITE
                      << std::endl;
        } else {
            std::fill(has_par, has_par + 200, false);
            write_annotated_tree(parse_tree.get_root());
            out.close();
            std::cout << "Annotated syntax tree written to " << out_address << std::endl;
        }
    } else {
        std::cout << RED << "Semantic analysis completed with " << num_errors
                  << " error(s). Output file will not be generated." << WHITE << std::endl;
    }
}

void SemanticAnalyzer::write_annotated_tree(Node<Symbol> *node, int num, bool last) {
    if (!node) return;
    Symbol &var = node->get_data();

    for (int i = 0; i < num * TAB - TAB; i++) {
        if (has_par[i])
            out << "│";
        else
            out << " ";
    }

    if (num > 0)
        out << (last ? "└── " : "├── ");

    out << var;

    std::vector<std::string> annotations;
    exp_type et = var.get_exp_type();

    if (et != TYPE_UNKNOWN && et != TYPE_VOID)
        annotations.push_back("type: " + exp_t_to_string(et));

    std::string val = var.get_val();
    if (!val.empty())
        annotations.push_back("val: '" + val + "'");

    std::string content = var.get_content();
    if (var.get_type() == TERMINAL && !content.empty() && val != content)
        annotations.push_back("content: '" + content + "'");

    if (!annotations.empty()) {
        out << "  [ ";
        for (size_t i = 0; i < annotations.size(); ++i)
            out << annotations[i] << (i == annotations.size() - 1 ? "" : ", ");
        out << " ]";
    }

    out << std::endl;

    has_par[num * TAB] = true;
    std::deque<Node<Symbol> *> children = node->get_children();
    for (auto it = children.begin(); it != children.end(); ++it) {
        bool is_last_child = (std::next(it) == children.end());
        if (is_last_child)
            has_par[num * TAB] = false;
        write_annotated_tree(*it, num + 1, is_last_child);
    }
}

SemanticAnalyzer::SemanticAnalyzer(Tree<Symbol>
                                   _parse_tree, std::string
                                   output_file_name) {
    parse_tree = _parse_tree;
    out_address = std::move(output_file_name);
    def_area = 0;
    current_func = "";
    num_errors = 0;
}