#include "code_generator.h"
#include <utility>

CodeGenerator::CodeGenerator(Tree<Symbol> _ast,
                             std::map<std::string, std::map<std::string, SymbolTableEntry>> _symbol_table,
                             std::string output_file_name) {
    ast = _ast;
    symbol_table = std::move(_symbol_table);
    out_address = std::move(output_file_name);
    current_func = "";
    temp_var_counter = 0;
    included_headers = {"stdio.h", "stdlib.h", "stdbool.h"};
}

std::string CodeGenerator::to_c_type(semantic_type stype) {
    switch (stype) {
        case INT:
            return "int";
        case BOOL:
            return "bool";
        case VOID:
            return "void";
        case ARRAY:
            // This is now mainly a fallback for pointers, as declarations are handled specially.
            return "void*";
        case TUPLE:
            return "struct tuple";
        default:
            return "int"; // Default for UNK type
    }
}

std::string CodeGenerator::to_c_type(const std::vector<semantic_type> &types) {
    if (types.empty()) return "void";

    std::stringstream ss;
    ss << "struct tuple_" << types.size() << " {";
    for (size_t i = 0; i < types.size(); ++i) {
        ss << to_c_type(types[i]) << " item" << i << ";";
    }
    ss << "}";
    return ss.str();
}

std::string CodeGenerator::indent_block(const std::string &block_code) {
    std::string result;
    std::istringstream stream(block_code);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty()) {
            result += "\t" + line + "\n";
        }
    }
    return result;
}

std::string CodeGenerator::generate_assignment_or_call_statement(Node<Symbol> *stmt_node) {
    auto children = stmt_node->get_children();
    // Structure: <stmt> -> T_Id <stmt_after_id> T_Semicolon

    std::string identifier = children[0]->get_data().get_content();
    Node<Symbol> *after_id_node = children[1];
    Node<Symbol> *first_child_of_after = after_id_node->get_children()[0];
    std::string rule_type = first_child_of_after->get_data().get_name();

    std::string code = "\t";

    if (rule_type == "T_Assign") {
        // Simple assignment: identifier = expression;
        code += identifier + " = " + generate_expression(after_id_node->get_children()[1]);
    } else if (rule_type == "T_LB") {
        // Array element assignment: identifier[index_expression] = value_expression;
        // Grammar: T_LB <exp> T_RB T_Assign <exp>
        code += identifier
                + "[" + generate_expression(after_id_node->get_children()[1]) + "]"
                + " = "
                + generate_expression(after_id_node->get_children()[4]);
    } else if (rule_type == "T_LP") {
        // Function call as a statement. We can reuse the existing function call generator.
        // We just need to build a fake <arith_factor> node for it.
        // Or more simply, build the string manually.
        std::string func_call_str = identifier + "(";
        Node<Symbol> *exp_ls_call_node = after_id_node->get_children()[1];
        if (!exp_ls_call_node->get_children().empty() &&
            exp_ls_call_node->get_children()[0]->get_data().get_name() != "eps") {
            func_call_str += generate_expression(exp_ls_call_node->get_children()[0]);
        }
        func_call_str += ")";
        code += func_call_str;
    }

    code += ";\n";
    return code;
}

std::string CodeGenerator::generate_code(Node<Symbol> *node) {
    if (!node) return "";

    std::string head_name = node->get_data().get_name();
    auto children = node->get_children();
    std::string code;

    if (head_name == "program" || head_name == "func_ls" || head_name == "stmt_ls") {
        for (auto child: children) {
            code += generate_code(child);
        }
    } else if (head_name == "stmt" && !children.empty() && children[0]->get_data().get_name() == "T_Id") {
        code = generate_assignment_or_call_statement(node);
    } else if (head_name == "func") {
        code = generate_function(node);
    } else if (head_name == "var_declaration") {
        code = generate_variable_declaration(node);
    } else if (head_name == "println_stmt") {
        code = generate_println(node);
    } else if (head_name == "if_stmt" || head_name == "loop_stmt" ||
               head_name == "break_stmt" || head_name == "continue_stmt") {
        code = generate_control_structures(node);
    } else if (head_name == "return_stmt") {
        if (!children.empty() && children[0]->get_data().get_name() != "eps") {
            code = "\treturn " + generate_expression(children[1]) + ";\n";
        }
    } else if (head_name == "exp" || head_name == "log_exp" || head_name == "rel_exp" ||
               head_name == "eq_exp" || head_name == "cmp_exp" || head_name == "arith_exp" ||
               head_name == "arith_term" || head_name == "arith_factor") {
        code = generate_expression(node);
    } else if (head_name == "T_Id" || head_name == "T_Decimal" || head_name == "T_Hexadecimal") {
        code = node->get_data().get_content();
    } else if (head_name == "T_True") {
        code = "true";
    } else if (head_name == "T_False") {
        code = "false";
    } else if (head_name == "T_String") {
        std::string content = node->get_data().get_content();
        size_t pos = 0;
        while ((pos = content.find('\"', pos))) {
            content.replace(pos, 1, "\\\"");
            pos += 2;
        }
        code = "\"" + content + "\"";
    } else if (!children.empty()) {
        for (auto child: children) {
            code += generate_code(child);
        }
    }

    return code;
}

std::string CodeGenerator::generate_variable_declaration(Node<Symbol> *node) {
    auto children = node->get_children();
    std::string code;

    bool is_mutable = !children[1]->get_children().empty() &&
                      children[1]->get_children()[0]->get_data().get_name() == "T_Mut";

    Node<Symbol> *pattern_node = children[2];

    // This handles single variable declarations (`let x ...`).
    // It does not handle tuple destructuring (`let (x,y) ...`) yet.
    if (!pattern_node->get_children().empty() && pattern_node->get_children()[0]->get_data().get_name() == "T_Id") {
        std::string var_name = pattern_node->get_children()[0]->get_data().get_content();

        // Retrieve the variable's information from the symbol table.
        // This assumes SemanticAnalyzer has already run and populated the table correctly.
        SymbolTableEntry var_entry = symbol_table[current_func][var_name];
        semantic_type var_type = var_entry.get_stype();

        code += "\t";
        if (!is_mutable) {
            code += "const ";
        }

        // Generate the type and name part of the declaration
        if (var_type == ARRAY) {
            semantic_type element_type = var_entry.get_arr_type();
            int array_len = var_entry.get_arr_len();
            code += to_c_type(element_type) + " " + var_name + "[" + std::to_string(array_len) + "]";
        } else {
            // This handles INT, BOOL, and other simple types.
            // If SemanticAnalyzer is fixed, this will correctly use 'int' for 'y'.
            code += to_c_type(var_type) + " " + var_name;
        }

        // Handle initialization if it exists
        Node<Symbol> *assign_opt_node = children[4];
        if (!assign_opt_node->get_children().empty() &&
            assign_opt_node->get_children()[0]->get_data().get_name() != "eps") {
            code += " = " + generate_expression(assign_opt_node->get_children()[1]);
        }

        code += ";\n";
    }
    // else: A place to add logic for tuple patterns if needed in the future.

    return code;
}

void CodeGenerator::run() {
    std::string final_code;

    for (const auto &header: included_headers) {
        final_code += "#include <" + header + ">\n";
    }
    final_code += "\n";

    std::set<std::vector<semantic_type>> tuple_types;
    for (const auto &scope: symbol_table) {
        for (const auto &symbol: scope.second) {
            const auto &tuple_types_vec = symbol.second.get_tuple_types();
            if (!tuple_types_vec.empty()) {
                tuple_types.insert(tuple_types_vec);
            }
        }
    }

    for (const auto &types: tuple_types) {
        final_code += to_c_type(types) + ";\n";
    }
    final_code += "\n";

    auto funcs = symbol_table[""];
    for (const auto &func: funcs) {
        if (func.first != "main") {
            semantic_type return_type = func.second.get_stype();
            final_code += to_c_type(return_type) + " " + func.first + "(";

            const auto &params = func.second.get_parameters();
            for (size_t i = 0; i < params.size(); ++i) {
                final_code += to_c_type(params[i].second) + " " + params[i].first;
                if (i != params.size() - 1) {
                    final_code += ", ";
                }
            }
            final_code += ");\n";
        }
    }
    final_code += "\n";

    std::string generated_body = generate_code(ast.get_root());
    final_code += generated_body;

    std::ofstream out_file(out_address);
    if (out_file.is_open()) {
        out_file << final_code;
        out_file.close();
        std::cout << "C code generated successfully and saved to " << out_address << std::endl;
    } else {
        std::cerr << "Error: Could not open file to write C code." << std::endl;
    }
}

std::string CodeGenerator::generate_function(Node<Symbol> *node) {
    auto children = node->get_children();
    std::string func_name = children[1]->get_data().get_content();
    current_func = func_name;

    semantic_type return_type = symbol_table[""][func_name].get_stype();
    // Special case for main, which often returns int in C
    std::string c_return_type = (func_name == "main") ? "void" : to_c_type(return_type);

    std::string code = c_return_type + " " + func_name + "(";

    const auto &params = symbol_table[""][func_name].get_parameters();
    for (size_t i = 0; i < params.size(); ++i) {
        code += to_c_type(params[i].second) + " " + params[i].first;
        if (i != params.size() - 1) {
            code += ", ";
        }
    }

    code += ") {\n";

    // Function body is child 6 (<stmt_ls>) in your grammar `T_Fn T_Id T_LP <func_args> T_RP <func_type> T_LC <stmt_ls> <return_stmt> T_RC`
    if (children.size() > 7) {
        code += generate_code(children[7]);
    }

    // Return statement
    if (children.size() > 8 && !children[8]->get_children().empty() &&
        children[8]->get_children()[0]->get_data().get_name() != "eps") {
        code += generate_code(children[8]);
    } else if (func_name == "main" && c_return_type == "int") {
        code += "\treturn 0;\n";
    }

    code += "}\n\n";
    current_func = "";
    return code;
}


std::string CodeGenerator::generate_expression(Node<Symbol> *node) {
    if (!node) return "";

    std::string head_name = node->get_data().get_name();
    auto children = node->get_children();
    std::string code;

    if (head_name == "arith_factor") {
        std::string factor_type = children[0]->get_data().get_name();
        if (factor_type == "T_Id") {
            if (children.size() > 1 && !children[1]->get_children().empty()) {
                if (children[1]->get_children()[0]->get_data().get_name() == "T_LP") {
                    code = generate_function_call(node);
                } else if (children[1]->get_children()[0]->get_data().get_name() == "T_LB") {
                    code = generate_array_access(node);
                } else {
                    code = generate_code(children[0]);
                }
            } else {
                code = generate_code(children[0]);
            }
        } else if (factor_type == "T_LP") {
            if (children[1]->get_children().size() > 1 &&
                children[1]->get_children()[1]->get_data().get_name() == "T_Comma") {
                code = generate_tuple_access(node);
            } else {
                code = "(" + generate_code(children[1]) + ")";
            }
        } else if (factor_type == "T_LOp_NOT") {
            code = "!" + generate_code(children[1]);
        } else if (factor_type == "T_AOp_MN") {
            code = "-" + generate_expression(children[1]);
        } else {
            code = generate_code(children[0]);
        }
    } else if (head_name == "log_exp") {
        code = generate_code(children[0]);
        Node<Symbol> *tail = children[1];
        while (tail && !tail->get_children().empty() && tail->get_children()[0]->get_data().get_name() != "eps") {
            code += " || " + generate_code(tail->get_children()[1]);
            tail = tail->get_children()[2];
        }
    } else if (head_name == "rel_exp") {
        code = generate_code(children[0]);
        Node<Symbol> *tail = children[1];
        while (tail && !tail->get_children().empty() && tail->get_children()[0]->get_data().get_name() != "eps") {
            code += " && " + generate_code(tail->get_children()[1]);
            tail = tail->get_children()[2];
        }
    } else if (head_name == "eq_exp") {
        code = generate_code(children[0]);
        Node<Symbol> *tail = children[1];
        while (tail && !tail->get_children().empty() && tail->get_children()[0]->get_data().get_name() != "eps") {
            std::string op = tail->get_children()[0]->get_data().get_name();
            if (op == "T_ROp_E") op = " == ";
            else if (op == "T_ROp_NE") op = " != ";
            code += op + generate_code(tail->get_children()[1]);
            tail = tail->get_children()[2];
        }
    } else if (head_name == "cmp_exp") {
        code = generate_code(children[0]);
        if (children.size() > 1 && !children[1]->get_children().empty() &&
            children[1]->get_children()[0]->get_data().get_name() != "eps") {
            Node<Symbol> *op_node = children[1]->get_children()[0]->get_children()[0];
            std::string op = op_node->get_data().get_name();
            if (op == "T_ROp_L") op = " < ";
            else if (op == "T_ROp_LE") op = " <= ";
            else if (op == "T_ROp_G") op = " > ";
            else if (op == "T_ROp_GE") op = " >= ";
            code += op + generate_code(children[1]->get_children()[1]);
        }
    } else if (head_name == "arith_exp" || head_name == "arith_term") {
        code = generate_code(children[0]);
        Node<Symbol> *tail = children[1];
        while (tail && !tail->get_children().empty() && tail->get_children()[0]->get_data().get_name() != "eps") {
            std::string op = tail->get_children()[0]->get_data().get_name();
            if (op == "T_AOp_Trust") op = " + ";
            else if (op == "T_AOp_MN") op = " - ";
            else if (op == "T_AOp_ML") op = " * ";
            else if (op == "T_AOp_DV") op = " / ";
            else if (op == "T_AOp_RM") op = " % ";
            code += op + generate_code(tail->get_children()[1]);
            tail = tail->get_children()[2];
        }
    } else if (head_name == "exp_ls" || head_name == "pure_exp_ls") {
        if (!children.empty() && children[0]->get_data().get_name() != "eps") {
            code += generate_code(children[0]);
            Node<Symbol> *tail = children[1];
            while (tail && !tail->get_children().empty() && tail->get_children()[0]->get_data().get_name() != "eps") {
                code += ", " + generate_code(tail->get_children()[1]);
                tail = tail->get_children()[2];
            }
        }
    } else if (!children.empty()) {
        code = generate_code(children[0]);
    }

    return code;
}

std::string CodeGenerator::generate_function_call(Node<Symbol> *node) {
    auto children = node->get_children(); // factor -> T_Id fac_id_opt
    std::string func_name = children[0]->get_data().get_content();

    std::string code = func_name + "(";

    // fac_id_opt -> T_LP <exp_ls_call> T_RP
    Node<Symbol> *exp_ls_call_node = children[1]->get_children()[1];
    if (!exp_ls_call_node->get_children().empty() &&
        exp_ls_call_node->get_children()[0]->get_data().get_name() != "eps") {
        code += generate_expression(exp_ls_call_node->get_children()[0]);
    }

    code += ")";
    return code;
}

std::string CodeGenerator::generate_array_access(Node<Symbol> *node) {
    auto children = node->get_children();
    std::string array_name = children[0]->get_data().get_content();
    // fac_id_opt -> T_LB <exp> T_RB
    std::string index = generate_expression(children[1]->get_children()[1]);

    return array_name + "[" + index + "]";
}

std::string CodeGenerator::generate_tuple_access(Node<Symbol> *node) {
    // fac_lparen -> <exp> <lpar_exp_suf>
    // lpar_exp_suf -> T_Comma <pure_exp_ls> T_RP
    auto children = node->get_children();
    std::string code = "(struct tuple){";
    code += generate_expression(children[0]); // first expression

    Node<Symbol> *pure_exp_ls_node = children[1]->get_children()[1]; // <pure_exp_ls>
    if (!pure_exp_ls_node->get_children().empty() &&
        pure_exp_ls_node->get_children()[0]->get_data().get_name() != "eps") {
        code += ", " + generate_expression(pure_exp_ls_node);
    }

    code += "}";
    return code;
}

std::string CodeGenerator::generate_println(Node<Symbol> *node) {
    auto children = node->get_children(); // println_stmt -> T_Print T_LP <println_args> T_RP T_Semicolon
    Node<Symbol> *args_node = children[2];
    std::string code;

    if (!args_node->get_children().empty()) {
        if (args_node->get_children()[0]->get_data().get_name() == "T_String") {
            std::string format_str = args_node->get_children()[0]->get_data().get_content();

            size_t pos = 0;
            while ((pos = format_str.find("{}", pos)) != std::string::npos) {
                format_str.replace(pos, 2, "%d");
                pos += 2;
            }

            code = "\tprintf(\"" + format_str + "\\n\"";

            if (args_node->get_children().size() > 1 && !args_node->get_children()[1]->get_children().empty() &&
                args_node->get_children()[1]->get_children()[0]->get_data().get_name() != "eps") {
                Node<Symbol> *format_args_list = args_node->get_children()[1]->get_children()[1];
                while (format_args_list && !format_args_list->get_children().empty()) {
                    code += ", " + generate_expression(format_args_list->get_children()[0]);
                    format_args_list = format_args_list->get_children()[1]; // tail
                    if (format_args_list && !format_args_list->get_children().empty()) {
                        format_args_list = format_args_list->get_children()[1]; // item
                    }
                }
            }

            code += ");\n";
        } else {
            code = "\tprintf(\"%d\\n\", " + generate_expression(args_node->get_children()[0]) + ");\n";
        }
    }

    return code;
}

std::string CodeGenerator::generate_control_structures(Node<Symbol> *node) {
    std::string head_name = node->get_data().get_name();
    auto children = node->get_children();
    std::string code;

    if (head_name == "if_stmt") {
        code = "\tif (" + generate_expression(children[1]) + ") {\n";

        code += indent_block(generate_code(children[3]));

        Node<Symbol> *else_opt_node = (children.size() > 5) ? children[5] : nullptr;
        if (else_opt_node && !else_opt_node->get_children().empty() &&
            else_opt_node->get_children()[0]->get_data().get_name() != "eps") {
            Node<Symbol> *else_alternative_node = else_opt_node->get_children()[1];

            if (else_alternative_node->get_children()[0]->get_data().get_name() == "if_stmt") {

                code += "\t} else ";

                std::string else_if_block = generate_code(else_alternative_node->get_children()[0]);
                if (!else_if_block.empty() && else_if_block[0] == '\t') {
                    else_if_block.erase(0, 1);
                }
                code += else_if_block;

            } else {
                code += "\t} else {\n";
                code += indent_block(generate_code(else_alternative_node->get_children()[1]));
                code += "\t}\n";
            }
        } else {
            code += "\t}\n";
        }
    } else if (head_name == "loop_stmt") {
        code = "\twhile (1) {\n";
        code += generate_code(children[2]);
        code += "\t}\n";
    } else if (head_name == "break_stmt") {
        code = "\tbreak;\n";
    } else if (head_name == "continue_stmt") {
        code = "\tcontinue;\n";
    }

    return code;
}