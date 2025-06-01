#ifndef MESSAGEDB_H
#define MESSAGEDB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define MAX_MESSAGE_LENGTH 1024
#define MAX_TIMESTAMP_LENGTH 32
#define DB_FILENAME "chat_history.txt"

struct Message {
    char timestamp[MAX_TIMESTAMP_LENGTH];
    char content[MAX_MESSAGE_LENGTH];
};

// Function declarations
bool initializeDatabase();
bool saveMessage(const char* message);
bool loadChatHistory();
void printChatHistory();
char* getCurrentTimestamp();

#endif // MESSAGEDB_H 