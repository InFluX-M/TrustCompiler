#include "lexical_analyzer.h"

char spaces[] = {ENDL, SPACE, TAB};

std::string key_words[NUM_KEYWORDS] = {
    "bool",
    "break",
    "continue",
    "else",
    "false",
    "fn",
    "i32",
    "if",
    "let",
    "loop",
    "mut",
    "println!",
    "return",
    "true"
};

bool is_spaces(const char &ch) {
    return std::find(std::begin(spaces), std::end(spaces), ch) != std::end(spaces);
}

class LexicalAnalyzer {
    private:
        std::string in_path, out_path;
        std::ifstream in;
        std::ofstream out;
        std::vector<Token> tokens;
        int num_erros = 0;

        Token is_space(int &index, const std::string &line, const int &line_number) {
            int len = (int)line.size();
            int state = 0, perv_index = index;

            while (index < len) {
                if (state == 0) {
                    if (is_spaces(line[index])) {
                        state = 1;
                    }
                    else {
                        state = 3;
                    }
                }
                else if (state == 1) {
                    if (line[index] == ENDL) {
                        state = 2;
                    }
                    else if (!is_spaces(line[index])) {
                        state = 2;
                        continue;
                    }
                }
                else if (state == 2) {
                    return Token(T_Whitespace, line_number);
                }
                else if (state == 3) {
                    index = perv_index;
                    return Token(Invalid, line_number);
                }
                index++;
            }
            
            return Token(T_Whitespace, line_number);
        }
};