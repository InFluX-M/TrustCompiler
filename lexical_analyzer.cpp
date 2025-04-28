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
        int len = (int) line.size();
        int state = 0, perv_index = index;

        while (index < len) {
            if (state == 0) {
                if (is_spaces(line[index])) {
                    state = 1;
                } else {
                    state = 3;
                }
            } else if (state == 1) {
                if (line[index] == ENDL) {
                    state = 2;
                } else if (!is_spaces(line[index])) {
                    state = 2;
                    continue;
                }
            } else if (state == 2) {
                return {T_Whitespace, line_number};
            } else if (state == 3) {
                index = perv_index;
                return {Invalid, line_number};
            }
            index++;
        }

        return {T_Whitespace, line_number};
    }

    Token is_comment(int &index, const std::string &line, const int &line_number) {
        int len = (int) line.size();
        int state = 0, perv_index = index;

        std::string content;

        while (index < len) {
            if (state == 0) {
                if (line[index] == '/') {
                    state = 1;
                } else {
                    state = 4;
                }
            } else if (state == 1) {
                if (line[index] == '/') {
                    state = 2;
                } else {
                    state = 4;
                }
            } else if (state == 2) {
                if (line[index] == ENDL) {
                    state = 3;
                    continue;
                }
                content += line[index];
            } else if (state == 3) {
                return {T_Comment, line_number, content};
            } else if (state == 4) {
                index = perv_index;
                return {Invalid, line_number};
            }
            index++;
        }

        index = perv_index;
        return {Invalid, line_number};
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
        int len = (int) line.size();
        int state = 0, perv_index = index;

        while (index < len) {
            if (state == 0) {
                if (line[index] == '+') {
                    state = 1;
                } else if (line[index] == '-') {
                    state = 2;
                } else if (line[index] == '*') {
                    state = 3;
                } else if (line[index] == '/') {
                    state = 4;
                } else if (line[index] == '%') {
                    state = 5;
                } else if (line[index] == '=') {
                    state = 6;
                } else if (line[index] == '!') {
                    state = 9;
                } else if (line[index] == '<') {
                    state = 12;
                } else if (line[index] == '>') {
                    state = 15;
                } else if (line[index] == '&') {
                    state = 18;
                } else if (line[index] == '|') {
                    state = 20;
                } else if (line[index] == '(') {
                    state = 22;
                } else if (line[index] == ')') {
                    state = 23;
                } else if (line[index] == '{') {
                    state = 24;
                } else if (line[index] == '}') {
                    state = 25;
                } else if (line[index] == '[') {
                    state = 26;
                } else if (line[index] == ']') {
                    state = 27;
                } else if (line[index] == ';') {
                    state = 28;
                } else if (line[index] == ',') {
                    state = 29;
                } else if (line[index] == ':') {
                    state = 30;
                } else if (line[index] == '-') {
                    state = 31;
                } else {
                    state = 33;
                }
            } else if (state == 1) {
                return {T_AOp_AD, line_number, "+"};
            } else if (state == 2) {
                return {T_AOp_MN, line_number, "-"};
            } else if (state == 3) {
                return {T_AOp_ML, line_number, "*"};
            } else if (state == 4) {
                return {T_AOp_DV, line_number, "/"};
            } else if (state == 5) {
                return {T_AOp_RM, line_number, "%"};
            } else if (state == 6) {
                if (line[index] == '=') {
                    state = 8;
                } else {
                    state = 7;
                    continue;
                }
            } else if (state == 7) {
                return {T_Assign, line_number, "="};
            } else if (state == 8) {
                return {T_ROp_E, line_number, "=="};
            } else if (state == 9) {
                if (line[index] == '=') {
                    state = 11;
                } else {
                    state = 10;
                    continue;
                }
            } else if (state == 10) {
                return {T_LOp_NOT, line_number, "!"};
            } else if (state == 11) {
                return {T_ROp_NE, line_number, "!="};
            } else if (state == 12) {
                if (line[index] == '=') {
                    state = 14;
                } else {
                    state = 13;
                    continue;
                }
            } else if (state == 13) {
                return {T_ROp_L, line_number, "<"};
            } else if (state == 14) {
                return {T_ROp_LE, line_number, "<="};
            } else if (state == 15) {
                if (line[index] == '=') {
                    state = 17;
                } else {
                    state = 16;
                    continue;
                }
            } else if (state == 16) {
                return {T_ROp_G, line_number, ">"};
            } else if (state == 17) {
                return {T_ROp_GE, line_number, ">="};
            } else if (state == 18) {
                if (line[index] == '&') {
                    state = 19;
                } else {
                    state = 33;
                }
            } else if (state == 19) {
                return {T_LOp_AND, line_number, "&&"};
            } else if (state == 20) {
                if (line[index] == '|') {
                    state = 21;
                } else {
                    state = 33;
                }
            } else if (state == 21) {
                return {T_LOp_OR, line_number, "||"};
            } else if (state == 22) {
                return {T_LP, line_number, "("};
            } else if (state == 23) {
                return {T_RP, line_number, ")"};
            } else if (state == 24) {
                return {T_LC, line_number, "{"};
            } else if (state == 25) {
                return {T_RC, line_number, "}"};
            } else if (state == 26) {
                return {T_LB, line_number, "["};
            } else if (state == 27) {
                return {T_RB, line_number, "]"};
            } else if (state == 28) {
                return {T_Semicolon, line_number, ";"};
            } else if (state == 29) {
                return {T_Comma, line_number, ","};
            } else if (state == 30) {
                return {T_Colon, line_number, ":"};
            } else if (state == 31) {
                if (line[index] == '>') {
                    state = 32;
                } else {
                    state = 33;
                }
            } else if (state == 32) {
                return {T_Arrow, line_number, "->"};
            } else if (state == 33) {
                index = perv_index;
                return {Invalid, line_number};
            }
            index++;
        }

        index = perv_index;
        return {Invalid, line_number};
    }


    Token is_string(int &index, const std::string &line, const int &line_number) {
        int len = (int) line.size();
        int state = 0, perv_index = index;
        std::string content;

        while (index < len) {
            if (state == 0) {
                if (line[index] == '"') {
                    state = 1;
                    content += line[index];
                } else {
                    state = 4;
                }
            } else if (state == 1) {
                if (line[index] == '\\') {
                    state = 2;
                    content += line[index];
                } else if (line[index] == '"') {
                    state = 3;
                    content += line[index];
                } else {
                    state = 1;
                    content += line[index];
                }
            } else if (state == 2) {
                state = 1;
                content += line[index];
            } else if (state == 3) {
                return {T_String, line_number, content};
            } else if (state == 4) {
                index = perv_index;
                return {Invalid, line_number};
            }
            index++;
        }

        index = perv_index;
        return {Invalid, line_number};
    }

    Token is_keyword(int &index, const std::string &line, const int &line_number) {
        int len = (int) line.size();
        int state = 0, perv_index = index;

        while (index < len) {
            if (state == 0) {
                state = 13;
                for (int i = 0; i < NUM_KEYWORDS; i++) {
                    int sz = (int) key_words[i].size();
                    if (line.std::string::substr(index, sz) == key_words[i]) {
                        if ((index + sz) < (int) line.size() &&
                            (std::isalnum(line[index + sz]) || line[index + sz] == '_')) {
                            continue;
                        }
                        index += sz;
                        state = i + 1;
                        break;
                    }
                }
            } else if (state == 1) {
                return {T_Bool, line_number, "bool"};
            } else if (state == 2) {
                return {T_Break, line_number, "break"};
            } else if (state == 3) {
                return {T_Continue, line_number, "continue"};
            } else if (state == 4) {
                return {T_Else, line_number, "else"};
            } else if (state == 5) {
                return {T_False, line_number, "false"};
            } else if (state == 6) {
                return {T_Fn, line_number, "fn"};
            } else if (state == 7) {
                return {T_Int, line_number, "i32"};
            } else if (state == 8) {
                return {T_If, line_number, "if"};
            } else if (state == 9) {
                return {T_Let, line_number, "let"};
            } else if (state == 10) {
                return {T_Loop, line_number, "loop"};
            } else if (state == 11) {
                return {T_Mut, line_number, "mut"};
            } else if (state == 12) {
                return {T_Return, line_number, "return"};
            } else if (state == 13) {
                return {T_Print, line_number, "print"};
            } else if (state == 14) {
                return {T_True, line_number, "true"};
            } else if (state == 15) {
                index = perv_index;
                return {Invalid, line_number};
            }
        }

        index = perv_index;
        return {Invalid, line_number};
    }

    Token is_id(int &index, const std::string &line, const int &line_number) {
        int len = (int) line.size();
        int state = 0, perv_index = index;
        std::string content;

        while (index < len) {
            if (state == 0) {
                if (std::isalpha(line[index]) || line[index] == '_') {
                    content += line[index];
                    state = 1;
                } else {
                    state = 3;
                }
            } else if (state == 1) {
                if (std::isalnum(line[index]) || line[index] == '_') {
                    content += line[index];
                    state = 1;
                } else {
                    state = 2;
                    continue;
                }
            } else if (state == 2) {
                int temp_index = 0;
                Token keyword_token = is_keyword(temp_index, content, line_number);

                if (keyword_token.get_type() != Invalid)
                    return keyword_token;

                return {T_Id, line_number, content};

            } else if (state == 3) {
                index = perv_index;
                return {Invalid, line_number};
            }
            index++;
        }

        if (!content.empty()) {
            int temp_index = 0;
            Token keyword_token = is_keyword(temp_index, content, line_number);

            if (keyword_token.get_type() != Invalid)
                return keyword_token;

            return {T_Id, line_number, content};
        }

        index = perv_index;
        return {Invalid, line_number};
    }

    Token is_decimal(int &index, const std::string &line, const int &line_number) {
        int len = (int) line.size();
        int state = 0, perv_index = index;
        std::string content;

        while (index < len) {
            if (state == 0) {
                if (line[index] == '-') {
                    content += line[index];
                    state = 1;
                } else if (std::isdigit(line[index])) {
                    content += line[index];
                    state = 2;
                } else {
                    state = 4;
                }
            } else if (state == 1) {
                if (std::isdigit(line[index])) {
                    content += line[index];
                    state = 2;
                } else {
                    state = 4;
                }
            } else if (state == 2) {
                if (std::isdigit(line[index])) {
                    content += line[index];
                } else {
                    state = 3;
                    continue;
                }
            } else if (state == 3) {
                return {T_Decimal, line_number, content};
            } else if (state == 4) {
                index = perv_index;
                return {Invalid, line_number};
            }
            index++;
        }

        if (state == 2) {
            return {T_Decimal, line_number, content};
        }

        index = perv_index;
        return {Invalid, line_number};
    }

    Token is_hexadecimal(int &index, const std::string &line, const int &line_number) {
        int len = (int) line.size();
        int state = 0, perv_index = index;

        std::string content;

        while (index < len) {
            if (state == 0) {
                if (line[index] == '0') {
                    state = 1;
                    content += line[index];
                } else {
                    state = 5;
                }
            } else if (state == 1) {
                if (std::tolower(line[index]) == 'x') {
                    state = 2;
                    content += line[index];
                } else {
                    state = 5;
                }
            } else if (state == 2) {
                if (std::isxdigit(line[index])) {
                    state = 3;
                    content += line[index];
                } else {
                    state = 5;
                }
            } else if (state == 3) {
                if (std::isxdigit(line[index])) {
                    state = 3;
                    content += line[index];
                } else {
                    state = 4;
                    continue;
                }
            } else if (state == 4) {
                return {T_Hexadecimal, line_number, content};
            } else if (state == 5) {
                index = perv_index;
                return {Invalid, line_number};
            }
            index++;
        }

        index = perv_index;
        return {Invalid, line_number};
    }

};