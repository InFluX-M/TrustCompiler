#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include <utility>

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
    std::string val;

    int arr_len;
    semantic_type arr_type;

public:
    SymbolTableEntry() {
        type = NONE;
        val = "";
    }

    SymbolTableEntry(id_type _type) {
        type = _type;
        stype = UNK;
        def_area = 0;
        mut = false;
        arr_len = 0;
        arr_type = UNK;
        val = "";
    }

    SymbolTableEntry(id_type _type, semantic_type _stype, int _def_area, bool _mut = false,
                     int _arr_len = 0, semantic_type _arr_type = UNK) {
        type = _type;
        stype = _stype;
        def_area = _def_area;
        mut = _mut;
        arr_len = _arr_len;
        arr_type = _arr_type;
        val = "";
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

    id_type get_type() {
        return type;
    }

    void set_stype(semantic_type _stype) {
        stype = _stype;
    }

    semantic_type get_stype() {
        return stype;
    }

    void add_to_parameters(std::pair<std::string, semantic_type> parameter) {
        parameters.push_back(parameter);
    }

    std::vector<std::pair<std::string, semantic_type>> &get_parameters() {
        return parameters;
    }

    void set_val(std::string _val) {
        val = std::move(_val);
    }

    std::string get_val() {
        return val;
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

    bool get_mut() {
        return mut;
    }

    void set_arr_len(int _arr_len) {
        arr_len = _arr_len;
    }

    int get_arr_len() {
        return arr_len;
    }

    void set_arr_type(semantic_type _arr_type) {
        arr_type = _arr_type;
    }

    semantic_type get_arr_type() {
        return arr_type;
    }

    void clear_parameters() {
        parameters.clear();
    }

    semantic_type get_stype() const {
        return stype;
    }

    const std::vector<std::pair<std::string, semantic_type>>& get_parameters() const {
        return parameters;
    }

    const std::vector<semantic_type>& get_tuple_types() const {
        return tuple_types;
    }
};

class SemanticAnalyzer {
private:
    std::string out_address;
    Tree<Symbol> parse_tree;
    std::ofstream out;
    bool has_par[200]{};

    std::map<std::string, std::map<std::string, SymbolTableEntry>> symbol_table;

    int def_area;
    std::string current_func;
    std::string code;
    int num_errors;

    void write_annotated_tree(Node<Symbol> *node, int num = 0, bool last = false);

public:
    void dfs(Node<Symbol> *node);

    void check_for_main_function();

    void analyze();

    SemanticAnalyzer(Tree<Symbol> _parse_tree, std::string output_file_name);

    std::map<std::string, std::map<std::string, SymbolTableEntry>> get_symbol_table() {
        return symbol_table;
    }

};

#endif // SEMANTIC_ANALYZER_H