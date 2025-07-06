#include "semantic_analyzer.h"
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <deque>
#include <sstream>
#include <system_error>

std::vector<std::string> split(std::string s, std::vector<char> chs) {
    int n = s.size();
    std::vector<std::string> sp;
    std::string tmp = "";
    for (int i = 0; i < n; i++) {
        bool f = 0;
        for (char ch: chs) {
            if (s[i] == ch) {
                f = 1;
                break;
            }
        }
        if (!f) {
            tmp += s[i];
        } else if (tmp != "") {
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
    int len = exp.size();
    int j = 1;
    int64_t result = 0;

    if (!sp.empty()) {
        try {
            result = std::stoll(sp[0]) * val;
        } catch (const std::out_of_range &oor) {
            return "ERROR_OVERFLOW";
        }
    }

    if (chs.size() == 2 && (chs[0] == '+' || chs[0] == '-')) {
        for (int i = 0; i < len; i++) {
            if (exp[i] == '+' && j < sp.size()) {
                try { result += std::stoll(sp[j++]); } catch (const std::out_of_range &oor) { return "ERROR_OVERFLOW"; }
            } else if (exp[i] == '-' && j < sp.size()) {
                try { result -= std::stoll(sp[j++]); } catch (const std::out_of_range &oor) { return "ERROR_OVERFLOW"; }
            }
        }
    } else if (chs.size() == 3 && (chs[0] == '*' || chs[0] == '/' || chs[0] == '%')) {
        for (int i = 0; i < len; i++) {
            if (exp[i] == '*' && j < sp.size()) {
                try { result *= std::stoll(sp[j++]); } catch (const std::out_of_range &oor) { return "ERROR_OVERFLOW"; }
            } else if (exp[i] == '/' && j < sp.size()) {
                long long divisor = 0;
                try { divisor = std::stoll(sp[j]); } catch (const std::out_of_range &oor) { return "ERROR_OVERFLOW"; }
                if (divisor != 0) {
                    result /= divisor;
                } else {
                    return "ERROR_DIVISION_BY_ZERO";
                }
                j++;
            } else if (exp[i] == '%' && j < sp.size()) {
                long long divisor = 0;
                try { divisor = std::stoll(sp[j]); } catch (const std::out_of_range &oor) { return "ERROR_OVERFLOW"; }
                if (divisor != 0) {
                    result %= divisor;
                } else {
                    return "ERROR_MODULO_BY_ZERO";
                }
                j++;
            }
        }
    }

    return std::to_string(result);
}

std::map<std::string, SymbolTableEntry> global_function_table;

SymbolTableEntry *find_variable_in_scope(std::map<std::string, std::map<std::string, SymbolTableEntry>> &symbol_table,
                                         const std::string &current_func, const std::string &var_name,
                                         int current_def_area) {
    for (int d = current_def_area; d >= 0; --d) {
        if (symbol_table.count(current_func) && symbol_table[current_func].count(var_name)) {
            if (symbol_table[current_func][var_name].get_def_area() == d) {
                return &symbol_table[current_func][var_name];
            }
        }
    }
    return nullptr;
}

SemanticAnalyzer::SemanticAnalyzer(Tree<Symbol> _parse_tree, std::string output_file_name) {
    parse_tree = _parse_tree;
    out_address = output_file_name;
    def_area = 0;
    current_func = "";
    num_errors = 0;
}

void SemanticAnalyzer::dfs(Node<Symbol> *node) {
    if (!node) {
        return;
    }

    std::deque<Node<Symbol> *> children = node->get_children();
    Symbol &symbol = node->get_data();
    std::string head_name = symbol.get_name();
    int line_number = symbol.get_line_number();

    bool scope_opened = false;
    if (head_name == "func" || head_name == "if_stmt" || head_name == "loop_stmt" || head_name == "T_LC") {
        def_area++;
        scope_opened = true;
    }

    for (auto child: children) {
        std::string child_name = child->get_data().get_name();

        if (head_name == "dec") {
            if (child_name == "id") {
                child->get_data().set_stype(children[0]->get_data().get_stype());
            } else if (child_name == "init_dec") {
                child->get_data().set_stype(children[0]->get_data().get_stype());
            }
        } else if (head_name == "init_dec") {
            if (child_name == "var_dec_global") {
                child->get_data().set_stype(symbol.get_stype());
                std::string id_name = node->get_parent()->get_children()[1]->get_data().get_content();
                semantic_type id_stype = node->get_parent()->get_children()[1]->get_data().get_stype();

                SymbolTableEntry *existing_entry = find_variable_in_scope(symbol_table, current_func, id_name,
                                                                          def_area);
                if (existing_entry == nullptr || existing_entry->get_def_area() != def_area) {
                    bool is_mut = false;
                    SymbolTableEntry new_entry(VAR, id_stype, def_area, is_mut);
                    new_entry.set_name(id_name);
                    symbol_table[current_func][id_name] = new_entry;
                } else {
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: Redefinition of variable '"
                              << id_name << "' in the same scope." << WHITE << std::endl;
                    std::cerr << "---------------------------------------------------------------" << std::endl;
                    num_errors++;
                }
            } else if (child_name == "func") {
                child->get_data().set_stype(symbol.get_stype());
                std::string id_name = node->get_parent()->get_children()[1]->get_data().get_content();
                semantic_type id_stype = node->get_parent()->get_children()[1]->get_data().get_stype();

                if (global_function_table.count(id_name)) {
                    global_function_table[id_name].set_stype(id_stype);
                }
            }
        } else if (head_name == "var_dec_global") {
            if (child_name == "more") {
                if (symbol.get_stype() != children[1]->get_data().get_stype() &&
                    children[1]->get_data().get_stype() != VOID) {
                    std::string id_name = node->get_parent()->get_parent()->get_children()[1]->get_data().get_content();
                    std::string left_type = semantic_type_to_string[symbol.get_stype()];
                    std::string right_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                    std::cerr << RED << "Semantic Error [Line " << line_number
                              << "]: Assigning types don't match for id '" << id_name << "'." << WHITE << std::endl;
                    std::cerr << RED << "id type is '" << left_type << "', but assign value type is '" << right_type
                              << "'." << WHITE << std::endl;
                    std::cerr << "---------------------------------------------------------------" << std::endl;
                    num_errors++;
                }
                child->get_data().set_stype(symbol.get_stype());
            }
        } else if (head_name == "more") {
            if (child_name == "var_dec_list") {
                child->get_data().set_stype(symbol.get_stype());
            }
        } else if (head_name == "var_dec") {
            if (child_name == "var_dec_list") {
                child->get_data().set_stype(children[0]->get_data().get_stype());
            }
        } else if (head_name == "var_dec_list") {
            if (child_name == "var_dec_init") {
                child->get_data().set_stype(symbol.get_stype());
            } else if (child_name == "var_dec_list2") {
                child->get_data().set_stype(symbol.get_stype());
            }
        } else if (head_name == "var_dec_list2") {
            if (child_name == "var_dec_init") {
                child->get_data().set_stype(symbol.get_stype());
            } else if (child_name == "var_dec_list2") {
                child->get_data().set_stype(symbol.get_stype());
            }
        } else if (head_name == "var_dec_init") {
            if (child_name == "var_id") {
                child->get_data().set_stype(symbol.get_stype());
            }
        } else if (head_name == "var_id") {
            if (child_name == "id") {
                child->get_data().set_stype(symbol.get_stype());
                std::string id_name = children[0]->get_data().get_content();
                semantic_type id_stype = children[0]->get_data().get_stype();

                SymbolTableEntry *existing_entry = find_variable_in_scope(symbol_table, current_func, id_name,
                                                                          def_area);
                if (existing_entry == nullptr || existing_entry->get_def_area() != def_area) {
                    bool is_mut = false;
                    Node<Symbol> *mut_opt_node = node->get_parent()->get_parent()->get_children()[1];
                    if (mut_opt_node && mut_opt_node->get_data().get_name() == "mut_opt" &&
                        mut_opt_node->get_children().size() > 0 &&
                        mut_opt_node->get_children()[0]->get_data().get_name() == "T_Mut") {
                        is_mut = true;
                    }

                    SymbolTableEntry new_entry(VAR, id_stype, def_area, is_mut);
                    new_entry.set_name(id_name);
                    symbol_table[current_func][id_name] = new_entry;
                } else {
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: Redefinition of variable '"
                              << id_name << "' in the same scope." << WHITE << std::endl;
                    std::cerr << "---------------------------------------------------------------" << std::endl;
                    num_errors++;
                }
            }
        } else if (head_name == "param") {
            if (child_name == "param_id") {
                child->get_data().set_stype(children[0]->get_data().get_stype());
            }
        } else if (head_name == "param_id") {
            if (child_name == "id") {
                child->get_data().set_stype(symbol.get_stype());
            }
        } else if (head_name == "for_stmt") {
            if (child_name == "stmt") {
                Node<Symbol> *var_dec = node->get_children()[2];
                if (var_dec && var_dec->get_children().size() > 0 &&
                    var_dec->get_children()[0]->get_data().get_name() == "type") {
                    Node<Symbol> *var_dec_list = var_dec->get_children()[1];
                    if (var_dec_list && var_dec_list->get_children().size() > 0 &&
                        var_dec_list->get_children()[0]->get_data().get_name() == "var_dec_init") {
                        std::string id_name = var_dec_list->get_children()[0]->get_children()[0]->get_children()[0]->get_data().get_content();
                        SymbolTableEntry *entry = find_variable_in_scope(symbol_table, current_func, id_name, def_area);
                        if (entry) entry->set_def_area(entry->get_def_area() + 1);

                        Node<Symbol> *var_dec_list2 = var_dec_list->get_children()[1];
                        while (var_dec_list2 && var_dec_list2->get_children().size() > 0 &&
                               var_dec_list2->get_children()[0]->get_data().get_name() != "eps") {
                            std::string id_list_name = var_dec_list2->get_children()[1]->get_children()[0]->get_children()[0]->get_data().get_content();
                            SymbolTableEntry *entry_list = find_variable_in_scope(symbol_table, current_func,
                                                                                  id_list_name, def_area);
                            if (entry_list) entry_list->set_def_area(entry_list->get_def_area() + 1);
                            var_dec_list2 = var_dec_list2->get_children()[2];
                        }
                    }
                }
            }
        } else if (head_name == "fac_id_opt") {
            if (child_name == "T_LP") {
                std::string id_name = node->get_parent()->get_children()[0]->get_data().get_content();
                if (!global_function_table.count(id_name)) {
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: Call to undefined function '"
                              << id_name << "'." << WHITE << std::endl;
                    std::cerr << "---------------------------------------------------------------" << std::endl;
                    num_errors++;
                    symbol.set_stype(VOID);
                } else {
                    symbol.set_stype(global_function_table[id_name].get_stype());
                }
            } else if (child_name == "ebracket") {
                std::string id_name = node->get_parent()->get_children()[0]->get_data().get_content();
                SymbolTableEntry *entry = find_variable_in_scope(symbol_table, current_func, id_name, def_area);

                if (entry == nullptr) {
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: Use of undefined variable '"
                              << id_name << "'." << WHITE << std::endl;
                    std::cerr << "---------------------------------------------------------------" << std::endl;
                    num_errors++;
                    symbol.set_stype(VOID);
                } else if (entry->get_type() == FUNC) {
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: Identifier '" << id_name
                              << "' is a function but used as a variable." << WHITE << std::endl;
                    std::cerr << "---------------------------------------------------------------" << std::endl;
                    num_errors++;
                    symbol.set_stype(VOID);
                } else {
                    symbol.set_stype(entry->get_stype());
                }
            }
        } else if (head_name == "stmt_after_id") {
            if (child_name == "T_Assign") {
                std::string id_name = node->get_parent()->get_children()[0]->get_data().get_content();
                SymbolTableEntry *var_entry = find_variable_in_scope(symbol_table, current_func, id_name, def_area);
                if (var_entry == nullptr) {
                    std::cerr << RED << "Semantic Error [Line " << line_number
                              << "]: Assignment to undeclared variable '"
                              << id_name << "'." << WHITE << std::endl;
                    std::cerr << "---------------------------------------------------------------" << std::endl;
                    num_errors++;
                } else if (!var_entry->get_mut()) {
                    std::cerr << RED << "Semantic Error [Line " << line_number
                              << "]: Cannot assign to immutable variable '"
                              << id_name << "'." << WHITE << std::endl;
                    std::cerr << "---------------------------------------------------------------" << std::endl;
                    num_errors++;
                } else if (var_entry->get_stype() == UNK) {
                    var_entry->set_stype(children[1]->get_data().get_stype());
                } else if (var_entry->get_stype() != children[1]->get_data().get_stype() &&
                           children[1]->get_data().get_stype() != VOID) {
                    std::string left_type = semantic_type_to_string[var_entry->get_stype()];
                    std::string right_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: Type mismatch in assignment for '"
                              << id_name << "'." << WHITE << std::endl;
                    std::cerr << RED << "Expected '" << left_type << "', but got '" << right_type << "'." << WHITE
                              << std::endl;
                    std::cerr << "---------------------------------------------------------------" << std::endl;
                    num_errors++;
                }
            }
        }


        dfs(child);
    }

    line_number = symbol.get_line_number();

    if (head_name == "func") {
        std::string func_name = children[1]->get_data().get_content();
        SymbolTableEntry &func_entry = global_function_table[func_name];

        Node<Symbol> *func_args_node = children[3];
        if (func_args_node && func_args_node->get_children().size() > 0 &&
            func_args_node->get_children()[0]->get_data().get_name() != "eps") {
            Node<Symbol> *current_arg = func_args_node->get_children()[0];
            Node<Symbol> *current_tail = func_args_node->get_children()[1];

            func_entry.clear_parameters();

            while (true) {
                std::string param_name = current_arg->get_children()[0]->get_data().get_content();
                semantic_type param_type = UNK;

                if (current_arg->get_children().size() > 1 &&
                    current_arg->get_children()[1]->get_data().get_name() == "arg_type" &&
                    current_arg->get_children()[1]->get_children().size() > 0 &&
                    current_arg->get_children()[1]->get_children()[0]->get_data().get_name() != "eps") {
                    param_type = current_arg->get_children()[1]->get_children()[1]->get_data().get_stype();
                }
                func_entry.add_to_parameters({param_name, param_type});

                SymbolTableEntry *existing_param_var = find_variable_in_scope(symbol_table, current_func, param_name,
                                                                              def_area);
                if (existing_param_var == nullptr || existing_param_var->get_def_area() != def_area) {
                    SymbolTableEntry new_param_var(VAR, param_type, def_area);
                    new_param_var.set_name(param_name);
                    symbol_table[current_func][param_name] = new_param_var;
                } else {
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: Redefinition of parameter '"
                              << param_name
                              << "' in the same scope." << WHITE << std::endl;
                    std::cerr << "---------------------------------------------------------------" << std::endl;
                    num_errors++;
                }

                if (current_tail->get_children().empty() ||
                    current_tail->get_children()[0]->get_data().get_name() == "eps") {
                    break;
                }
                current_arg = current_tail->get_children()[1];
                current_tail = current_tail->get_children()[2];
            }
        }

        semantic_type return_type = VOID;
        Node<Symbol> *func_type_node = children[4];
        if (func_type_node && func_type_node->get_children().size() > 0 &&
            func_type_node->get_children()[0]->get_data().get_name() != "eps") {
            return_type = func_type_node->get_children()[1]->get_data().get_stype();
        }
        func_entry.set_stype(return_type);
    }

    if (scope_opened) {
        std::map<std::string, SymbolTableEntry> temp_current_func_vars_to_remove;
        if (symbol_table.count(current_func)) {
            for (const auto &entry_pair: symbol_table[current_func]) {
                if (entry_pair.second.get_def_area() == def_area) {
                    temp_current_func_vars_to_remove[entry_pair.first] = entry_pair.second;
                }
            }
            for (const auto &entry_pair: temp_current_func_vars_to_remove) {
                symbol_table[current_func].erase(entry_pair.first);
            }
        }
        def_area--;
    }


    if (head_name == "dec") {
        symbol.set_stype(children[0]->get_data().get_stype());
    } else if (head_name == "var_dec") {
        symbol.set_stype(children[0]->get_data().get_stype());
    } else if (head_name == "var_dec_init") {
        std::string var_name = children[0]->get_children()[0]->get_data().get_content();
        SymbolTableEntry *var_entry = find_variable_in_scope(symbol_table, current_func, var_name, def_area);

        if (var_entry && var_entry->get_stype() == UNK) {
            var_entry->set_stype(children[1]->get_data().get_stype());
            symbol.set_stype(children[1]->get_data().get_stype());
        } else if (var_entry && var_entry->get_stype() != children[1]->get_data().get_stype() &&
                   children[1]->get_data().get_stype() != VOID) {
            std::string id_name = children[0]->get_children()[0]->get_data().get_content();
            std::string left_type = semantic_type_to_string[children[0]->get_data().get_stype()];
            std::string right_type = semantic_type_to_string[children[1]->get_data().get_stype()];
            std::cerr << RED << "Semantic Error [Line " << line_number << "]: Assigning types don't match for id '"
                      << id_name << "'." << WHITE << std::endl;
            std::cerr << RED << "id type is '" << left_type << "', but assign value type is '" << right_type << "'."
                      << WHITE << std::endl;
            std::cerr << "---------------------------------------------------------------" << std::endl;
            num_errors++;
        }
        if (var_entry && !var_entry->get_mut()) {
            std::cerr << RED << "Semantic Error [Line " << line_number << "]: Cannot assign to immutable variable '"
                      << var_name << "'." << WHITE << std::endl;
            std::cerr << "---------------------------------------------------------------" << std::endl;
            num_errors++;
        }

    } else if (head_name == "sz") {
        if (children[0]->get_data().get_name() == "[") {
            if (children[1]->get_data().get_stype() != VOID && children[1]->get_data().get_stype() != INT) {
                Node<Symbol> *tmp = node->get_parent();
                std::string id_name;
                while (tmp) {
                    if (tmp->get_data().get_name() == "dec") {
                        id_name = tmp->get_children()[1]->get_data().get_content();
                        break;
                    } else if (tmp->get_data().get_name() == "var_dec_init") {
                        id_name = tmp->get_children()[0]->get_children()[0]->get_data().get_content();
                        break;
                    }
                    tmp = tmp->get_parent();
                }
                std::string size_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number
                          << "]: The array size must have an int type for array '" << id_name << "'." << WHITE
                          << std::endl;
                std::cerr << RED << "The array size type is '" << size_type << "'." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
        }
    } else if (head_name == "optexp") {
        if (children[0]->get_data().get_name() == "exp") {
            symbol.set_stype(children[0]->get_data().get_stype());
            symbol.set_val(children[0]->get_data().get_val());

            if (!children[0]->get_data().get_val().empty() && children[0]->get_data().get_stype() == INT) {
                if (children[0]->get_data().get_val() == "ERROR_OVERFLOW" ||
                    children[0]->get_data().get_val() == "ERROR_DIVISION_BY_ZERO" ||
                    children[0]->get_data().get_val() == "ERROR_MODULO_BY_ZERO") {
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: Arithmetic error in array index: "
                              << children[0]->get_data().get_val() << "." << WHITE << std::endl;
                    std::cerr << "---------------------------------------------------------------" << std::endl;
                    num_errors++;
                } else {
                    if (std::stoll(children[0]->get_data().get_val()) < 0) {
                        Node<Symbol> *tmp = node->get_parent();
                        std::string id_name = "unknown_array";
                        while (tmp) {
                            if (tmp->get_data().get_name() == "dec") {
                                id_name = tmp->get_children()[1]->get_data().get_content();
                                break;
                            } else if (tmp->get_data().get_name() == "var_dec_init") {
                                id_name = tmp->get_children()[0]->get_children()[0]->get_data().get_content();
                                break;
                            } else if (tmp->get_data().get_name() == "ebracket") {
                                if (tmp->get_parent() && tmp->get_parent()->get_parent() &&
                                    tmp->get_parent()->get_parent()->get_children().size() > 0) {
                                    id_name = tmp->get_parent()->get_parent()->get_children()[0]->get_data().get_content();
                                }
                                break;
                            }
                            tmp = tmp->get_parent();
                        }
                        long long index_val = std::stoll(children[0]->get_data().get_val());
                        std::cerr << RED << "Semantic Error [Line " << line_number
                                  << "]: The array index/size value must be non-negative for '" << id_name << "'."
                                  << WHITE << std::endl;
                        std::cerr << RED << "The array index/size value is '" << std::to_string(index_val) << "'."
                                  << WHITE << std::endl;
                        std::cerr << "---------------------------------------------------------------" << std::endl;
                        num_errors++;
                    }
                }
            }
        } else if (children[0]->get_data().get_name() == "eps") {
            symbol.set_stype(VOID);
        }
    } else if (head_name == "initial") {
        if (children[0]->get_data().get_name() == "=") {
            symbol.set_stype(children[1]->get_data().get_stype());
        } else if (children[0]->get_data().get_name() == "eps") {
            symbol.set_stype(VOID);
        }
    } else if (head_name == "eexp") {
        if (children[0]->get_data().get_name() == "exp1") {
            symbol.set_stype(children[0]->get_data().get_stype());
        } else if (children[0]->get_data().get_name() == "{") {
            symbol.set_stype(children[1]->get_data().get_stype());
        }
    } else if (head_name == "exp_list") {
        if (children[0]->get_data().get_name() == "exp1") {
            if (children[1]->get_data().get_stype() != VOID &&
                children[0]->get_data().get_stype() != children[1]->get_data().get_stype()) {
                Node<Symbol> *tmp = node->get_parent();
                std::string id_name = "unknown_array";
                while (tmp) {
                    if (tmp->get_data().get_name() == "dec") {
                        id_name = tmp->get_children()[1]->get_data().get_content();
                        break;
                    } else if (tmp->get_data().get_name() == "var_dec_init") {
                        id_name = tmp->get_children()[0]->get_children()[0]->get_data().get_content();
                        break;
                    }
                    tmp = tmp->get_parent();
                }
                std::string member_type1 = semantic_type_to_string[children[0]->get_data().get_stype()];
                std::string member_type2 = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number
                          << "]: All array members must have the same type for array '" << id_name << "'." << WHITE
                          << std::endl;
                std::cerr << RED << "'" << member_type1 << "' and '" << member_type2 << "' types exist in array."
                          << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
                symbol.set_stype(VOID);
            } else {
                symbol.set_stype(children[0]->get_data().get_stype());
            }
        } else if (children[0]->get_data().get_name() == "eps") {
            symbol.set_stype(VOID);
        }
    } else if (head_name == "exp_list2") {
        if (children[0]->get_data().get_name() == ",") {
            if (children[2]->get_data().get_stype() != VOID &&
                children[1]->get_data().get_stype() != children[2]->get_data().get_stype()) {
                Node<Symbol> *tmp = node->get_parent();
                std::string id_name = "unknown_array";
                while (tmp) {
                    if (tmp->get_data().get_name() == "dec") {
                        id_name = tmp->get_children()[1]->get_data().get_content();
                        break;
                    } else if (tmp->get_data().get_name() == "var_dec_init") {
                        id_name = tmp->get_children()[0]->get_children()[0]->get_data().get_content();
                        break;
                    }
                    tmp = tmp->get_parent();
                }
                std::string member_type1 = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::string member_type2 = semantic_type_to_string[children[2]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number
                          << "]: All array members must have the same type for array '" << id_name << "'." << WHITE
                          << std::endl;
                std::cerr << RED << "'" << member_type1 << "' and '" << member_type2 << "' types exist in array."
                          << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
                symbol.set_stype(VOID);
            } else {
                symbol.set_stype(children[1]->get_data().get_stype());
            }
        } else if (children[0]->get_data().get_name() == "eps") {
            symbol.set_stype(VOID);
        }
    } else if (head_name == "type") {
        if (children[0]->get_data().get_name() == "int") {
            symbol.set_stype(INT);
        } else if (children[0]->get_data().get_name() == "bool") {
            symbol.set_stype(BOOL);
        }
    } else if (head_name == "param") {
        symbol.set_stype(children[0]->get_data().get_stype());
        std::string param_id = children[1]->get_children()[0]->get_data().get_content();

        if (global_function_table.count(current_func)) {
            global_function_table[current_func].add_to_parameters({param_id, symbol.get_stype()});
        }

        SymbolTableEntry *existing_param_var = find_variable_in_scope(symbol_table, current_func, param_id, def_area);
        if (existing_param_var == nullptr || existing_param_var->get_def_area() != def_area) {
            SymbolTableEntry new_param_var(VAR, symbol.get_stype(), def_area);
            new_param_var.set_name(param_id);
            symbol_table[current_func][param_id] = new_param_var;
        } else {
            std::cerr << RED << "Semantic Error [Line " << line_number << "]: Redefinition of parameter '" << param_id
                      << "' in the same scope." << WHITE << std::endl;
            std::cerr << "---------------------------------------------------------------" << std::endl;
            num_errors++;
        }
    } else if (head_name == "stmt") {
        if (children[0]->get_data().get_name() == "return_stmt") {
            semantic_type expected_return_type = VOID;
            if (global_function_table.count(current_func)) {
                expected_return_type = global_function_table[current_func].get_stype();
            }

            if (children[0]->get_data().get_stype() != expected_return_type) {
                std::string func_type_str = semantic_type_to_string[expected_return_type];
                std::string return_type_str = semantic_type_to_string[children[0]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number << "]: Return statement for function '"
                          << current_func << "' has unmatched type." << WHITE << std::endl;
                std::cerr << RED << "Function type is '" << func_type_str << "' but return type is '" << return_type_str
                          << "'." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
        }
    } else if (head_name == "if_stmt") {
        if (children[2]->get_data().get_stype() != VOID && children[2]->get_data().get_stype() != BOOL) {
            std::string condition_type = semantic_type_to_string[children[2]->get_data().get_stype()];
            std::cerr << RED << "Semantic Error [Line " << line_number
                      << "]: The condition type of if statement must have a bool type." << WHITE << std::endl;
            std::cerr << RED << "The condition type is '" << condition_type << "'." << WHITE << std::endl;
            std::cerr << "---------------------------------------------------------------" << std::endl;
            num_errors++;
        }
    } else if (head_name == "return_stmt") {
        symbol.set_stype(children[1]->get_data().get_stype());
    } else if (head_name == "exp") {
        if (children.size() > 1 && children[1]->get_data().get_stype() != VOID) {
            symbol.set_stype(children[1]->get_data().get_stype());
        } else {
            symbol.set_stype(children[0]->get_data().get_stype());
            symbol.set_val(children[0]->get_data().get_val());
        }
    } else if (head_name == "t1") {
        if (children[0]->get_data().get_name() == ",") {
            if (children[2]->get_data().get_stype() != VOID &&
                children[1]->get_data().get_stype() != children[2]->get_data().get_stype()) {
                std::string member_type1 = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::string member_type2 = semantic_type_to_string[children[2]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number
                          << "]: Tuple members must have matching types." << WHITE << std::endl;
                std::cerr << RED << "Types '" << member_type1 << "' and '" << member_type2 << "' exist in tuple."
                          << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
                symbol.set_stype(VOID);
            } else {
                symbol.set_stype(children[1]->get_data().get_stype());
            }
        } else if (children[0]->get_data().get_name() == "eps") {
            symbol.set_stype(VOID);
        }
    } else if (head_name == "exp1") {
        if (children.size() > 1 && children[1]->get_data().get_stype() != VOID) {
            if (children[0]->get_data().get_stype() != children[1]->get_data().get_stype()) {
                std::string id_name = "unknown_var";
                Node<Symbol> *tmp_node = node;
                while (tmp_node && tmp_node->get_data().get_name() != "stmt_after_id" &&
                       tmp_node->get_data().get_name() != "var_dec_init") {
                    tmp_node = tmp_node->get_parent();
                }
                if (tmp_node && tmp_node->get_data().get_name() == "stmt_after_id") {
                    if (tmp_node->get_parent() && tmp_node->get_parent()->get_children().size() > 0) {
                        id_name = tmp_node->get_parent()->get_children()[0]->get_data().get_content();
                    }
                } else if (tmp_node && tmp_node->get_data().get_name() == "var_dec_init") {
                    if (tmp_node->get_children().size() > 0 && tmp_node->get_children()[0]->get_children().size() > 0) {
                        id_name = tmp_node->get_children()[0]->get_children()[0]->get_data().get_content();
                    }
                }

                std::string left_type = semantic_type_to_string[children[0]->get_data().get_stype()];
                std::string right_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number
                          << "]: Type mismatch in assignment/operation for '" << id_name << "'." << WHITE << std::endl;
                std::cerr << RED << "Expected '" << left_type << "', but got '" << right_type << "'." << WHITE
                          << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
            symbol.set_stype(children[0]->get_data().get_stype());
            symbol.set_val(children[0]->get_data().get_val());
        } else {
            symbol.set_stype(children[0]->get_data().get_stype());
            symbol.set_val(children[0]->get_data().get_val());
        }
    } else if (head_name == "t2") {
        if (children[0]->get_data().get_name() == "=") {
            if (children[2]->get_data().get_stype() != VOID &&
                children[1]->get_data().get_stype() != children[2]->get_data().get_stype()) {
                std::string id_name = "unknown_var";
                Node<Symbol> *tmp_node = node;
                while (tmp_node && tmp_node->get_data().get_name() != "stmt_after_id" &&
                       tmp_node->get_data().get_name() != "var_dec_init") {
                    tmp_node = tmp_node->get_parent();
                }
                if (tmp_node && tmp_node->get_data().get_name() == "stmt_after_id") {
                    if (tmp_node->get_parent() && tmp_node->get_parent()->get_children().size() > 0) {
                        id_name = tmp_node->get_parent()->get_children()[0]->get_data().get_content();
                    }
                } else if (tmp_node && tmp_node->get_data().get_name() == "var_dec_init") {
                    if (tmp_node->get_children().size() > 0 && tmp_node->get_children()[0]->get_children().size() > 0) {
                        id_name = tmp_node->get_children()[0]->get_children()[0]->get_data().get_content();
                    }
                }
                std::string left_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::string right_type = semantic_type_to_string[children[2]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number
                          << "]: Type mismatch in assignment/operation for '" << id_name << "'." << WHITE << std::endl;
                std::cerr << RED << "Expected '" << left_type << "', but got '" << right_type << "'." << WHITE
                          << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
                symbol.set_stype(children[1]->get_data().get_stype());
            } else {
                symbol.set_stype(children[1]->get_data().get_stype());
            }
        } else if (children[0]->get_data().get_name() == "eps") {
            symbol.set_stype(VOID);
        }
    } else if (head_name == "exp2") {
        if (children.size() > 1 && children[1]->get_data().get_stype() != VOID) {
            if (!(children[0]->get_data().get_stype() == BOOL && children[1]->get_data().get_stype() == BOOL)) {
                std::string left_type = semantic_type_to_string[children[0]->get_data().get_stype()];
                std::string right_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number
                          << "]: Logical operator '||' takes 2 values with boolean type." << WHITE << std::endl;
                std::cerr << RED << "First value type is '" << left_type << "', but second value type is '"
                          << right_type << "'." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
            symbol.set_stype(BOOL);
            symbol.set_val(children[0]->get_data().get_val());
        } else {
            symbol.set_stype(children[0]->get_data().get_stype());
            symbol.set_val(children[0]->get_data().get_val());
        }
    } else if (head_name == "t3") {
        if (children[0]->get_data().get_name() == "||") {
            if (children[2]->get_data().get_stype() != VOID &&
                !(children[1]->get_data().get_stype() == BOOL && children[2]->get_data().get_stype() == BOOL)) {
                std::string left_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::string right_type = semantic_type_to_string[children[2]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number
                          << "]: Logical operator '||' takes 2 values with boolean type." << WHITE << std::endl;
                std::cerr << RED << "First value type is '" << left_type << "', but second value type is '"
                          << right_type << "'." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
            symbol.set_stype(BOOL);
            symbol.set_val(children[1]->get_data().get_val() + children[0]->get_data().get_name() +
                           children[2]->get_data().get_val());
        } else if (children[0]->get_data().get_name() == "eps") {
            symbol.set_stype(VOID);
        }
    } else if (head_name == "exp3") {
        if (children.size() > 1 && children[1]->get_data().get_stype() != VOID) {
            if (!(children[0]->get_data().get_stype() == BOOL && children[1]->get_data().get_stype() == BOOL)) {
                std::string left_type = semantic_type_to_string[children[0]->get_data().get_stype()];
                std::string right_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number
                          << "]: Logical operator '&&' takes 2 values with boolean type." << WHITE << std::endl;
                std::cerr << RED << "First value type is '" << left_type << "', but second value type is '"
                          << right_type << "'." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
            symbol.set_stype(BOOL);
            symbol.set_val(children[0]->get_data().get_val());
        } else {
            symbol.set_stype(children[0]->get_data().get_stype());
            symbol.set_val(children[0]->get_data().get_val());
        }
    } else if (head_name == "t4") {
        if (children[0]->get_data().get_name() == "&&") {
            if (children[2]->get_data().get_stype() != VOID &&
                !(children[1]->get_data().get_stype() == BOOL && children[2]->get_data().get_stype() == BOOL)) {
                std::string left_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::string right_type = semantic_type_to_string[children[2]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number
                          << "]: Logical operator '&&' takes 2 values with boolean type." << WHITE << std::endl;
                std::cerr << RED << "First value type is '" << left_type << "', but second value type is '"
                          << right_type << "'." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
            symbol.set_stype(BOOL);
            symbol.set_val(children[1]->get_data().get_val() + children[0]->get_data().get_name() +
                           children[2]->get_data().get_val());
        } else if (children[0]->get_data().get_name() == "eps") {
            symbol.set_stype(VOID);
        }
    } else if (head_name == "exp4") {
        if (children.size() > 1 && children[1]->get_data().get_stype() != VOID) {
            if (children[0]->get_data().get_stype() != children[1]->get_data().get_stype()) {
                std::string op = children[1]->get_children()[0]->get_data().get_name();
                std::string left_type = semantic_type_to_string[children[0]->get_data().get_stype()];
                std::string right_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number << "]: Comparison operator '" << op
                          << "' takes 2 values of the same type." << WHITE << std::endl;
                std::cerr << RED << "First value type is '" << left_type << "', but second value type is '"
                          << right_type << "'." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
            symbol.set_stype(BOOL);
            symbol.set_val(children[0]->get_data().get_val());
        } else {
            symbol.set_stype(children[0]->get_data().get_stype());
            symbol.set_val(children[0]->get_data().get_val());
        }
    } else if (head_name == "t5") {
        if (children[0]->get_data().get_name() == "==" || children[0]->get_data().get_name() == "!=") {
            if (children[2]->get_data().get_stype() != VOID &&
                children[1]->get_data().get_stype() != children[2]->get_data().get_stype()) {
                std::string op = children[0]->get_data().get_name();
                std::string left_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::string right_type = semantic_type_to_string[children[2]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number << "]: Comparison operator '" << op
                          << "' takes 2 values of the same type." << WHITE << std::endl;
                std::cerr << RED << "First value type is '" << left_type << "', but second value type is '"
                          << right_type << "'." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
            symbol.set_stype(BOOL);
            symbol.set_val(children[1]->get_data().get_val() + children[0]->get_data().get_name() +
                           children[2]->get_data().get_val());
        } else if (children[0]->get_data().get_name() == "eps") {
            symbol.set_stype(VOID);
        }
    } else if (head_name == "exp5") {
        if (children.size() > 1 && children[1]->get_data().get_stype() != VOID) {
            if (children[0]->get_data().get_stype() != children[1]->get_data().get_stype() ||
                children[0]->get_data().get_stype() != INT) {
                std::string op = children[1]->get_children()[0]->get_data().get_name();
                std::string left_type = semantic_type_to_string[children[0]->get_data().get_stype()];
                std::string right_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number << "]: Relational operator '" << op
                          << "' takes 2 integer values." << WHITE << std::endl;
                std::cerr << RED << "First value type is '" << left_type << "', but second value type is '"
                          << right_type << "'." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
            symbol.set_stype(BOOL);
            symbol.set_val(children[0]->get_data().get_val());
        } else {
            symbol.set_stype(children[0]->get_data().get_stype());
            symbol.set_val(children[0]->get_data().get_val());
        }
    } else if (head_name == "t6") {
        if (children[0]->get_data().get_name() == "<" || children[0]->get_data().get_name() == ">" ||
            children[0]->get_data().get_name() == "<=" || children[0]->get_data().get_name() == ">=") {
            if (children[2]->get_data().get_stype() != VOID &&
                (children[1]->get_data().get_stype() != children[2]->get_data().get_stype() ||
                 children[1]->get_data().get_stype() != INT)) {
                std::string op = children[0]->get_data().get_name();
                std::string left_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::string right_type = semantic_type_to_string[children[2]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number << "]: Relational operator '" << op
                          << "' takes 2 integer values." << WHITE << std::endl;
                std::cerr << RED << "First value type is '" << left_type << "', but second value type is '"
                          << right_type << "'." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
            symbol.set_stype(BOOL);
            symbol.set_val(children[1]->get_data().get_val() + children[0]->get_data().get_name() +
                           children[2]->get_data().get_val());
        } else if (children[0]->get_data().get_name() == "eps") {
            symbol.set_stype(VOID);
        }
    } else if (head_name == "exp6") {
        if (children.size() > 1 && children[1]->get_data().get_stype() != VOID) {
            if (children[0]->get_data().get_stype() != children[1]->get_data().get_stype() ||
                children[0]->get_data().get_stype() != INT) {
                std::string op = children[1]->get_children()[0]->get_data().get_name();
                std::string left_type = semantic_type_to_string[children[0]->get_data().get_stype()];
                std::string right_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number << "]: Arithmetic operator '" << op
                          << "' takes 2 integer values." << WHITE << std::endl;
                std::cerr << RED << "First value type is '" << left_type << "', but second value type is '"
                          << right_type << "'." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
            symbol.set_stype(INT);
            std::vector<char> chs = {'+', '-'};
            symbol.set_val(eval(children[0]->get_data().get_val() + children[1]->get_data().get_val(), chs));
        } else {
            symbol.set_stype(children[0]->get_data().get_stype());
            symbol.set_val(children[0]->get_data().get_val());
        }
    } else if (head_name == "t7") {
        if (children[0]->get_data().get_name() == "+" || children[0]->get_data().get_name() == "-") {
            if (children[2]->get_data().get_stype() != VOID &&
                (children[1]->get_data().get_stype() != children[2]->get_data().get_stype() ||
                 children[1]->get_data().get_stype() != INT)) {
                std::string op = children[0]->get_data().get_name();
                std::string left_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::string right_type = semantic_type_to_string[children[2]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number << "]: Arithmetic operator '" << op
                          << "' takes 2 integer values." << WHITE << std::endl;
                std::cerr << RED << "First value type is '" << left_type << "', but second value type is '"
                          << right_type << "'." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
            symbol.set_stype(INT);
            symbol.set_val(children[0]->get_data().get_name() + children[1]->get_data().get_val() +
                           children[2]->get_data().get_val());
        } else if (children[0]->get_data().get_name() == "eps") {
            symbol.set_stype(VOID);
        }
    } else if (head_name == "exp7") {
        if (children.size() > 1 && children[1]->get_data().get_stype() != VOID) {
            if (children[0]->get_data().get_stype() != children[1]->get_data().get_stype() ||
                children[0]->get_data().get_stype() != INT) {
                std::string op = children[1]->get_children()[0]->get_data().get_name();
                std::string left_type = semantic_type_to_string[children[0]->get_data().get_stype()];
                std::string right_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number << "]: Arithmetic operator '" << op
                          << "' takes 2 integer values." << WHITE << std::endl;
                std::cerr << RED << "First value type is '" << left_type << "', but second value type is '"
                          << right_type << "'." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
            symbol.set_stype(INT);
            std::vector<char> chs = {'*', '/', '%'};
            symbol.set_val(eval(children[0]->get_data().get_val() + children[1]->get_data().get_val(), chs));
        } else {
            symbol.set_stype(children[0]->get_data().get_stype());
            symbol.set_val(children[0]->get_data().get_val());
        }
    } else if (head_name == "t8") {
        if (children[0]->get_data().get_name() == "*" || children[0]->get_data().get_name() == "/" ||
            children[0]->get_data().get_name() == "%") {
            if (children[2]->get_data().get_stype() != VOID &&
                (children[1]->get_data().get_stype() != children[2]->get_data().get_stype() ||
                 children[1]->get_data().get_stype() != INT)) {
                std::string op = children[0]->get_data().get_name();
                std::string left_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::string right_type = semantic_type_to_string[children[2]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number << "]: Arithmetic operator '" << op
                          << "' takes 2 integer values." << WHITE << std::endl;
                std::cerr << RED << "First value type is '" << left_type << "', but second value type is '"
                          << right_type << "'." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
            symbol.set_stype(INT);
            symbol.set_val(children[0]->get_data().get_name() + children[1]->get_data().get_val() +
                           children[2]->get_data().get_val());
        } else if (children[0]->get_data().get_name() == "eps") {
            symbol.set_stype(VOID);
        }
    } else if (head_name == "exp8") {
        if (children[0]->get_data().get_name() == "+" || children[0]->get_data().get_name() == "-") {
            if (children[1]->get_data().get_stype() != INT) {
                std::string op = children[0]->get_data().get_name();
                std::string right_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number << "]: Unary operator '" << op
                          << "' takes an integer value." << WHITE << std::endl;
                std::cerr << RED << "Value type is '" << right_type << "'." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
            symbol.set_stype(INT);
            if (children[0]->get_data().get_name() == "-") {
                symbol.set_val("-" + children[1]->get_data().get_val());
            } else {
                symbol.set_val(children[1]->get_data().get_val());
            }
        } else if (children[0]->get_data().get_name() == "!") {
            if (children[1]->get_data().get_stype() != BOOL) {
                std::string op = children[0]->get_data().get_name();
                std::string right_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number << "]: Unary operator '" << op
                          << "' takes a boolean value." << WHITE << std::endl;
                std::cerr << RED << "Value type is '" << right_type << "'." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
            symbol.set_stype(BOOL);
            symbol.set_val("!" + children[1]->get_data().get_val());
        } else if (children[0]->get_data().get_name() == "exp9") {
            symbol.set_stype(children[0]->get_data().get_stype());
            symbol.set_val(children[0]->get_data().get_val());
        }
    } else if (head_name == "exp9") {
        if (children[0]->get_data().get_name() == "T_LP") {
            symbol.set_stype(children[1]->get_data().get_stype());
            symbol.set_val(children[1]->get_data().get_val());
        } else if (children[0]->get_data().get_name() == "id") {
            SymbolTableEntry *entry = find_variable_in_scope(symbol_table, current_func,
                                                             children[0]->get_data().get_content(), def_area);
            if (entry == nullptr) {
                std::cerr << RED << "Semantic Error [Line " << line_number << "]: Use of undeclared identifier '"
                          << children[0]->get_data().get_content() << "'." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
                symbol.set_stype(VOID);
                symbol.set_val("0");
            } else {
                children[0]->get_data().set_stype(entry->get_stype());
                symbol.set_stype(entry->get_stype());
                symbol.set_val("0");
            }
        } else if (children[0]->get_data().get_name() == "constant") {
            symbol.set_stype(children[0]->get_data().get_stype());
            symbol.set_val(children[0]->get_data().get_val());
        }
    } else if (head_name == "fac_id_opt") {
        if (node->get_children()[0]->get_data().get_name() == "T_LP") {
            std::string func_id = node->get_parent()->get_children()[0]->get_data().get_content();
            if (!global_function_table.count(func_id)) {
                std::cerr << RED << "Semantic Error [Line " << line_number << "]: Call to undefined function '"
                          << func_id << "'." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
                symbol.set_stype(VOID);
            } else {
                SymbolTableEntry &func_entry = global_function_table[func_id];
                std::vector<std::pair<std::string, semantic_type>> &define_params = func_entry.get_parameters();
                std::vector<semantic_type> &used_params_types = children[1]->get_data().get_params_type();

                if (define_params.size() != used_params_types.size()) {
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: Function '" << func_id
                              << "' expects " << define_params.size() << " arguments but " << used_params_types.size()
                              << " provided." << WHITE << std::endl;
                    std::cerr << "---------------------------------------------------------------" << std::endl;
                    num_errors++;
                } else {
                    for (size_t i = 0; i < define_params.size(); ++i) {
                        if (define_params[i].second == UNK) {
                            global_function_table[func_id].get_parameters()[i].second = used_params_types[i];
                        } else if (define_params[i].second != used_params_types[i]) {
                            std::cerr << RED << "Semantic Error [Line " << line_number << "]: Type mismatch for "
                                      << i + 1 << "th argument in call to '" << func_id << "'." << WHITE << std::endl;
                            std::cerr << RED << "Expected '" << semantic_type_to_string[define_params[i].second]
                                      << "' but got '" << semantic_type_to_string[used_params_types[i]] << "'." << WHITE
                                      << std::endl;
                            std::cerr << "---------------------------------------------------------------" << std::endl;
                            num_errors++;
                        }
                    }
                }
                symbol.set_stype(func_entry.get_stype());
            }
        }
    } else if (head_name == "params_call") {
        if (node->get_children()[0]->get_data().get_name() == "exp1") {
            symbol.add_to_params_type(children[0]->get_data().get_stype());
            if (children.size() > 1 && children[1]->get_data().get_name() == "params_call2") {
                symbol.add_to_params_type(children[1]->get_data().get_params_type());
            }
        }
    } else if (head_name == "params_call2") {
        if (children[0]->get_data().get_name() == ",") {
            symbol.add_to_params_type(children[1]->get_data().get_stype());
            if (children.size() > 2 && children[2]->get_data().get_name() == "params_call2") {
                symbol.add_to_params_type(children[2]->get_data().get_params_type());
            }
        }
    } else if (head_name == "ebracket") {
        if (children[0]->get_data().get_name() == "[") {
            if (children[1]->get_data().get_stype() != INT) {
                std::string id_name = node->get_parent()->get_parent()->get_children()[0]->get_data().get_content();
                std::string index_type = semantic_type_to_string[children[1]->get_data().get_stype()];
                std::cerr << RED << "Semantic Error [Line " << line_number
                          << "]: Array index must be of type 'i32' for '" << id_name << "'." << WHITE << std::endl;
                std::cerr << RED << "The array index type is '" << index_type << "'." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
            if (!children[1]->get_data().get_val().empty() && children[1]->get_data().get_stype() == INT) {
                if (children[1]->get_data().get_val() == "ERROR_OVERFLOW" ||
                    children[1]->get_data().get_val() == "ERROR_DIVISION_BY_ZERO" ||
                    children[1]->get_data().get_val() == "ERROR_MODULO_BY_ZERO") {
                    std::cerr << RED << "Semantic Error [Line " << line_number << "]: Arithmetic error in array index: "
                              << children[1]->get_data().get_val() << "." << WHITE << std::endl;
                    std::cerr << "---------------------------------------------------------------" << std::endl;
                    num_errors++;
                } else if (std::stoll(children[1]->get_data().get_val()) < 0) {
                    std::string id_name = node->get_parent()->get_parent()->get_children()[0]->get_data().get_content();
                    long long index_val = std::stoll(children[1]->get_data().get_val());
                    std::cerr << RED << "Semantic Error [Line " << line_number
                              << "]: Array index value must be non-negative for '" << id_name << "'." << WHITE
                              << std::endl;
                    std::cerr << RED << "The array index value is '" << std::to_string(index_val) << "'." << WHITE
                              << std::endl;
                    std::cerr << "---------------------------------------------------------------" << std::endl;
                    num_errors++;
                }
            }
            std::string array_id_name = node->get_parent()->get_parent()->get_children()[0]->get_data().get_content();
            SymbolTableEntry *array_entry = find_variable_in_scope(symbol_table, current_func, array_id_name, def_area);
            if (array_entry && array_entry->get_stype() == ARRAY) {
                symbol.set_stype(array_entry->get_arr_type());
            } else {
                symbol.set_stype(VOID);
            }
        }
    } else if (head_name == "constant") {
        if (children[0]->get_data().get_name() == "decimal") {
            symbol.set_stype(INT);
            symbol.set_val(children[0]->get_data().get_content());
        } else if (children[0]->get_data().get_name() == "hexadecimal") {
            symbol.set_stype(INT);
            try {
                symbol.set_val(std::to_string(std::stoll(children[0]->get_data().get_content(), nullptr, 16)));
            } catch (const std::out_of_range &oor) {
                symbol.set_val("ERROR_OVERFLOW");
                std::cerr << RED << "Semantic Error [Line " << line_number
                          << "]: Hexadecimal literal out of integer range." << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
        }
//        else if (children[0]->get_data().get_name() == "string") {
//            symbol.set_stype(TYPE_STRING);
//            symbol.set_val(children[0]->get_data().get_content());
//        } else if (children[0]->get_data().get_name() == "character") {
//            symbol.set_stype(UNK);
//            symbol.set_val(children[0]->get_data().get_content());
//        }
        else if (children[0]->get_data().get_name() == "false") {
            symbol.set_stype(BOOL);
            symbol.set_val("false");
        } else if (children[0]->get_data().get_name() == "true") {
            symbol.set_stype(BOOL);
            symbol.set_val("true");
        }
    }
}

void SemanticAnalyzer::make_syntax_tree(Node<Symbol> *node) {
    if (node->get_data().get_type() == TERMINAL && node->get_data().get_name() != "eps") {
        if (node->get_data().get_name() == "print") {
            code += "printf ";
        } else {
            code += node->get_data().get_content() + " ";
        }
        return;
    }
    for (auto child: node->get_children()) {
        make_syntax_tree(child);
    }
}

void SemanticAnalyzer::analyse() {
    dfs(parse_tree.get_root());

    if (!global_function_table.count("main")) {
        std::cerr << RED << "Semantic Error: Program must contain a 'main' function." << WHITE << std::endl;
        std::cerr << "---------------------------------------------------------------" << std::endl;
        num_errors++;
    } else {
        SymbolTableEntry &main_entry = global_function_table["main"];
        if (main_entry.get_stype() != VOID) {
            std::string main_type = semantic_type_to_string[main_entry.get_stype()];
            std::cerr << RED << "Semantic Error: The return type of 'main' function is invalid." << WHITE << std::endl;
            std::cerr << RED << "Expected 'void' type but found '" << main_type << "' type." << WHITE << std::endl;
            std::cerr << "---------------------------------------------------------------" << std::endl;
            num_errors++;
        }
        if (main_entry.get_parameters().size() != 0) {
            std::cerr << RED << "Semantic Error: 'main' function accepts no arguments." << WHITE << std::endl;
            std::cerr << "---------------------------------------------------------------" << std::endl;
            num_errors++;
        }
    }

    if (num_errors == 0) {
        std::cerr << GREEN << "Semantic analyse complete" << WHITE << std::endl;
    } else {
        std::cerr << YELLOW << "Semantic analyse complete with " << num_errors << " errors." << WHITE << std::endl;
    }
}

void SemanticAnalyzer::run_code() {
    if (num_errors == 0) {
        code = "#include <stdio.h>\n#include <stdbool.h>\n";
        make_syntax_tree(parse_tree.get_root());
        std::ofstream out;
        out.open(out_address);
        if (!out.is_open()) {
            std::cerr << RED << "File error: couldn't open output file" << WHITE << std::endl;
            exit(FILE_ERROR);
        }
        out << code;
        out.close();

        std::string command = "gcc " + out_address + " -o " + out_address + ".exe && ./" + out_address + ".exe";
        try {
            int result = std::system(command.c_str());
            if (result != 0) {
                std::cerr << RED << "Compilation or execution failed with code: " << result << WHITE << std::endl;
            }
        } catch (const std::system_error &e) {
            std::cerr << RED << "System command execution failed: " << e.what() << WHITE << std::endl;
        }
    }
}