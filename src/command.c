#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "command.h"
#include "lexer.h"
#include "utils.h"
#include "shellstate.h"

/**
 * run_builtin() - Check if @cmd is a valid built-in command and if so, try to run it
 *
 * Return is > 0 if the command was a builtin
*/
ssize_t run_builtin(struct Command *cmd)
{
    if (!strcmp(cmd->name, "cd")) {
        char *dir = cmd->argv[1];

        if (cmd->argv[1] == NULL) {
            dir = getenv("HOME");
            if (!dir) {
                fputs("cd: no directory provided and no HOME envvar set.\n", stderr);
                return 1;
            }
            return 1;
        } else if (!strcmp(cmd->argv[1], "-")) {
            if (SHELL_STATE.oldpwd[0] == 0) {
                fputs("scsh: error: no oldpwd yet!\n", stderr);
                return 1;
            }

            dir = SHELL_STATE.oldpwd;
        }

        if (chdir(dir) < 0)
            perror("cd");

        strncpy(SHELL_STATE.oldpwd, SHELL_STATE.pwd, CONF_MAX_DIR_LEN);
        getcwd(SHELL_STATE.pwd, CONF_MAX_DIR_LEN);
        /* TODO using putenv would probably be better here as this makes an unnecessary copy */
        setenv("PWD",    SHELL_STATE.pwd, 1);
        setenv("OLDPWD", SHELL_STATE.oldpwd, 1);
        return 1;
    } else if (!strcmp(cmd->name, "exit")) {
        exit(0);
    } 

    return 0;
}

void do_exec(struct Command *cmd)
{
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

    execvp(cmd->name, cmd->argv); // exec never returns when successful
    panic("scsh: exec");
}

/**
 * run_cmd() - Try to run the specified command
 *
 * Return: the next command to run
*/
int run_cmd(struct Command *cmd)
{
    if (run_builtin(cmd) > 0)
        return 0;

    int frk_outer = fork();
    if (frk_outer == 0) {
        while (cmd->op == operators[OP_PIPE]) {
            int pipefds[2];
            if (pipe(pipefds) < 0)
                panic("scsh: pipe");

            int frk_pipe = fork();
            if (frk_pipe == 0) {
                // frk_pipe child
                if (dup2(pipefds[1], 1) < 0) // stdout -> pipe write
                    panic("scsh: dup");
                do_exec(cmd);
            } else if (frk_pipe > 0) {
                // frk_pipe parent
                cmd = cmd->next;
                if (dup2(pipefds[0], 0) < 0) // pipe read -> stdin
                    panic("scsh: dup");
                if (close(pipefds[1]) < 0) // close unused write end (if we don't, read will block)
                    panic("scsh: close");
            } else {
                // frk_pipe error
                panic("scsh: fork");
            }
        }

        do_exec(cmd);
    } else if (frk_outer > 0) {
        int status;
        waitpid(frk_outer, &status, 0);
        if (WIFEXITED(status))
            SHELL_STATE.last_exit = WEXITSTATUS(status);
        return status;
    } else {
        panic("scsh: pipe");
    }

    __builtin_unreachable();
}

/**
 * fill_cmd() - Read the next valid Command from @tokens into @cmd
 *
 * Return:
 * -1: an error has occured
 *  0: the last command has been parsed and there are no more commands
 * >0: a command has been parsed but more might follow. Seek forward the tokens array
 *     by @Return elements on the next call.
*/
int fill_cmd(const char *tokens[], struct Command *cmd)
{
    cmd->next = NULL;
    cmd->prev = NULL;
    cmd->op = operators[OP_NONE];
    cmd->name = tokens[0];
    cmd->argv = (char *const *)tokens;
    cmd->redir = NULL;
    cmd->redir_append = false;

    const char **tok = tokens;
    while (*tok != NULL) {
        if (*tok == operators[OP_REDIR] ||
            *tok == operators[OP_REDIR_APPEND]) {
            // setup redirection
            // checking this might not be necessary anymore since the lexer performs validation
            if (*(tok + 1) != NULL) {
                cmd->redir = *(tok + 1);
                // mark redir_append
                cmd->redir_append = *tok == operators[OP_REDIR_APPEND];

                // terminate argv
                *tok = NULL;

                tok += 2;
                continue;
            }
        }

        if (tok_is_operator(*tok)) {
            // operator, store it as part of cmd
            cmd->op = *tok;
            *tok = NULL;

            return (*(tok + 1) == NULL) ? 0 : tok - tokens + 1;
        }

        tok++;
    }

    return 0;
}

/*
 * cmd_list_from_tok() - Fill a linked list of [struct Command *] using @tokens
 *
 * Return: NULL on failure, ptr to allocated [struct Command *] linked list on success.
*/
struct Command *cmd_list_from_tok(const char *tokens[])
{
    struct Command *root, *curr;
    int seek;

    root = malloc(sizeof(struct Command));
    if (!root)
        panic("malloc");

    curr = root;
    for(;;) {
        seek = fill_cmd(tokens, curr);
        if (seek == -1) {
            // error, free whatever was allocated
            free_cmd_list(root);
            return NULL;
        }
        else if (seek == 0)
            break;
        tokens += seek;
        // print_cmd(curr);

        curr->next = malloc(sizeof(struct Command));
        if (!curr->next)
            panic("malloc");

        curr->next->prev = curr;
        curr = curr->next;
    }

    curr->next = NULL;

    return root;
}
