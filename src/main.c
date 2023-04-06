#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "./utils.h"

extern char **environ;

struct Command {
    char *name;
    char **argv;
    bool builtin;
    // 'end operators' as in "&&", "||" or "|" that determine what happens with the next command that is run
    const char *endop;
    // a file to redirect output to if '>' or '>>' was specified
    const char *redir;
    bool redir_append;
};

static const char *endops[] = { "&&", "||", "|", ";", NULL };
enum ENDOPS {
    ENDOP_AND,
    ENDOP_OR,
    ENDOP_PIPE,
    ENDOP_SEMICOLON,
};

void print_cmd(struct Command *cmd)
{
    printf("Command {\nname: %s, argv: ", cmd->name);

    for(int i = 0; cmd->argv[i] != NULL; i++)
        printf("[%s] ", cmd->argv[i]);

    printf("\nbuiltin: %b\nendop: %s\nredir: %s\n}\n", cmd->builtin, cmd->endop, cmd->redir);
}


/**
 * run_builtin() - Check if @cmd is a valid built-in command and if so, try to run it.
*/
ssize_t run_builtin(struct Command *cmd)
{
    if (!strcmp(cmd->name, "cd")) {
        // TODO: cd to home like bash
        if (cmd->argv[1] == NULL) {
            fputs("cd: no directory provided", stderr);
            return 1;
        }

        if (cmd->argv[2] != NULL) {
            fputs("cd: too many arguments", stderr);
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
*/
ssize_t run_cmd(struct Command *cmd)
{
    if (run_builtin(cmd) > 0)
        return 0;

    pid_t f = fork();
    if (f < 0) {
        perror("fork");
        return f;
    }

    if (f == 0) {
        if (cmd->redir != NULL) {
            int redir_fd = open(cmd->redir,
                                cmd->redir_append ? O_WRONLY | O_CREAT | O_APPEND : O_WRONLY | O_CREAT,
                                S_IRUSR | S_IWUSR);
            if (redir_fd < 0) {
                perror("redir");
                exit(1);
            }
            dup2(redir_fd, 1);
        }

        execvp(cmd->name, cmd->argv);
        perror("exec"); // exec has returned, which only happens on error
        exit(1);
    } else {
        waitpid(f, NULL, 0);
    }

    return 0;
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
ssize_t next_cmd(char *tokens[], size_t count, struct Command *cmd)
{
    cmd->endop = NULL;
    cmd->redir = NULL;
    cmd->redir_append = false;

    cmd->name = tokens[0];
    cmd->argv = tokens;

    for (int i = 1; tokens[i] != NULL; i++) {
        // check for redirection
        if (*(tokens[i]) == '>') {
            if (*(tokens[i] + 1) == '>' && *(tokens[i] + 2) == 0) {
                cmd->redir_append = true;
            } else if (*(tokens[i] + 1) != 0) {
                continue;
            }
           
            if (tokens[i+1] == NULL || tokens[i+2] != NULL) {
                fputs("redir: too many or no redir files provided\n", stderr);
                return -1;
            }

            cmd->redir = tokens[i+1];
            tokens[i] = NULL;

            i++; // advance by two tokens to skip the redir file name
            continue;
        }

        // check for other ending operators
        for (int j = 0; endops[j] != NULL; j++) {
            if (!strcmp(tokens[i], endops[j])) {
                cmd->endop = endops[j];
                tokens[i] = NULL;
                return count - i;
            }
        }
    }

    return 0;
}

/**
 * tokenize_line() - Split a line into tokens (interpreting quoted strings as one string)
*/
size_t tokenize_line(char *str, char *tokens[], size_t size)
{
    char **tok_ptr = tokens;
#define AUX_SIZE 256
    char *aux_buff[AUX_SIZE];
    size_t sa = split_into(str, aux_buff, AUX_SIZE, "\"");

    for (size_t i = 0; i < sa && tok_ptr < tokens+size-1; i += 2) {
        size_t st = split_into(aux_buff[i], tok_ptr, size - (tok_ptr - tokens), " ");
        tok_ptr += st;

        if (i != sa - 1) {
            *tok_ptr = aux_buff[i+1];
            tok_ptr++;
        }
    }
    *tok_ptr = NULL;

    return tok_ptr - tokens;
}

/**
 * interp_line() - Interpret a single shell instruction line (which can contain many commands)
 * @line: the line to interpret
 *
 * Return:
 * -1: an error has occured and it should be handled outside the function.
 *  0: invalid command.
 *  1: success.
*/
int interp_line(char *line)
{
    // split the line into parallel paths.
#define MAX_PARALLEL_CMDS 256
    char *parallel[MAX_PARALLEL_CMDS] = {NULL};
#define MAX_TOKENS 512
    char *tokens[MAX_TOKENS];

    struct Command curr_cmd;

    size_t cc = split_into(line, parallel, MAX_PARALLEL_CMDS, "&");

    for (size_t i = 0; i < cc; i++) {
        size_t tc = tokenize_line(parallel[i], tokens, MAX_TOKENS);
        ssize_t seek = 0;

        do {
            seek = next_cmd(tokens + seek, tc, &curr_cmd);
            if (seek < 0)
                break;
            //print_cmd(&curr_cmd);
            run_cmd(&curr_cmd);
        } while (seek > 0);
    }

    return 1;
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
    (void)argv;
    if (argc <= 1) {
        run(stdin);
    } else if (argc == 2) {
        FILE* file = fopen(argv[1], "r");
        if (file == NULL) {
            fprintf(stderr, "%s: %s", argv[1], strerror(errno));
            return 1;
        }

        run(file);
    } else {
        fputs("scsh: too many arguments\n", stderr);
        return 1;
    }

    return 0;
}
