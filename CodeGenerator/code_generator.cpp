#include "code_generator.h"

#include <utility>

CodeGenerator::CodeGenerator(Tree<Symbol> _ast,
                             std::map<std::string, std::map<std::string, SymbolTableEntry>> _symbol_table,
                             std::string output_file_name) {
    ast = _ast;
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

    std::vector<std::string> child_codes;
    child_codes.reserve(children.size());
    for (auto child: children) {
        child_codes.push_back(generate_code(child));
    }

    if (head_name == "program" || head_name == "func_ls" || head_name == "stmt_ls") {
        for (const auto &child_code: child_codes) {
            code += child_code;
        }
    } else if (head_name == "func") {
        std::string func_name = children[1]->get_data().get_content();
        this->current_func = func_name;
        semantic_type return_type = symbol_table[""][func_name].get_stype();

        code += to_c_type(return_type) + " " + func_name + "(";
        code += child_codes[3];
        code += ") {\n";
        code += child_codes[7];
        code += child_codes[8];

        if (func_name == "main" && return_type == VOID) {
            code += "\treturn 0;\n";
        }
        code += "}\n\n";
        this->current_func = "";
    } else if (head_name == "var_declaration") {
        std::string var_name = children[2]->get_children()[0]->get_data().get_content();
        semantic_type stype = symbol_table[current_func][var_name].get_stype();
        code += "\t" + to_c_type(stype) + " " + var_name;
        if (children[4]->get_children()[0]->get_data().get_name() != "eps") {
            code += " = " + child_codes[4];
        }
        code += ";\n";
    } else if (head_name == "assign_opt" || head_name == "exp") {
        if (!child_codes.empty()) code += child_codes[0];
    } else if (head_name == "arith_exp") {
        code = child_codes[0];
        Node<Symbol> *tail_node = children[1];
        if (tail_node->get_children()[0]->get_data().get_name() != "eps") {
            std::string op = tail_node->get_children()[0]->get_data().get_content();
            std::string right_code = generate_code(tail_node->get_children()[1]);
            code = "(" + code + " " + op + " " + right_code + ")";
        }
    } else if (head_name == "T_Id" || head_name == "T_Decimal") {
        code = node->get_data().get_content();
    }

    return code;
}