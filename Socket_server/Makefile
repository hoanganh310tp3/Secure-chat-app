CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread
SRCDIR = ../SocketUtil
DBDIR = ../MessageDB
AUTHDIR = ../Auth
SOURCES = $(SRCDIR)/socketutil.c $(DBDIR)/messagedb.c $(AUTHDIR)/auth.c main.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = server
LIBS = -lcrypt -lssl -lcrypto  

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean