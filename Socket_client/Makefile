CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread
SOCKETUTIL_DIR = ../SocketUtil

TARGET = client

SOURCES = main.c $(SOCKETUTIL_DIR)/socketutil.c

INCLUDES = -I$(SOCKETUTIL_DIR)

OBJECTS = $(SOURCES:.c=.o)

LIBS = -lssl -lcrypto

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run