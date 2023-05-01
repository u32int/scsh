#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "utils.h"
#include "lexer.h"

// the order of operators matters here, the onces at lower indices get higher priority in the lexer 
const char *operators[] = { "&&", "&", "||", "|", ">>", ">", ";", NULL };

/* tokenize_line() - turn a passed string into an array of tokens
 *
 * Return
 * -1: parse error, the tokens array is unusable
 *>=0: the number of tokens generated
*/
int tokenize_line(char *line, const char **tokens, size_t size)
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
            tokens[tki] = prev;
            tki++;

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
                fputs("parse error: unterminated '\"' (quote)", stderr);
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
        tokens[tki] = prev;
        tki++;
    }

    tokens[tki] = NULL;

    return tki;
}

bool tok_is_operator(const char *token)
{
    return token >= operators[0] && token <= operators[OP_NONE-1];
}
