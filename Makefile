CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror
DBG_CFLAGS ?= -Wall -Wextra -g3 -fsanitize=undefined,address -Wno-unused-variable -Wno-unused-parameter -Wno-unused-function

CFILES = src/main.c src/utils.c src/lexer.c src/command.c
OUT = scsh

all: $(CFILES)
	$(CC) $(CFLAGS) $(CFILES) -o $(OUT)

debug: $(CFILES)
	$(CC) $(DBG_CFLAGS) $(CFILES) -o $(OUT)

clean: $(OUT)
	rm $(OUT)

.PHONY: all clean debug
