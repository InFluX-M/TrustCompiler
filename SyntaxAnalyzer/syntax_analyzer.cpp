#include "syntax_analyzer.h"


Rule::Rule() {
    type = EMPTY;
}

Rule::Rule(rule_type _type) {
    type = _type;
}

void Rule::set_head(Symbol _head) {
    head = std::move(_head);
}

Symbol &Rule::get_head() {
    return head;
}

void Rule::set_type(rule_type _type) {
    type = _type;
}

rule_type Rule::get_type() {
    return type;
}

void Rule::add_to_body(const Symbol &var) {
    body.push_back(var);
}

std::vector<Symbol> &Rule::get_body() {
    return body;
}

std::string Rule::toString() {
    std::string res;
    res += head.toString() + " -> ";
    for (auto &part: body) {
        res += part.toString() + " ";
    }
    if (type == SYNCH) {
        res = "SYNCH";
    }
    if (type == EMPTY) {
        res = "EMPTY";
    }
    return res;
}

std::ostream &operator<<(std::ostream &out, Rule &rule) {
    return out << rule.toString();
}

SyntaxAnalyzer::SyntaxAnalyzer(std::vector<Token> _tokens, std::string output_file) {
    tokens = std::move(_tokens);
    out_address = std::move(output_file);
    update_grammar();
    num_errors = 0;
}

void SyntaxAnalyzer::extract(std::string line) {
    line = strip(line);
    std::string head_str, body_str;
    int len = (int) line.size();
    for (int i = 0; i < len; i++) {
        if (line[i] == '<' or line[i] == '>') {
            continue;
        }
        if (line[i] != ' ') {
            head_str += line[i];
        } else {
            while (line[i] == ' ' || line[i] == '-' || line[i] == '>') {
                i++;
            }
            while (i < len) {
                body_str += line[i];
                i++;
            }
        }
    }

    Symbol head = Symbol(head_str, VARIABLE);
    variables.insert(head);

    std::vector<std::string> rules_str = split(body_str, '@');
    for (auto &rule: rules_str) {
        rule = strip(rule);
    }
    int number_rules = (int) rules_str.size();
    if (number_rules < 1) {
        std::cerr << RED << "Grammar Error: Invalid grammar in file" << WHITE << std::endl;
        exit(GRAMMAR_ERROR);
    }

    for (int i = 0; i < number_rules; i++) {
        Rule rule;
        rule.set_head(head);
        rule.set_type(VALID);
        std::vector<std::string> rule_parts = split(rules_str[i]);
        for (auto & rule_part : rule_parts) {
            Symbol tmp;
            if (rule_part == "ε") {
                tmp.set_name("eps");
                tmp.set_type(TERMINAL);
                terminals.insert(tmp);
            } else if (rule_part[0] == '<') {
                tmp.set_name(rule_part.substr(1, rule_part.size() - 2));
                tmp.set_type(VARIABLE);
                variables.insert(tmp);
            } else {
                tmp.set_name(rule_part);
                tmp.set_type(TERMINAL);
                terminals.insert(tmp);
            }
            rule.add_to_body(tmp);
        }
        rules.push_back(rule);
        self_rules[rule.get_head()].push_back(rule);
    }
}

void SyntaxAnalyzer::calc_firsts() {
    for (const auto &term: terminals) {
        calc_first(term);
    }
    for (const auto &var: variables) {
        calc_first(var);
    }
}

void SyntaxAnalyzer::calc_first(const Symbol &var) {
    if (var.get_type() == TERMINAL) {
        firsts[var].insert(var);
        first_done[var] = true;
        return;
    }

    for (auto &rule: self_rules[var]) {
        bool exist_eps = false;
        for (auto &part_body: rule.get_body()) {
            if (!first_done[part_body]) {
                calc_first(part_body);
            }
            exist_eps = false;
            for (const auto& first: firsts[part_body]) {
                if (first == eps) {
                    exist_eps = true;
                } else {
                    firsts[var].insert(first);
                }
            }
            if (!exist_eps) {
                break;
            }
        }
        if (exist_eps) {
            firsts[var].insert(eps);
        }
    }
    first_done[var] = true;
}

void SyntaxAnalyzer::print_firsts() {
    for (const auto &term: terminals) {
        print_first(term);
    }
    for (const auto &var: variables) {
        print_first(var);
    }
}

void SyntaxAnalyzer::print_first(const Symbol &var) {
    std::cout << "First[" + var.get_name() + "]:";
    for (auto first: firsts[var]) {
        std::cout << " " << first;
    }
    std::cout << std::endl;
}

void SyntaxAnalyzer::calc_follows() {
    for (const auto &var: variables) {
        if (var.get_name() == START_VAR) {
            follows[var].insert(Symbol("$", TERMINAL));
        }
        calc_follow(var);
    }
    relaxation();
}

void SyntaxAnalyzer::calc_follow(const Symbol &var) {
    for (auto &rule: rules) {
        std::vector<Symbol> &body = rule.get_body();
        int body_len = (int) body.size();

        for (int i = 0; i < body_len; i++) {
            if (body[i] == var) {
                bool all_eps = true;
                for (int j = i + 1; j < body_len; j++) {
                    bool has_eps = false;
                    for (const auto &first: firsts[body[j]]) {
                        if (first == eps) {
                            has_eps = true;
                        } else {
                            follows[var].insert(first);
                        }
                    }
                    if (!has_eps) {
                        all_eps = false;
                        break;
                    }
                }
                if (all_eps && rule.get_head() != var) {
                    for (const auto &follow: follows[rule.get_head()]) {
                        follows[var].insert(follow);
                    }
                    graph[rule.get_head()].insert(var);
                }
            }
        }
    }
}

void SyntaxAnalyzer::relaxation() {
    std::set<Symbol> set;
    for (const auto &var: variables) {
        set.insert(var);
    }

    while (!set.empty()) {
        Symbol var = *set.begin();
        set.erase(set.begin());

        for (const auto &v: graph[var]) {
            int old_size = (int) follows[v].size();
            for (const auto &follow: follows[var]) {
                follows[v].insert(follow);
            }
            int new_size = (int) follows[v].size();
            if (new_size > old_size) {
                set.insert(v);
            }
        }
    }
}

void SyntaxAnalyzer::print_follows() {
    for (const auto &var: variables) {
        print_follow(var);
    }
}

void SyntaxAnalyzer::print_follow(const Symbol &var) {
    std::cout << "Follow[" + var.get_name() + "]:";
    for (auto follow: follows[var]) {
        std::cout << " " << follow;
    }
    std::cout << std::endl;
}

void SyntaxAnalyzer::set_matches() {
    match[T_Bool] = "T_Bool";
    match[T_Break] = "T_Break";
    match[T_Continue] = "T_Continue";
    match[T_Else] = "T_Else";
    match[T_False] = "T_False";
    match[T_Fn] = "T_Fn";
    match[T_Int] = "T_Int";
    match[T_If] = "T_If";
    match[T_Let] = "T_Let";
    match[T_Loop] = "T_Loop";
    match[T_Mut] = "T_Mut";
    match[T_Print] = "T_Print!";
    match[T_Return] = "T_Return";
    match[T_True] = "T_True";

    match[T_AOp_AD] = "T_AOp_AD";
    match[T_AOp_MN] = "T_AOp_MN";
    match[T_AOp_ML] = "T_AOp_ML";
    match[T_AOp_DV] = "T_AOp_DV";
    match[T_AOp_RM] = "T_AOp_RM";

    match[T_ROp_L] = "T_ROp_L";
    match[T_ROp_G] = "T_ROp_G";
    match[T_ROp_LE] = "T_ROp_LE";
    match[T_ROp_GE] = "T_ROp_GE";
    match[T_ROp_NE] = "T_ROp_NE";
    match[T_ROp_E] = "T_ROp_E";

    match[T_LOp_AND] = "T_LOp_AND";
    match[T_LOp_OR] = "T_LOp_OR";
    match[T_LOp_NOT] = "T_LOp_NOT";

    match[T_Assign] = "T_Assign";
    match[T_LP] = "T_LP";
    match[T_RP] = "T_RP";
    match[T_LC] = "T_LC";
    match[T_RC] = "T_RC";
    match[T_LB] = "T_LB";
    match[T_RB] = "T_RB";
    match[T_Semicolon] = "T_Semicolon";
    match[T_Comma] = "T_Comma";
    match[T_Colon] = "T_Colon";
    match[T_Arrow] = "T_Arrow";

    match[T_Id] = "T_Id";
    match[T_Decimal] = "T_Decimal";
    match[T_Hexadecimal] = "T_Hexadecimal";
    match[T_String] = "T_String";

    match[Invalid] = "invalid";
    match[Eof] = "$";
}

void SyntaxAnalyzer::make_table() {
    for (auto &rule: rules) {
        std::vector<Symbol> &body = rule.get_body();
        Symbol head = rule.get_head();
        bool all_eps = true;
        for (const auto &part_body: body) {
            bool has_eps = false;
            for (const auto &first: firsts[part_body]) {
                if (first == eps) {
                    has_eps = true;
                } else {
                    table[{head, first}] = rule;
                }
            }
            if (!has_eps) {
                all_eps = false;
                break;
            }
        }

        if (all_eps) {
            for (const Symbol &var: follows[rule.get_head()]) {
                table[{rule.get_head(), var}] = rule;
            }
        } else {
            for (const Symbol &var: follows[rule.get_head()]) {
                if (table[{rule.get_head(), var}].get_type() != VALID) {
                    table[{rule.get_head(), var}] = Rule(SYNCH);
                }
            }
        }
    }

    Symbol semicolon = Symbol(";", TERMINAL);
    Symbol closed_curly = Symbol("}", TERMINAL);
    for (const auto &var: variables) {
        if (table[{var, semicolon}].get_type() != VALID) {
            table[{var, semicolon}] = Rule(SYNCH);
        }
        if (table[{var, closed_curly}].get_type() != VALID) {
            table[{var, closed_curly}] = Rule(SYNCH);
        }
    }
}

void SyntaxAnalyzer::write_table() {
    std::ofstream table_file;
    table_file.open(TABLE_PATH);
    if (!table_file.is_open()) {
        std::cerr << RED << "File Error: Couldn't open table file for write" << WHITE << std::endl;
        exit(FILE_ERROR);
    }
    for (auto &col: table) {
        Symbol head1 = col.first.first;
        Symbol head2 = col.first.second;
        Rule rule = col.second;

        table_file << "# " << head1 << ' ' << head2 << '\n';
        table_file << rule << '\n';
    }

    table_file.close();
}

void SyntaxAnalyzer::read_table() {
    std::ifstream table_file;
    table_file.open(TABLE_PATH);
    if (!table_file.is_open()) {
        std::cerr << RED << "File Error: Couldn't open table file for read" << WHITE << std::endl;
        exit(FILE_ERROR);
    }

    std::string line;
    Symbol head1("", VARIABLE), head2("", TERMINAL);
    while (getline(table_file, line)) {
        line = strip(line);
        std::vector<std::string> line_parts = split(line);
        int part_rules = (int) line_parts.size();
        if (line_parts[0] != "SYNCH" && line_parts[0] != "EMPTY" && part_rules < 3) {
            std::cerr << RED << "Table Error: Invalid table in file" << WHITE << std::endl;
            exit(GRAMMAR_ERROR);
        }

        if (line_parts[0] == "#") {
            head1.set_name(line_parts[1]);
            head2.set_name(line_parts[2].substr(1, (int) line_parts[2].size() - 2));
        } else if (line_parts[0] == "SYNCH") {
            Rule rule(SYNCH);
            table[{head1, head2}] = rule;
        } else if (line_parts[0] == "EMPTY") {
            Rule rule(EMPTY);
            table[{head1, head2}] = rule;
        } else {
            Symbol head(line_parts[0], VARIABLE), tmp;
            Rule rule(VALID);
            rule.set_head(head);

            for (int i = 2; i < part_rules; i++) {
                if (line_parts[i][0] == '<') {
                    tmp.set_name(line_parts[i].substr(1, line_parts[i].size() - 2));
                    tmp.set_type(VARIABLE);
                } else {
                    tmp.set_name(line_parts[i]);
                    tmp.set_type(TERMINAL);
                }
                rule.add_to_body(tmp);
            }
            table[{head1, head2}] = rule;
        }
    }
    table_file.close();
}

void SyntaxAnalyzer::write_tree(Node<Symbol> *node, int num, bool last) {
    Symbol var = node->get_data();

    for (int i = 0; i < num * TAB - TAB; i++) {
        if (has_par[i]) {
            out << "│";
        } else {
            out << " ";
        }
    }
    if (num) {
        if (last) {
            out << "└── ";
        } else {
            out << "├── ";
        }
    }
    out << var << "\n";
    if (!node->get_data().get_content().empty()) {
        for (int i = 0; i < num * TAB; i++) {
            if (has_par[i]) {
                out << "│";
            } else {
                out << " ";
            }
        }
        out << "└── '" << node->get_data().get_content() << "'" << "\n";
    }

    has_par[num * TAB] = true;
    std::deque<Node<Symbol> *> children = node->get_children();
    for (auto child: children) {
        bool end = false;
        if (child == children.back()) {
            has_par[num * TAB] = false;
            end = true;
        }
        write_tree(child, num + 1, end);
    }
}

void SyntaxAnalyzer::update_grammar() {
    std::ifstream in;
    in.open(GRAMMAR_PATH);
    if (!in.is_open()) {
        std::cerr << RED << "File Error: Couldn't open grammar input file" << WHITE << std::endl;
        exit(FILE_ERROR);
    }

    std::string line;
    while (getline(in, line)) {
        if (!line.empty()) {
            extract(line);
        }
    }
    in.close();

    calc_firsts();
    calc_follows();
    make_table();
    write_table();
}

void SyntaxAnalyzer::make_tree(bool update) {
    if (update) {
        update_grammar();
    } else {
        read_table();
    }
    set_matches();

    std::stack<Node<Symbol> *> stack;
    int index = 0;
    tokens.emplace_back(Eof);
    int tokens_len = (int) tokens.size();

    auto *Eof_node = new Node<Symbol>(Symbol("$", TERMINAL), nullptr);
    stack.push(Eof_node);
    auto *root = new Node<Symbol>(Symbol(START_VAR, VARIABLE), nullptr);
    stack.push(root);

    tree.set_root(root);

    while (index < tokens_len && !stack.empty()) {
        Node<Symbol> *top_node = stack.top();
        Symbol top_var = top_node->get_data();
        stack.pop();

        Symbol term = Symbol(match[tokens[index].get_type()], TERMINAL);
        int line_number = tokens[index].get_line_number();

        if (top_var.get_type() == TERMINAL) {
            if (term == top_var) {
                std::string content = tokens[index].get_content();
                top_node->get_data().set_content(content);
                top_node->get_data().set_line_number(line_number);
                index++;
            } else {
                std::cerr << RED << "Syntax Error: Terminals don't match, line: " << line_number << WHITE
                          << std::endl;
                std::cerr << RED
                          << "Expected '" + top_var.get_name() + "' , but found '" + term.get_name() + "' instead."
                          << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
        } else {
            Rule rule = table[{top_var, term}];
            if (rule.get_type() == VALID) {
                top_node->get_data().set_line_number(line_number);

                std::vector<Symbol> body = rule.get_body();
                std::reverse(body.begin(), body.end());
                for (const auto &var: body) {
                    auto *node = new Node<Symbol>(var, top_node);
                    top_node->push_front_children(node);
                    if (var != eps) {
                        stack.push(node);
                    }
                }
            } else if (rule.get_type() == SYNCH) {
                std::cerr << RED << "Syntax Error: Synch, line: " << line_number << WHITE << std::endl;
                std::cerr << RED
                          << "Synced by adding '" + top_var.get_name() + "' before '" + term.get_name() + "'."
                          << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            } else if (rule.get_type() == EMPTY) {
                std::cerr << RED << "Syntax Error: Empty cell, line: " << line_number << WHITE << std::endl;
                std::cerr << RED << "Ignored '" + term.get_name() + "' due to syntax mismatch." << WHITE
                          << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                index++;
                stack.push(top_node);
                num_errors++;
            } else {
                std::cerr << "Unknown Error: ?, line: " << line_number << WHITE << std::endl;
                std::cerr << "---------------------------------------------------------------" << std::endl;
                num_errors++;
            }
        }
    }
    if (num_errors == 0) {
        std::cout << GREEN << "Parsed tree successfully" << WHITE << std::endl;
    } else {
        std::cout << YELLOW << "Parsed tree unsuccessfully" << WHITE << std::endl;
    }
}

void SyntaxAnalyzer::write() {
    if (num_errors) {
        return;
    }

    out.open(out_address);
    if (!out.is_open()) {
        std::cerr << RED << "File Error: Couldn't open output file" << WHITE << std::endl;
        exit(FILE_ERROR);
    }
    std::fill(has_par, has_par + 200, false);
    write_tree(tree.get_root());
    out.close();
}

Tree<Symbol> SyntaxAnalyzer::get_tree() const {
    return tree;
}