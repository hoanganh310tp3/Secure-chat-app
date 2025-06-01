CC=gcc
CFLAGS=-Wall -Wextra -std=c99
SOURCES=auth.c
OBJECTS=$(SOURCES:.c=.o)
LIBS=-lcrypt

all: $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBS)

clean:
	rm -f $(OBJECTS)

.PHONY: all clean 