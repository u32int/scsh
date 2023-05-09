#ifndef _COMMAND_H
#define _COMMAND_H

#include <stdbool.h>

struct Command {
    const char *name;
    char *const *argv;
    // operator associated with the command
    const char *op;
    // a file to redirect output to if '>' or '>>' was specified. This data is outside of the union because
    // it can be combined with other operators. TODO: '<'
    const char *redir;
    bool redir_append;
    struct Command *next, *prev;
};

int run_cmd(struct Command *cmd);
struct Command *cmd_list_from_tok(const char *tokens[]);

#endif /* _COMMAND_H */
