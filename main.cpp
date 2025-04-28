#include "lexical_analyzer.cpp"
#include <iostream>

int main() {
    std::string input_file, output_file = "tokens.txt";

    std::cout << "Enter the input file name: ";
    std::cin >> input_file;

    LexicalAnalyzer lexer(input_file, output_file);

    lexer.tokenize();
    lexer.write();

    std::cout << "Lexical analysis completed successfully!" << std::endl;

    return SUCCESS;
}
