CC ?= gcc
CFLAGS ?= -Wall -Wextra -g

CFILES = src/main.c src/utils.c
OUT = scsh

all: $(OUT)

$(OUT): $(CFILES)
	$(CC) $(CFILES) $(CFLAGS) -o $(OUT)

clean:
	rm $(OUT)
