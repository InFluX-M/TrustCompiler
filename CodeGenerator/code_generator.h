#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H

#include "../utils.h"
#include "../SemanticAnalyzer/semantic_analyzer.h"
#include <fstream>

class CodeGenerator {
private:
    Tree<Symbol> ast;
    std::string out_address;
    std::map<std::string, std::map<std::string, SymbolTableEntry>> symbol_table;
    std::string current_func;

    std::string generate_code(Node<Symbol> *node);

    static std::string to_c_type(semantic_type stype);

public:
    CodeGenerator(Tree<Symbol> _ast,
                  std::map<std::string, std::map<std::string, SymbolTableEntry>> _symbol_table,
                  std::string output_file_name);

    void run();
};

#endif //CODE_GENERATOR_H