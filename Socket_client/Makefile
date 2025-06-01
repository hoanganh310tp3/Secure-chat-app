CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread
SOCKETUTIL_DIR = ../SocketUtil

TARGET = client

SOURCES = main.c $(SOCKETUTIL_DIR)/socketutil.c

INCLUDES = -I$(SOCKETUTIL_DIR)

OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run