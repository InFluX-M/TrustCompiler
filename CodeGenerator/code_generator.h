#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H

#include "../utils.h"
#include "../SemanticAnalyzer/semantic_analyzer.h"
#include <fstream>
#include <vector>
#include <set>
#include <sstream>

class CodeGenerator {
private:
    Tree<Symbol> ast;
    std::string out_address;
    std::map<std::string, std::map<std::string, SymbolTableEntry>> symbol_table;
    std::string current_func;
    std::set<std::string> included_headers;
    int temp_var_counter;

    // Helper functions
    std::string to_c_type(semantic_type stype);

    std::string to_c_type(const std::vector<semantic_type> &types);

    void add_header(const std::string &header);

    std::string generate_temp_var();

    // Main generation functions
    std::string generate_code(Node<Symbol> *node);

    std::string generate_function(Node<Symbol> *node);

    std::string generate_variable_declaration(Node<Symbol> *node);

    std::string generate_expression(Node<Symbol> *node);

    std::string generate_println(Node<Symbol> *node);

    std::string generate_control_structures(Node<Symbol> *node);

    std::string generate_array_declaration(Node<Symbol> *node);

    std::string generate_tuple_declaration(Node<Symbol> *node);

    std::string generate_function_call(Node<Symbol> *node);

    std::string generate_array_access(Node<Symbol> *node);

    std::string generate_tuple_access(Node<Symbol> *node);

public:
    CodeGenerator(Tree<Symbol> _ast,
                  std::map<std::string, std::map<std::string, SymbolTableEntry>> _symbol_table,
                  std::string output_file_name);

    void run();
};

#endif //CODE_GENERATOR_H