#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "utils.h"
#include "config.h"
#include "lexer.h"

/**
 * run_builtin() - Check if @cmd is a valid built-in command and if so, try to run it.
*/
ssize_t run_builtin(struct Command *cmd)
{
    if (!strcmp(cmd->name, "cd")) {
        // TODO: cd to home like bash
        if (cmd->argv[1] == NULL) {
            fputs("cd: no directory provided\n", stderr);
            return 1;
        }

        if (cmd->argv[2] != NULL) {
            fputs("cd: too many arguments\n", stderr);
            return 1;
        }

        if (chdir(cmd->argv[1]) < 0) {
            perror("cd");
            return 1;
        }

        return 1;
    } else if (!strcmp(cmd->name, "exit")) {
        exit(0);
    } 

    return 0;
}

/**
 * run_cmd() - Try to run the specified command
 *
 * Return:
 * >= 0: exit code of the command run
 * < 0: error
*/
int run_cmd(struct Command *cmd)
{
    if (run_builtin(cmd) > 0)
        return 0;

    int frk = fork();
    if (frk == 0) {
        // child
        if (cmd->redir) {
            int flags = cmd->redir_append ?
                O_WRONLY | O_CREAT | O_APPEND :
                O_WRONLY | O_CREAT;

            // rw-r--r--
            int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

            int fd = open(cmd->redir, flags, mode);
            if (fd < 0) {
                perror("[sys_open] redirection");
                exit(1);
            }

            // replace stdout with newly opened file
            if (dup2(fd, 1) < 0) {
                perror("[sys_dup] redirection");
                exit(1);
            }
        }

        execvp(cmd->name, cmd->argv);

        // exec should never return when successful
        perror("exec");
        exit(1);
    } else if (frk > 0) {
        // parent
        int child_status;
        waitpid(frk, &child_status, 0);
        return child_status;
    } else {
        // error
        perror("fork");
        return -1;
    }

    return 0; // unreachable
}

/**
 * next_cmd() - Read the next valid Command from @tokens into @cmd
 *
 * Return:
 * -1: an error has occured
 *  0: the last command has been parsed and there are no more commands
 * >0: a command has been parsed but more might follow. Seek forward the tokens array
 *     by @Return elements on the next call.
*/
ssize_t next_cmd(const char *tokens[], struct Command *cmd)
{
    if (tok_is_operator(tokens[0])) {
        fputs("scsh: syntax error: unexpected operator as first token\n", stderr);
        return -1;
    }

    cmd->endop = operators[OP_NONE];

    cmd->name = tokens[0];
    cmd->argv = (char *const *)tokens;
    cmd->redir = NULL;
    cmd->redir_append = false;

    const char **curr = tokens;

    while (*curr != NULL) {
        if (*curr == operators[OP_REDIR] ||
            *curr == operators[OP_REDIR_APPEND]) {
            if (*(curr + 1) != NULL) {
                cmd->redir = *(curr + 1);
                // mark redir_append
                cmd->redir_append = *curr == operators[OP_REDIR_APPEND];

                // terminate argv
                *curr = NULL;

                curr += 2;
                continue;
            } else {
                fputs("scsh: redirection target file expected.\n", stderr);
                return -1;
            }
        } else if (*curr == operators[OP_AMPERSAND]) {
            todo("ampersand token handler");
        } else if (tok_is_operator(*curr)) {
            // a different operator, store it as part of cmd
            cmd->endop = *curr;
            *curr = NULL;

            return curr - tokens + 1;
        }

        curr++;
    }

    return 0;
}


/**
 * interp_line() - Interpret a single shell instruction line (which can contain many expressions)
 * @line
 *
 * Return:
 * -1: an error has occured.
 *  0: success.
 *  1: invalid command.
*/
int interp_line(char *line)
{
    const char *tokens[CONF_MAX_TOKENS];
    ssize_t tok_count, ns, seek = 0;
    struct Command cmd;

    if ((tok_count = tokenize_line(line, tokens, CONF_MAX_TOKENS)) < 0) {
        fputs("scsh: syntax error", stderr);
        return 1;
    }

    do {
        ns = next_cmd(tokens + seek, &cmd);
        if (ns < 0) // error
            return 1;

        //print_cmd(&cmd);
        run_cmd(&cmd);

        seek += ns;
    } while (ns > 0);

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

        interp_line(line);
    }

    free(line);
}

int main(int argc, char **argv)
{
    if (argc <= 1) {
        run(stdin);
    } else if (argc == 2) {
        FILE* file = fopen(argv[1], "r");
        if (file == NULL) {
            fprintf(stderr, "%s: %s", argv[1], strerror(errno));
            return 1;
        }

        run(file);
        fclose(file);
    } else {
        fputs("scsh: too many arguments\n", stderr);
        return 1;
    }

    return 0;

}
