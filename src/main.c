#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>

#include "utils.h"
#include "config.h"
#include "lexer.h"
#include "command.h"
#include "shellstate.h"

struct ShellState SHELL_STATE;

/**
 * exec_line() - Execute a single shell instruction line
 *
 * Return:
 * -1: an error has occured.
 *  0: success.
 *  1: invalid command.
*/
int exec_line(char *line)
{
    const char *tokens[CONF_MAX_TOKENS];
    bool free_list[CONF_MAX_TOKENS] = { false };
    struct Command *root, *cmd;

    if(tokenize_line(line, tokens, free_list, CONF_MAX_TOKENS) < 0) {
        free_array((void **)tokens, free_list, CONF_MAX_TOKENS);
        return 1;
    }

    if (tokens[0] == NULL)
        return 0;
 
    root = cmd_list_from_tok(tokens);
    if (!root) {
        free_array((void **)tokens, free_list, CONF_MAX_TOKENS);
        return 1;
    }

    //print_cmd_list(root);
    cmd = root;
    while (cmd) {
        if (cmd->prev) {
            if (cmd->prev->op == operators[OP_AND] && SHELL_STATE.last_exit != 0)
                break;

            if (cmd->prev->op == operators[OP_OR] && SHELL_STATE.last_exit == 0)
                break;
        }
        run_cmd(cmd);
        // seek to the nearest non-pipe (this is a suboptimal way of handling this, we should probably somehow
        // return this info from the func)
        while(cmd && cmd->op == operators[OP_PIPE])
            cmd = cmd->next;

        if (cmd)
            cmd = cmd->next;
    }

    free_cmd_list(root);
    free_array((void **)tokens, free_list, CONF_MAX_TOKENS);
    return 0;
}

/**
 * run() - Start the main shell loop
 * @stream: the stream to read instruction lines from
 *
*/
void run(FILE *stream)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    for (;;) {
        if (stream == stdin)
            fputs("scsh $ ", stdout);

        nread = getline(&line, &len, stream);
        if (nread < 0)
            break;

        if (line[nread-1] == '\n')
            line[nread-1] = 0; // truncate newline

        if (*line == 0 || *line == '#')
            continue; // empty or comment line, ignore

        exec_line(line);
    }

    free(line);
}

void setup()
{
    getcwd(SHELL_STATE.pwd, CONF_MAX_DIR_LEN);
}

int main(int argc, char **argv)
{
    if (argc <= 1) {
        setup();
        run(stdin);
    } else if (argc == 2) {
        FILE* file = fopen(argv[1], "r");
        if (file == NULL) {
            fprintf(stderr, "%s: %s", argv[1], strerror(errno));
            return 1;
        }

        setup();
        run(file);
        fclose(file);
    } else {
        fputs("scsh: too many arguments\n", stderr);
        return 1;
    }

    return 0;
}
