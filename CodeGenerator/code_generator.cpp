#include "code_generator.h"
#include <utility>

CodeGenerator::CodeGenerator(Tree<Symbol> _ast,
                             std::map<std::string, std::map<std::string, SymbolTableEntry>> _symbol_table,
                             std::string output_file_name) {
    ast = std::move(_ast);
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
            return "int*";
        case TUPLE:
            return "struct tuple";
        default:
            return "int";
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

void CodeGenerator::add_header(const std::string &header) {
    included_headers.insert(header);
}

std::string CodeGenerator::generate_temp_var() {
    return "tmp_" + std::to_string(temp_var_counter++);
}

void CodeGenerator::run() {
    std::string final_code;

    // Add necessary headers
    for (const auto &header: included_headers) {
        final_code += "#include <" + header + ">\n";
    }
    final_code += "\n";

    // Generate tuple type definitions
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

    // Generate forward declarations for functions
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

    // Generate main code
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

std::string CodeGenerator::generate_code(Node<Symbol> *node) {
    if (!node) return "";

    std::string head_name = node->get_data().get_name();
    auto children = node->get_children();
    std::string code;

    if (head_name == "program" || head_name == "func_ls" || head_name == "stmt_ls") {
        for (auto child: children) {
            code += generate_code(child);
        }
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
        // Escape quotes
        size_t pos = 0;
        while ((pos = content.find('\"', pos))) {
            content.replace(pos, 1, "\\\"");
            pos += 2;
        }
        code = "\"" + content + "\"";
    } else if (!children.empty()) {
        return generate_code(children[0]);
    }

    return code;
}

std::string CodeGenerator::generate_function(Node<Symbol> *node) {
    auto children = node->get_children();
    std::string func_name = children[1]->get_data().get_content();
    current_func = func_name;

    // Return type
    semantic_type return_type = VOID;
    if (children.size() > 5 && children[5]->get_children().size() > 1) {
        return_type = children[5]->get_children()[1]->get_data().get_stype();
    }

    std::string code = to_c_type(return_type) + " " + func_name + "(";

    // Function parameters
    if (children.size() > 3) {
        Node<Symbol> *args_node = children[3];
        if (args_node->get_children().size() > 0 &&
            args_node->get_children()[0]->get_data().get_name() != "eps") {
            Node<Symbol> *arg_node = args_node->get_children()[0];
            std::string arg_name = arg_node->get_children()[0]->get_data().get_content();
            semantic_type arg_type = UNK;

            if (arg_node->get_children().size() > 1 &&
                arg_node->get_children()[1]->get_children().size() > 1) {
                arg_type = arg_node->get_children()[1]->get_children()[1]->get_data().get_stype();
            }

            code += to_c_type(arg_type) + " " + arg_name;

            // Process remaining arguments
            Node<Symbol> *args_tail = args_node->get_children()[1];
            while (args_tail->get_children().size() > 0 &&
                   args_tail->get_children()[0]->get_data().get_name() != "eps") {
                arg_node = args_tail->get_children()[1];
                arg_name = arg_node->get_children()[0]->get_data().get_content();
                arg_type = UNK;

                if (arg_node->get_children().size() > 1 &&
                    arg_node->get_children()[1]->get_children().size() > 1) {
                    arg_type = arg_node->get_children()[1]->get_children()[1]->get_data().get_stype();
                }

                code += ", " + to_c_type(arg_type) + " " + arg_name;
                args_tail = args_tail->get_children()[2];
            }
        }
    }

    code += ") {\n";

    // Function body
    if (children.size() > 7) {
        code += generate_code(children[7]);
    }

    // Return statement
    if (children.size() > 8 && children[8]->get_children().size() > 0 &&
        children[8]->get_children()[0]->get_data().get_name() != "eps") {
        code += generate_code(children[8]);
    } else if (return_type != VOID) {
        code += "\treturn 0; // Default return\n";
    }

    code += "}\n\n";
    current_func = "";
    return code;
}

std::string CodeGenerator::generate_variable_declaration(Node<Symbol> *node) {
    auto children = node->get_children();
    std::string code;

    bool is_mutable = (children[1]->get_children().size() > 0 &&
                       children[1]->get_children()[0]->get_data().get_name() == "T_Mut");

    // Handle pattern (single variable or tuple)
    if (children[2]->get_children().size() == 1 &&
        children[2]->get_children()[0]->get_data().get_name() == "T_Id") {
        // Single variable
        std::string var_name = children[2]->get_children()[0]->get_data().get_content();
        semantic_type var_type = UNK;

        // Type annotation
        if (children[3]->get_children().size() > 0 &&
            children[3]->get_children()[0]->get_data().get_name() != "eps") {
            var_type = children[3]->get_children()[1]->get_data().get_stype();
        }

        // Initialization
        std::string init_value;
        if (children[4]->get_children().size() > 0 &&
            children[4]->get_children()[0]->get_data().get_name() != "eps") {
            init_value = generate_expression(children[4]->get_children()[1]);

            // If type wasn't specified, infer from initialization
            if (var_type == UNK) {
                if (children[4]->get_children()[1]->get_data().get_stype() != UNK) {
                    var_type = children[4]->get_children()[1]->get_data().get_stype();
                }
            }
        }

        // اصلاح این بخش
        code = std::string("\t") + (is_mutable ? "" : "const ") + to_c_type(var_type) + " " + var_name;
        if (!init_value.empty()) {
            code += " = " + init_value;
        }
        code += ";\n";
    } else {
        // Tuple pattern
        Node<Symbol> *id_ls = children[2]->get_children()[1];
        std::vector<std::string> var_names;

        // Collect variable names
        var_names.push_back(id_ls->get_children()[0]->get_data().get_content());
        Node<Symbol> *id_ls_tail = id_ls->get_children()[1];
        while (id_ls_tail->get_children().size() > 0 &&
               id_ls_tail->get_children()[0]->get_data().get_name() != "eps") {
            var_names.push_back(id_ls_tail->get_children()[1]->get_data().get_content());
            id_ls_tail = id_ls_tail->get_children()[2];
        }

        // Generate tuple initialization
        if (children[4]->get_children().size() > 0 &&
            children[4]->get_children()[0]->get_data().get_name() != "eps") {
            std::string tuple_init = generate_expression(children[4]->get_children()[1]);

            // Generate individual variable assignments
            for (size_t i = 0; i < var_names.size(); ++i) {
                // اصلاح این بخش
                code += std::string("\t") + (is_mutable ? "" : "const ") + "int " + var_names[i] +
                        " = ((" + tuple_init + ").item" + std::to_string(i) + ");\n";
            }
        } else {
            // No initialization, just declare variables
            for (const auto &name: var_names) {
                // اصلاح این بخش
                code += std::string("\t") + (is_mutable ? "" : "const ") + "int " + name + ";\n";
            }
        }
    }

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
                    // Function call
                    code = generate_function_call(node);
                } else if (children[1]->get_children()[0]->get_data().get_name() == "T_LB") {
                    // Array access
                    code = generate_array_access(node);
                } else {
                    // Simple variable
                    code = generate_code(children[0]);
                }
            } else {
                // Simple variable
                code = generate_code(children[0]);
            }
        } else if (factor_type == "T_LP") {
            // Parentheses or tuple
            if (children[1]->get_children().size() > 1 &&
                children[1]->get_children()[1]->get_data().get_name() == "T_Comma") {
                // Tuple
                code = generate_tuple_access(node);
            } else {
                // Simple parentheses
                code = "(" + generate_code(children[1]) + ")";
            }
        } else if (factor_type == "T_LOp_NOT") {
            // Logical NOT
            code = "!(" + generate_code(children[1]) + ")";
        } else {
            // Literal value
            code = generate_code(children[0]);
        }
    } else if (head_name == "log_exp") {
        code = generate_code(children[0]);
        if (children.size() > 1 && children[1]->get_children().size() > 0 &&
            children[1]->get_children()[0]->get_data().get_name() != "eps") {
            code += " || " + generate_code(children[1]->get_children()[1]);
        }
    } else if (head_name == "rel_exp") {
        code = generate_code(children[0]);
        if (children.size() > 1 && children[1]->get_children().size() > 0 &&
            children[1]->get_children()[0]->get_data().get_name() != "eps") {
            code += " && " + generate_code(children[1]->get_children()[1]);
        }
    } else if (head_name == "eq_exp") {
        code = generate_code(children[0]);
        if (children.size() > 1 && children[1]->get_children().size() > 0 &&
            children[1]->get_children()[0]->get_data().get_name() != "eps") {
            std::string op = children[1]->get_children()[0]->get_data().get_content();
            if (op == "T_ROp_E") op = " == ";
            else if (op == "T_ROp_NE") op = " != ";
            code += op + generate_code(children[1]->get_children()[1]);
        }
    } else if (head_name == "cmp_exp") {
        code = generate_code(children[0]);
        if (children.size() > 1 && children[1]->get_children().size() > 0 &&
            children[1]->get_children()[0]->get_data().get_name() != "eps") {
            std::string op = children[1]->get_children()[0]->get_data().get_content();
            if (op == "T_ROp_L") op = " < ";
            else if (op == "T_ROp_LE") op = " <= ";
            else if (op == "T_ROp_G") op = " > ";
            else if (op == "T_ROp_GE") op = " >= ";
            code += op + generate_code(children[1]->get_children()[1]);
        }
    } else if (head_name == "arith_exp" || head_name == "arith_term") {
        code = generate_code(children[0]);
        if (children.size() > 1 && children[1]->get_children().size() > 0 &&
            children[1]->get_children()[0]->get_data().get_name() != "eps") {
            std::string op = children[1]->get_children()[0]->get_data().get_content();
            if (op == "T_AOp_Trust") op = " + ";
            else if (op == "T_AOp_MN") op = " - ";
            else if (op == "T_AOp_ML") op = " * ";
            else if (op == "T_AOp_DV") op = " / ";
            else if (op == "T_AOp_RM") op = " % ";
            code += op + generate_code(children[1]->get_children()[1]);
        }
    } else if (head_name == "exp_ls") {
        if (!children.empty() && children[0]->get_data().get_name() != "eps") {
            code += generate_code(children[0]);
            code += generate_code(children[1]);
        }
    } else if (head_name == "exp_ls_tail") {
        if (!children.empty() && children[0]->get_data().get_name() != "eps") {
            code += ", " + generate_code(children[1]);
            code += generate_code(children[2]);
        }
    } else {
        code = generate_code(children[0]);
    }

    return code;
}

std::string CodeGenerator::generate_function_call(Node<Symbol> *node) {
    auto children = node->get_children();
    std::string func_name = children[0]->get_data().get_content();

    std::string code = func_name + "(";
    if (children.size() > 1 && children[1]->get_children().size() > 0 &&
        children[1]->get_children()[0]->get_data().get_name() != "eps") {
        Node<Symbol> *exp_ls = children[1]->get_children()[1];
        code += generate_code(exp_ls);
    }
    code += ")";
    return code;
}

std::string CodeGenerator::generate_array_access(Node<Symbol> *node) {
    auto children = node->get_children();
    std::string array_name = children[0]->get_data().get_content();
    std::string index = generate_code(children[1]->get_children()[1]);

    return array_name + "[" + index + "]";
}

std::string CodeGenerator::generate_tuple_access(Node<Symbol> *node) {
    auto children = node->get_children();
    std::string temp_var = generate_temp_var();
    std::string code;

    // Generate tuple value
    std::string tuple_value;
    if (children[1]->get_children().size() > 0 &&
        children[1]->get_children()[0]->get_data().get_name() != "eps") {
        tuple_value = generate_code(children[1]->get_children()[0]);

        if (children[1]->get_children().size() > 1 &&
            children[1]->get_children()[1]->get_data().get_name() == "T_Comma") {
            Node<Symbol> *exp_ls = children[1]->get_children()[1];
            tuple_value += ", " + generate_code(exp_ls->get_children()[1]);
        }
    }

    // Create temporary struct
    code = "(struct tuple){" + tuple_value + "}";
    return code;
}

std::string CodeGenerator::generate_println(Node<Symbol> *node) {
    auto children = node->get_children();
    Node<Symbol> *args_node = children[2];
    std::string code;

    if (args_node->get_children().size() > 0) {
        if (args_node->get_children()[0]->get_data().get_name() == "T_String") {
            std::string format_str = args_node->get_children()[0]->get_data().get_content();

            // Replace {} with %d for printf format
            size_t pos = format_str.find("{}");
            while (pos != std::string::npos) {
                format_str.replace(pos, 2, "%d");
                pos = format_str.find("{}", pos + 2);
            }

            code = "\tprintf(\"" + format_str + "\\n\"";

            // Add format arguments
            if (args_node->get_children().size() > 1 &&
                args_node->get_children()[1]->get_children().size() > 0) {
                Node<Symbol> *format_args = args_node->get_children()[1];
                while (format_args->get_children().size() > 0 &&
                       format_args->get_children()[0]->get_data().get_name() != "eps") {
                    code += ", " + generate_expression(format_args->get_children()[1]);
                    format_args = format_args->get_children()[2];
                }
            }

            code += ");\n";
        } else {
            // Simple case without formatting
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
        code += generate_code(children[3]); // then block

        // else part
        if (children.size() > 4 && children[4]->get_children().size() > 0) {
            Node<Symbol> *else_node = children[4]->get_children()[1];
            if (else_node->get_data().get_name() == "if_stmt") {
                code += "\t} else " + generate_control_structures(else_node);
            } else {
                code += "\t} else {\n";
                code += generate_code(else_node);
                code += "\t}\n";
            }
        } else {
            code += "\t}\n";
        }
    } else if (head_name == "loop_stmt") {
        code = "\twhile (1) {\n";
        code += generate_code(children[1]); // loop body
        code += "\t}\n";
    } else if (head_name == "break_stmt") {
        code = "\tbreak;\n";
    } else if (head_name == "continue_stmt") {
        code = "\tcontinue;\n";
    }

    return code;
}