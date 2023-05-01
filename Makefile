CC ?= gcc
CFLAGS_DEBUG ?= -Wall -Wextra -g3 -fsanitize=undefined,address -Wno-unused-variable -Wno-unused-parameter -Wno-unused-function
CFLAGS ?= -Wall -Wextra

CFILES = src/main.c src/utils.c src/lexer.c
OUT = scsh

all: $(OUT)

$(OUT): $(CFILES)
	$(CC) $(CFILES) $(CFLAGS_DEBUG) -o $(OUT)

clean:
	rm $(OUT)

.PHONY: all clean debug
