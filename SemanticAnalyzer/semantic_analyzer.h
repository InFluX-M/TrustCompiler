#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "../utils.h"

enum id_type {
    VAR,
    FUNC,
    NONE
};

class SymbolTableEntry {
private:
    std::string name;
    id_type type;
    semantic_type stype;
    std::vector<std::pair<std::string, semantic_type>> parameters;
    std::vector<semantic_type> tuple_types;
    int def_area;
    bool mut;

    int arr_len;
    semantic_type arr_type;

public:
    SymbolTableEntry() {
        type = NONE;
    }

    SymbolTableEntry(id_type _type) {
        type = _type;
        stype = UNK;
        def_area = 0;
        mut = false;
        arr_len = 0;
        arr_type = UNK;
    }

    SymbolTableEntry(id_type _type, semantic_type _stype, int _def_area, bool _mut = false,
                     int _arr_len = 0, semantic_type _arr_type = UNK) {
        type = _type;
        stype = _stype;
        def_area = _def_area;
        mut = _mut;
        arr_len = _arr_len;
        arr_type = _arr_type;
    }


    void set_name(std::string _name) {
        name = std::move(_name);
    }

    std::string get_name() const {
        return name;
    }

    void set_type(id_type _type) {
        type = _type;
    }

    id_type get_type() const {
        return type;
    }

    void set_stype(semantic_type _stype) {
        stype = _stype;
    }

    semantic_type get_stype() const {
        return stype;
    }

    void add_to_parameters(const std::pair<std::string, semantic_type> &parameter) {
        parameters.push_back(parameter);
    }

    std::vector<std::pair<std::string, semantic_type>> &get_parameters() {
        return parameters;
    }

    void add_to_tuple_types(semantic_type _stype) {
        tuple_types.push_back(_stype);
    }

    void add_to_tuple_types(std::vector<semantic_type> &_tuple_types) {
        for (auto _stype: _tuple_types) {
            tuple_types.push_back(_stype);
        }
    }

    std::vector<semantic_type> &get_tuple_types() {
        return tuple_types;
    }

    void set_def_area(int _def_area) {
        def_area = _def_area;
    }

    int get_def_area() const {
        return def_area;
    }

    void set_mut(bool _mut) {
        mut = _mut;
    }

    bool get_mut() const {
        return mut;
    }

    void set_arr_len(int _arr_len) {
        arr_len = _arr_len;
    }

    int get_arr_len() const {
        return arr_len;
    }

    void set_arr_type(semantic_type _arr_type) {
        arr_type = _arr_type;
    }

    semantic_type get_arr_type() const {
        return arr_type;
    }

    void clear_parameters() {
        parameters.clear();
    }
};

class SemanticAnalyzer {
private:
    std::string out_address;
    Tree<Symbol> parse_tree;

    std::map<std::string, std::map<std::string, SymbolTableEntry>> symbol_table;

    int def_area;
    std::string current_func;
    std::string code;
    int num_errors;

    void make_syntax_tree(Node<Symbol> *node);

public:
    void dfs(Node<Symbol> *node);

    void analyse();

    void run_code();

    SemanticAnalyzer(Tree<Symbol> _parse_tree, std::string output_file_name);
};

#endif // SEMANTIC_ANALYZER_H