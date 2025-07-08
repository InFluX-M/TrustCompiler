#include "code_generator.h"
#include <utility>

CodeGenerator::CodeGenerator(Tree<Symbol> _ast,
                             std::map<std::string, std::map<std::string, SymbolTableEntry>> _symbol_table,
                             std::string output_file_name) {
    ast = std::move(_ast);
    symbol_table = std::move(_symbol_table);
    out_address = std::move(output_file_name);
    current_func = "";
}

std::string CodeGenerator::to_c_type(semantic_type stype) {
    if (stype == INT) return "int";
    if (stype == BOOL) return "int";
    if (stype == VOID) return "void";
    return "/* unknown type */";
}

void CodeGenerator::run() {
    std::string final_code;
    std::string generated_body = generate_code(ast.get_root());

    final_code += "#include <stdio.h>\n";
    final_code += "#include <stdlib.h>\n\n";
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

    // --- Program Structure & Lists ---
    if (head_name == "program" || head_name == "func_ls" || head_name == "stmt_ls") {
        for (auto child: children) {
            code += generate_code(child);
        }
    }
        // --- Function Definition ---
    else if (head_name == "func") {
        std::string func_name = children[1]->get_data().get_content();
        this->current_func = func_name;
        semantic_type return_type = symbol_table[""][func_name].get_stype();

        code += to_c_type(return_type) + " " + func_name + "(";
        code += generate_code(children[3]); // <func_args>
        code += ") {\n";
        code += generate_code(children[7]); // <stmt_ls>
        code += generate_code(children[8]); // <return_stmt>

        if (func_name == "main" && return_type == VOID) {
            code += "\treturn 0;\n";
        }
        code += "}\n\n";
        this->current_func = "";
    }
        // --- Function Arguments ---
    else if (head_name == "func_args") {
        if (children.empty() || children[0]->get_data().get_name() == "eps") return "";
        code += generate_code(children[0]); // <arg>
        code += generate_code(children[1]); // <func_args_tail>
    } else if (head_name == "func_args_tail") {
        if (children.empty() || children[0]->get_data().get_name() == "eps") return "";
        code += ", " + generate_code(children[1]); // <arg>
        code += generate_code(children[2]);       // <func_args_tail>
    }
    else if (head_name == "arg") {
        std::string arg_name = children[0]->get_data().get_content();

        // Look in the global symbol table entry for the current function
        SymbolTableEntry &func_entry = symbol_table[""][current_func];
        semantic_type arg_type = UNK; // Default
        // Find the parameter by name to get its type
        for (const auto &param: func_entry.get_parameters()) {
            if (param.first == arg_name) {
                arg_type = param.second;
                break;
            }
        }
        code += to_c_type(arg_type) + " " + arg_name;
    }
        // --- Statements ---
    else if (head_name == "var_declaration") {
        std::string var_name = children[2]->get_children()[0]->get_data().get_content();
        semantic_type stype = symbol_table[current_func][var_name].get_stype();
        code += "\t" + to_c_type(stype) + " " + var_name;

        if (children[4]->get_children()[0]->get_data().get_name() != "eps") {
            code += " = " + generate_code(children[4]);
        }
        code += ";\n";
    } else if (head_name == "return_stmt") {
        if (children.empty() || children[0]->get_data().get_name() == "eps") return "";
        code += "\treturn " + generate_code(children[1]) + ";\n";
    } else if (head_name == "println_stmt") {
        Node<Symbol> *println_args_node = children[2];
        Node<Symbol> *arg_node = println_args_node->get_children()[0];
        code += "\tprintf(\"%d\\n\", " + generate_code(arg_node) + ");\n";
    }
        // --- Expressions ---
    else if (head_name == "assign_opt" || head_name == "exp" || head_name == "log_exp" || head_name == "rel_exp" ||
             head_name == "eq_exp" || head_name == "cmp_exp" || head_name == "arith_exp" || head_name == "arith_term") {
        if (children.empty()) return "";
        code = generate_code(children[0]);
    }
        // --- Expression Factors & Literals ---
    else if (head_name == "arith_factor") {
        std::string factor_type = children[0]->get_data().get_name();
        if (factor_type == "T_Id") {
            if (children.size() > 1 && !children[1]->get_children().empty() &&
                children[1]->get_children()[0]->get_data().get_name() == "T_LP") {
                code = generate_code(children[0]) + "(" + generate_code(children[1]->get_children()[1]) + ")";
            } else {
                code = generate_code(children[0]);
            }
        } else if (factor_type == "T_LP") {
            code = "(" + generate_code(children[1]) + ")";
        } else {
            code = generate_code(children[0]);
        }
    }
    else if (head_name == "exp_ls_call") {
        if (children.empty() || children[0]->get_data().get_name() == "eps") return "";
        // Just pass through to the <exp_ls> node
        code = generate_code(children[0]);
    } else if (head_name == "exp_ls") {
        if (children.empty() || children[0]->get_data().get_name() == "eps") return "";
        // Handle the first argument
        code += generate_code(children[0]);
        // Handle the rest of the arguments
        code += generate_code(children[1]);
    } else if (head_name == "exp_ls_tail") {
        if (children.empty() || children[0]->get_data().get_name() == "eps") return "";
        // children[0] is T_Comma, children[1] is <arg_item>, children[2] is <exp_ls_tail>
        code += ", " + generate_code(children[1]);
        code += generate_code(children[2]);
    } else if (head_name == "arg_item") {
        if (children.empty()) return "";
        code = generate_code(children[0]); // <exp>
    }
        // --- Terminals ---
    else if (head_name == "T_Id" || head_name == "T_Decimal") {
        code = node->get_data().get_content();
    } else if (head_name == "T_True") {
        code = "1";
    } else if (head_name == "T_False") {
        code = "0";
    }
        // --- Fallback for simple pass-through rules ---
    else if (!children.empty()) {
        return generate_code(children[0]);
    }

    return code;
}