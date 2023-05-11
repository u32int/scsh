#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"
#include "command.h"
#include "lexer.h"

extern unsigned char LAST_EXIT_CODE;

// the order of operators matters here, the onces at lower indices get higher priority in the lexer 
const char *operators[] = { "&&", "&", "||", "|", ">>", ">", ";", NULL };

bool tok_is_operator(const char *token)
{
    return token >= operators[0] && token <= operators[OP_NONE-1];
}

/* expand_variable() - expands a variable allocating memory for it on the heap or pointing to an env var.
 *
 * Return:
 * 0: variable does not exits and has not been expanded. No memory has been allocated.
 * 1: variable is valid and @var_ptr has been replaced with a pointer to the allocted memory. It is
 *    the responsibility of the caller to free the memory.
 * 2: variable is a valid env var. Since env vars aren't allocated on the heap, _don't_ call free() on it.
*/
int expand_variable(char **var_ptr)
{
    const char *var = *var_ptr + 1; // skip '$'
    if (!strcmp(var, "?")) {
        *var_ptr = malloc(4); // exit codes range 0-255 - max 3 chars + null terminator
        if (!var_ptr)
            panic("scsh: malloc");

        snprintf(*var_ptr, 4, "%u", LAST_EXIT_CODE);
        return 1;
    } else {
        char *envvar = getenv(var);
        if (envvar) {
            *var_ptr = envvar;
            return 2;
        }
    }

    return 0;
}

/* add_token() - helper function for tokenize_line()
*/
int add_token(const char *tokens[], size_t *tki, char *tok_start, bool free_list[])
{
    tokens[*tki] = tok_start;
    if (tokens[*tki][0] == '$') {
        int ex = expand_variable((char **)&tokens[*tki]);
        if (!ex) {
            fprintf(stderr, "scsh: '%s' is not a valid variable\n", tokens[*tki]);
            return -1;
        }
        if (ex == 1) // can't just free_list[*tki] = ex, check return of expand_variable()
            free_list[*tki] = true;
    }
    (*tki)++;

    return 0;
}

/* validate_tokens() - does a second pass across the tokens array to check for syntax errors that are awkward
 *                     in the first pass.
*/
int validate_tokens(const char *tokens[], size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (tok_is_operator(tokens[i])) {
            if (i == 0 || tok_is_operator(tokens[i - 1])) {
                fprintf(stderr, "scsh: syntax error: '%s' unexpected operator\n", tokens[i]);
                return -1;
            }

            if (i == len - 1 &&
                tokens[i] != operators[OP_SEMICOLON] &&
                tokens[i] != operators[OP_AMPERSAND]) {
                fprintf(stderr, "scsh: syntax error: '%s' unexpected operator at the end of a command\n",
                        tokens[i]);
                return -1;
            }
        }
    }

    return 0;
}

/* tokenize_line() - turn a passed string into an array of tokens
 *
 * @size: size of tokens and free_list
 *
 * Return
 * -1: parse error, the tokens array is unusable
 *>=0: the number of tokens generated
*/
int tokenize_line(char *line, const char *tokens[], bool free_list[], size_t size)
{
    char *curr = line;
    size_t tki = 0;

    // trim start
    while(*curr == ' ')
        curr++;

    char *prev = curr;
    while (*curr != 0 && tki < size) {
        switch (*curr) {
        // whitespace serves as a delimiter between tokens (though it is optional)
        case ' ': {
            // avoid making 0 width tokens (essentially ignore excess whitespace)
            if (prev == curr) {
                curr++; prev++;
                continue;
            }

            // add token
            *curr = 0;
            if(add_token(tokens, &tki, prev, free_list) < 0)
                return -1;

            curr++;
            prev = curr;
        } break;
        // double quotes signify a contigious chunk of text
        case '\"': {
            curr++;
            prev = curr;

            // seek until the next quote
            while (*curr != '\"' && *curr != 0)
                curr++;


            if (*curr == 0) {
                // maybe instead of throwing an error here we could do something similar to bash
                // and let the user continue typing on the next line (PS2/'> ')
                fputs("scsh: parse error: unterminated quote (\")\n", stderr);
                return -1;
            }

            // add the new token
            *curr = 0;
            tokens[tki] = prev;
            tki++;

            curr++;
            prev = curr;
        } break;
        default: {
            // check for operators
            bool is_op = false;
            for (int i = 0; operators[i] != NULL; i++) {
                // uhhh this is not great and we could do better performance wise
                size_t oplen = strlen(operators[i]);
                if (!strncmp(operators[i], curr, oplen)) {
                    if (*(curr + 1) == 0 && i == OP_PIPE) {
                        fputs("scsh: parse error: unterminated pipe\n", stderr);
                        return -1;
                    }

                    is_op = true;

                    // save preceding string, if any (ex. in 'ls&&' the 'ls')
                    if (curr != prev) {
                        *curr = 0;
                        tokens[tki] = prev;
                        tki++;
                    }

                    // add the op to the list of tokens
                    tokens[tki] = operators[i];
                    tki++;

                    curr += oplen;
                    prev = curr;
                    break;
                }
            }

            if (!is_op)
                curr++;
        } break;
        }
    }

    // add the remaining part as one token
    if (prev != curr) {
        *curr = 0;
        if(add_token(tokens, &tki, prev, free_list) < 0)
            return -1;
    }
    tokens[tki] = NULL;

    if (validate_tokens(tokens, tki) < 0) {
        return -1;
    }

    return tki;
}
