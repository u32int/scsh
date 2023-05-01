#ifndef _LEXER_H
#define _LEXER_H

// see the lexer.c file for the array of operators
extern const char *operators[];
enum Operator {
    OP_AND,
    OP_AMPERSAND,
    OP_OR,
    OP_PIPE,
    OP_REDIR_APPEND,
    OP_REDIR,
    OP_SEMICOLON,
    OP_NONE
};

struct Command {
    const char *name;
    char *const *argv;
    // 'end operators' as in "&&", "||" or "|" that determine what happens with the next command that is run
    const char *endop;
    // a file to redirect output to if '>' or '>>' was specified
    // TODO: '<'
    const char *redir;
    bool redir_append;
};

int tokenize_line(char *line, const char **tokens, size_t size);
bool tok_is_operator(const char *str);

#endif
