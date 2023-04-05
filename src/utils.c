#include <stdio.h>
#include <string.h>

#include "./utils.h"

void print_strarray(char **array) {
    fputs("[ ", stdout);
    for (int i = 0; array[i] != NULL; i++) {
        printf("<%s> ", array[i]);
    }
    fputs("]\n", stdout);
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

