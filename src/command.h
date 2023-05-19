#ifndef _COMMAND_H
#define _COMMAND_H

#include <stdbool.h>

struct Command {
    const char *name;
    char *const *argv;
    // operator associated with the command
    const char *op;
    // files to redirect output to if '>', '>>' or '<' was specified.
    const char *redir;
    bool redir_append;
    const char *redir_in;
    // linked list
    struct Command *next, *prev;
};

int run_cmd(struct Command *cmd);
struct Command *cmd_list_from_tok(const char *tokens[]);

#endif /* _COMMAND_H */
