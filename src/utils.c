#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command.h"
#include "utils.h"

void print_strarray(const char **array) {
    fputs("[ ", stdout);
    for (int i = 0; array[i] != NULL; i++) {
        printf("<%s> ", array[i]);
    }
    fputs("]\n", stdout);
}

void print_cmd(struct Command *cmd)
{
    printf("Command {\nname: %s, argv: ", cmd->name);

    for(int i = 0; cmd->argv[i] != NULL; i++)
        printf("[%s] ", cmd->argv[i]);

    printf("\nop: %s\nredir: %s (append: %d) next %p\n}\n", cmd->op, cmd->redir, cmd->redir_append, cmd->next);
}

void print_cmd_stream(struct Command *cmd)
{
    while(cmd != NULL) {
        print_cmd(cmd);
        cmd = cmd->next;
    }
}

size_t split_into(char *str, char *array[], size_t size, const char *delim)
{
    size_t i;
    for (i = 0; i < size && str != NULL; i++) {
        array[i] = strsep(&str, delim);

        // ignore empty strings
        if (*array[i] == 0) {
            i--;
            continue;
        } 
    }

    array[i] = NULL;
    return i;
}

_Noreturn void panic(const char *msg)
{
    perror(msg);
    exit(1);
}

_Noreturn void todo(const char *msg)
{
    fprintf(stderr,"todo - \"%s\" is not implemented\n", msg);
    exit(1);
}
