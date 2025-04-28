#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <stack>
#include <deque>
#include <set>
#include <map>
#include <algorithm>

#define SUCCESS 0
#define FAILURE 1
#define FILE_ERROR 2
#define GRAMMAR_ERROR 3

#define ENDL '\n'
#define SPACE ' '
#define TAB '\t'

#define COLORED_ERRORS true

const std::string WHITE = COLORED_ERRORS ? "\033[0;m" : "";
const std::string RED = COLORED_ERRORS ? "\033[0;31m" : "";
const std::string GREEN = COLORED_ERRORS ? "\033[0;32m" : "";
const std::string YELLOW = COLORED_ERRORS ? "\033[0;33m" : "";

enum token_type {
    T_Bool,
    T_Break,
    T_Continue,
    T_Else,
    T_False,
    T_Fn,
    T_Int,
    T_If,
    T_Let,
    T_Loop,
    T_Mut,
    T_Return,
    T_Print,
    T_True,

    T_AOp_AD,
    T_AOp_MN,
    T_AOp_ML,
    T_AOp_DV,
    T_AOp_RM,

    T_ROp_L,
    T_ROp_G,
    T_ROp_LE,
    T_ROp_GE,
    T_ROp_NE,
    T_ROp_E,

    T_LOp_AND,
    T_LOp_OR,
    T_LOp_NOT,

    T_Assign,
    T_LP,
    T_RP,
    T_LC,
    T_RC,
    T_LB,
    T_RB,
    
    T_Semicolon,
    T_Comma,
    T_Colon,
    T_Arrow,
    T_Id,
    T_Decimal,
    T_Hexadecimal,
    T_String,
    T_Comment,
    T_Whitespace,

    Invalid,
    Eof
};

const std::string type_to_string[] = {
    "T_Bool",
    "T_Break",
    "T_Continue",
    "T_Else",
    "T_False",
    "T_Fn",
    "T_Int",
    "T_If",
    "T_Let",
    "T_Loop",
    "T_Mut",
    "T_Return",
    "T_Print",
    "T_True",

    "T_AOp_AD",
    "T_AOp_MN",
    "T_AOp_ML",
    "T_AOp_DV",
    "T_AOp_RM",

    "T_ROp_L",
    "T_ROp_G",
    "T_ROp_LE",
    "T_ROp_GE",
    "T_ROp_NE",
    "T_ROp_E",

    "T_LOp_AND",
    "T_LOp_OR",
    "T_LOp_NOT",

    "T_Assign",
    "T_LP",
    "T_RP",
    "T_LC",
    "T_RC",
    "T_LB",
    "T_RB",
    
    "T_Semicolon",
    "T_Comma",
    "T_Colon",
    "T_Arrow",
    "T_Id",
    "T_Decimal",
    "T_Hexadecimal",
    "T_String",
    "T_Comment",
    "T_Whitespace",

    "Invalid",
    "Eof"
};

class Token {
    private:
        token_type type;
        int line_number;
        std::string content;

    public:
        Token(token_type _type, int _line_number = -1, std::string _content = "") {
            type = _type;
            line_number = _line_number;
            content = _content;
        }

        token_type get_type() {
            return type;
        }

        int get_line_number() {
            return line_number;
        }

        std::string get_content() {
            return content;
        }


        void set_type(token_type _type) {
            type = _type;
        }

        void set_line_number(int _line_number) {
            line_number = _line_number;
        }

        void set_content(std::string _content) {
            content = _content;
        }


        std::string toString() const {
            if (content == "") {
                return "< type: " + type_to_string[type] + ", line: " + std::to_string(line_number) + " >";
            }
            return "< type: " + type_to_string[type] + ", line: " + std::to_string(line_number) + ", content: " + content + " >";
        }

        friend std::ostream& operator << (std::ostream &out, const Token &token) {
            return out << token.toString();
        }
};
