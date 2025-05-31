CC = gcc
CFLAGS = -Wall -Wextra -std=c99
SOCKETUTIL_DIR = ../SocketUtil
TARGET = server

SOURCES = main.c $(SOCKETUTIL_DIR)/socketutil.c

INCLUDES = -I$(SOCKETUTIL_DIR)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: clean run