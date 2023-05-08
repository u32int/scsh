#ifndef _UTILS_H
#define _UTILS_H

#include <stdbool.h>
#include "lexer.h"
#include "command.h"

void print_strarray(const char **array);
size_t split_into(char *str, char *array[], size_t size, const char *delim);
void print_cmd(struct Command *cmd);
void print_cmd_stream(struct Command *cmd);

_Noreturn void panic(const char *msg);
_Noreturn void todo(const char *msg);

#endif
