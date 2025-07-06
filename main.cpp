#include "LexicalAnalyzer/lexical_analyzer.h"
#include "SyntaxAnalyzer/syntax_analyzer.h"
#include "SemanticAnalyzer/semantic_analyzer.h"

#include <iostream>

int main() {
    std::string input_file = "Test/", output_file = "Output/", file;

    std::cout << "Enter the file name: ";
    std::cin >> file;

    LexicalAnalyzer lexer(input_file + file, output_file + file + ".lex");

    lexer.tokenize();
    lexer.write();

    std::cout << "Lexical analysis completed successfully!" << std::endl;

    SyntaxAnalyzer syn_analyzer(lexer.get_tokens(), output_file + file + ".syn");

    syn_analyzer.make_tree(true);
    syn_analyzer.write();

    std::cout << "Syntax analysis completed successfully!" << std::endl;

    SemanticAnalyzer sem_analyzer(syn_analyzer.get_tree(), output_file + file + ".sem");
     sem_analyzer.dfs(syn_analyzer.get_tree().get_root());

    std::cout << "Semantic analysis completed successfully!" << std::endl;

    return SUCCESS;
}