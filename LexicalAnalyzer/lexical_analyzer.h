#ifndef LEXICAL_ANALYZER_H
#define LEXICAL_ANALYZER_H

#include "../utils.h"
#include <vector>
#include <string>
#include <fstream>

#define NUM_KEYWORDS 14
#define TOKENIZE_WHITESPACE false
#define TOKENIZE_COMMENT false

extern char spaces[];
extern std::string key_words[NUM_KEYWORDS];

bool is_spaces(const char &ch);

class LexicalAnalyzer {
public:
    LexicalAnalyzer(const std::string &input_file, const std::string &output_file);

private:
    std::string in_path, out_path;
    std::ifstream in;
    std::ofstream out;
    std::vector<Token> tokens;
    int num_errors = 0;

    Token is_space(int &index, const std::string &line, const int &line_number);

    Token is_comment(int &index, const std::string &line, const int &line_number);

    Token is_operator(int &index, const std::string &line, const int &line_number);

    Token is_string(int &index, const std::string &line, const int &line_number);

    Token is_keyword(int &index, const std::string &line, const int &line_number);

    Token is_id(int &index, const std::string &line, const int &line_number);

    Token is_decimal(int &index, const std::string &line, const int &line_number);

    Token is_hexadecimal(int &index, const std::string &line, const int &line_number);

    void add_token_if_needed(Token token);

    void extract(std::string &line);

public:
    void tokenize();

    void read_tokens();

    void write_tokens();

    void write();

    void run();

    std::vector<Token> get_tokens();
};

#endif // LEXICAL_ANALYZER_H