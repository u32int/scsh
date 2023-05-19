#ifndef _LEXER_H
#define _LEXER_H

#include <stdbool.h>

// see the lexer.c file for the array of operators
extern const char *operators[];
enum Operator {
    OP_AND = 0,
    OP_AMPERSAND,
    OP_OR,
    OP_PIPE,
    OP_REDIR_APPEND,
    OP_REDIR,
    OP_REDIR_IN,
    OP_SEMICOLON,
    OP_NONE
};

int tokenize_line(char *line, const char *tokens[], bool free_list[], size_t size);
bool tok_is_operator(const char *str);

#endif
