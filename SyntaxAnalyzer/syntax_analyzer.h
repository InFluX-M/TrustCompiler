#ifndef SYNTAX_ANALYZER_H
#define SYNTAX_ANALYZER_H

#include "../utils.h"

#define GRAMMAR_PATH "../grammar.txt"
#define TABLE_PATH "../table.txt"
#define START_VAR "program"

enum rule_type {
    VALID,
    SYNCH,
    EMPTY
};

const Symbol eps = Symbol("eps", TERMINAL);

#endif // SYNTAX_ANALYZER_H