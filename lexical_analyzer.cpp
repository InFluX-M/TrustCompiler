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
        int num_errors = 0;

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

        Token is_comment(int &index, const std::string &line, const int &line_number) {
            int len = (int)line.size();
            int state = 0, perv_index = index;

            std::string content = "";

            while (index < len) {
                if (state == 0) {
                    if (line[index] == '/') {
                        state = 1;
                    }
                    else {
                        state = 4;
                    }
                }
                else if (state == 1) {
                    if (line[index] == '/') {
                        state = 2;
                    }
                    else {
                        state = 4;
                    }
                }
                else if (state == 2) {
                    if (line[index] == ENDL) {
                        state = 3;
                        continue;
                    }
                    content += line[index];
                }
                else if (state == 3) {
                    return Token(T_Comment, line_number, content);
                }
                else if (state == 4) {
                    index = perv_index;
                    return Token(Invalid, line_number);
                }
                index++;
            }
            
            index = perv_index;
            return Token(Invalid, line_number);
        }

        /*
            T_AOp_AD +    1
            T_AOp_MN -    2
            T_AOp_ML *    3
            T_AOp_DV /    4
            T_AOp_RM %    5

            =             6
            T_Assign =    7
            T_ROp_E ==    8
            !             9
            T_LOp_NOT !   10
            T_ROp_NE !=   11
            <             12
            T_ROp_L <     13
            T_ROp_LE <=   14
            >             15
            T_ROp_G >     16
            T_ROp_GE >=   17

            &             18
            T_LOp_AND &&  19

            |             20
            T_LOp_OR ||   21

            T_LP (        22
            T_RP )        23
            T_LC {        24
            T_RC }        25
            T_LB [        26
            T_RB ]        27

            T_Semicolon ; 28
            T_Comma ,     29
            T_Colon :     30
            -             31
            T_Arrow ->    32

            Invalid       33
        */

        Token is_operator(int &index, const std::string &line, const int &line_number) {
            int len = (int)line.size();
            int state = 0, perv_index = index;

            while (index < len) {
                if (state == 0) {
                    if (line[index] == '+') {
                        state = 1;
                    }
                    else if (line[index] == '-') {
                        state = 2;
                    }
                    else if (line[index] == '*') {
                        state = 3;
                    }
                    else if (line[index] == '/') {
                        state = 4;
                    }
                    else if (line[index] == '%') {
                        state = 5;
                    }
                    else if (line[index] == '=') {
                        state = 6;
                    }
                    else if (line[index] == '!') {
                        state = 9;
                    }
                    else if (line[index] == '<') {
                        state = 12;
                    }
                    else if (line[index] == '>') {
                        state = 15;
                    }
                    else if (line[index] == '&') {
                        state = 18;
                    }
                    else if (line[index] == '|') {
                        state = 20;
                    }
                    else if (line[index] == '(') {
                        state = 22;
                    }
                    else if (line[index] == ')') {
                        state = 23;
                    }
                    else if (line[index] == '{') {
                        state = 24;
                    }
                    else if (line[index] == '}') {
                        state = 25;
                    }
                    else if (line[index] == '[') {
                        state = 26;
                    }
                    else if (line[index] == ']') {
                        state = 27;
                    }
                    else if (line[index] == ';') {
                        state = 28;
                    }
                    else if (line[index] == ',') {
                        state = 29;
                    }
                    else if (line[index] == ':') {
                        state = 30;
                    }
                    else if (line[index] == '-') {
                        state = 31;
                    }
                    else {
                        state = 33;
                    }
                }
                else if (state == 1) {
                    return Token(T_AOp_AD, line_number, "+");
                }
                else if (state == 2) {
                    return Token(T_AOp_MN, line_number, "-");
                }
                else if (state == 3) {
                    return Token(T_AOp_ML, line_number, "*");
                }
                else if (state == 4) {
                    return Token(T_AOp_DV, line_number, "/");
                }
                else if (state == 5) {
                    return Token(T_AOp_RM, line_number, "%");
                }
                else if (state == 6) {
                    if (line[index] == '=') {
                        state = 8;
                    }
                    else {
                        state = 7;
                        continue;
                    }
                }
                else if (state == 7) {
                    return Token(T_Assign, line_number, "=");
                }
                else if (state == 8) {
                    return Token(T_ROp_E, line_number, "==");
                }
                else if (state == 9) {
                    if (line[index] == '=') {
                        state = 11;
                    }
                    else {
                        state = 10;
                        continue;
                    }
                }
                else if (state == 10) {
                    return Token(T_LOp_NOT, line_number, "!");
                }
                else if (state == 11) {
                    return Token(T_ROp_NE, line_number, "!=");
                }
                else if (state == 12) {
                    if (line[index] == '=') {
                        state = 14;
                    }
                    else {
                        state = 13;
                        continue;
                    }
                }
                else if (state == 13) {
                    return Token(T_ROp_L, line_number, "<");
                }
                else if (state == 14) {
                    return Token(T_ROp_LE, line_number, "<=");
                }
                else if (state == 15) {
                    if (line[index] == '=') {
                        state = 17;
                    }
                    else {
                        state = 16;
                        continue;
                    }
                }
                else if (state == 16) {
                    return Token(T_ROp_G, line_number, ">");
                }
                else if (state == 17) {
                    return Token(T_ROp_GE, line_number, ">=");
                }
                else if (state == 18) {
                    if (line[index] == '&') {
                        state = 19;
                    }
                    else {
                        state = 33;
                    }
                }
                else if (state == 19) {
                    return Token(T_LOp_AND, line_number, "&&");
                }
                else if (state == 20) {
                    if (line[index] == '|') {
                        state = 21;
                    }
                    else {
                        state = 33;
                    }
                }
                else if (state == 21) {
                    return Token(T_LOp_OR, line_number, "||");
                }
                else if (state == 22) {
                    return Token(T_LP, line_number, "(");
                }
                else if (state == 23) {
                    return Token(T_RP, line_number, ")");
                }
                else if (state == 24) {
                    return Token(T_LC, line_number, "{");
                }
                else if (state == 25) {
                    return Token(T_RC, line_number, "}");
                }
                else if (state == 26) {
                    return Token(T_LB, line_number, "[");
                }
                else if (state == 27) {
                    return Token(T_RB, line_number, "]");
                }
                else if (state == 28) {
                    return Token(T_Semicolon, line_number, ";");
                }
                else if (state == 29) {
                    return Token(T_Comma, line_number, ",");
                }
                else if (state == 30) {
                    return Token(T_Colon, line_number, ":");
                }
                else if (state == 31) {
                    if (line[index] == '>') {
                        state = 32;
                    }
                    else {
                        state = 33;
                    }
                }
                else if (state == 32) {
                    return Token(T_Arrow, line_number, "!=");
                }
                else if (state == 33) {
                    index = perv_index;
                    return Token(Invalid, line_number);
                }
                index++;
            }
            
            index = perv_index;
            return Token(Invalid, line_number);
        }


        Token is_string(int &index, const std::string &line, const int &line_number) {
            int len = (int)line.size();
            int state = 0, perv_index = index;
            std::string content = "";

            while (index < len) {
                if (state == 0) {
                    if (line[index] == '"') {
                        state = 1;
                        content += line[index];
                    }
                    else {
                        state = 4;
                    }
                }
                else if (state == 1) {
                    if (line[index] == '\\') {
                        state = 2;
                        content += line[index];
                    }
                    else if (line[index] == '"') {
                        state = 3;
                        content += line[index];
                    } 
                    else {
                        state = 1;
                        content += line[index];
                    }
                }
                else if (state == 2) {
                    state = 1;
                    content += line[index];
                }
                else if (state == 3) {
                    return Token(T_String, line_number, content);
                }
                else if (state == 4) {
                    index = perv_index;
                    return Token(Invalid, line_number);
                }
                index++;
            }

            index = perv_index;
            return Token(Invalid, line_number);
        }

};