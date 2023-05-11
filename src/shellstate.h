#ifndef _SHELLSTATE_H
#define _SHELLSTATE_H

#include "config.h"

struct ShellState {
    int last_exit;
    char pwd[CONF_MAX_DIR_LEN], oldpwd[CONF_MAX_DIR_LEN];
};

extern struct ShellState SHELL_STATE;

#endif /* _SHELLSTATE_H */
