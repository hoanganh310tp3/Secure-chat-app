CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread
SOCKETUTIL_DIR = ../SocketUtil
DBDIR = ../MessageDB
TARGET = server

SOURCES = $(SOCKETUTIL_DIR)/socketutil.c $(DBDIR)/messagedb.c main.c

INCLUDES = -I$(SOCKETUTIL_DIR)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: clean run