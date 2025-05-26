#include "LexicalAnalyzer/lexical_analyzer.h"
#include "SyntaxAnalyzer/syntax_analyzer.h"
#include <iostream>

int main() {
    std::string input_file = "Test/", output_file = "Output/", file;

    std::cout << "Enter the file name: ";
    std::cin >> file;

    LexicalAnalyzer lexer(input_file + file, output_file + file + ".out");

    lexer.tokenize();
    lexer.write();

    std::cout << "Lexical analysis completed successfully!" << std::endl;

    SyntaxAnalyzer syn_analyzer(lexer.get_tokens(), output_file + file + ".syntax");

    syn_analyzer.make_tree(false);
    syn_analyzer.write();

    std::cout << "Syntax analysis completed successfully!" << std::endl;


    return SUCCESS;
}