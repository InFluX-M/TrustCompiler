#include "LexicalAnalyzer/lexical_analyzer.h"
#include "SyntaxAnalyzer/syntax_analyzer.h"
#include "SemanticAnalyzer/semantic_analyzer.h"
#include "CodeGenerator/code_generator.h"
#include <cstdlib>
#include <iostream>

int main() {
    std::string input_file = "../Test/", output_file = "../Output/", file;

    std::cout << "Enter the file name: ";
    std::cin >> file;

    LexicalAnalyzer lexer(input_file + file, output_file + file + ".lex");
    lexer.run();

    SyntaxAnalyzer syn_analyzer(lexer.get_tokens(), output_file + file + ".syn");
    syn_analyzer.run();

    SemanticAnalyzer sem_analyzer(syn_analyzer.get_tree().get_root(), output_file + file + ".sem");
    sem_analyzer.analyze();

    CodeGenerator code_generator(syn_analyzer.get_tree().get_root(), sem_analyzer.get_symbol_table(),
                                 output_file + file + ".c");

    code_generator.run();

    std::string command = "gcc ../Output/" + file + ".c -o " + "../Output/" + file + ".out";
    int compileStatus = system(command.c_str());
    if (compileStatus != 0) {
        std::cerr << "Compilation failed.\n";
        return FAILURE;
    }

    command = "../Output/" + file + ".out";
    int runStatus = system(command.c_str());
    if (runStatus != 0) {
        std::cerr << "Execution failed.\n";
        return FAILURE;
    }

    return SUCCESS;
}