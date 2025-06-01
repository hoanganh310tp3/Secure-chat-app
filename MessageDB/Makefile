CC=gcc
CFLAGS=-Wall -Wextra -std=c99
SOURCES=messagedb.c
OBJECTS=$(SOURCES:.c=.o)

all: $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS)

.PHONY: all clean 