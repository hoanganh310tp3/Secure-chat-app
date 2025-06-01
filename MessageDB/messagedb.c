#include "messagedb.h"

bool initializeDatabase() {
    FILE *file = fopen(DB_FILENAME, "a"); // Create file if it doesn't exist
    if (file == NULL) {
        printf("Error: Could not initialize database file\n");
        return false;
    }
    fclose(file);
    return true;
}

char* getCurrentTimestamp() {
    static char timestamp[MAX_TIMESTAMP_LENGTH];
    time_t rawtime;
    struct tm *timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", timeinfo);
    return timestamp;
}

bool saveMessage(const char* message) {
    FILE *file = fopen(DB_FILENAME, "a");
    if (file == NULL) {
        printf("Error: Could not open database file for writing\n");
        return false;
    }
    
    char* timestamp = getCurrentTimestamp();
    fprintf(file, "%s %s\n", timestamp, message);
    fclose(file);
    
    return true;
}

bool loadChatHistory() {
    FILE *file = fopen(DB_FILENAME, "r");
    if (file == NULL) {
        printf("No chat history found. Starting fresh.\n");
        return true; // Not an error, just no history yet
    }
    
    printf("=== Loading Chat History ===\n");
    char line[MAX_MESSAGE_LENGTH + MAX_TIMESTAMP_LENGTH];
    
    while (fgets(line, sizeof(line), file) != NULL) {
        // Remove newline character
        line[strcspn(line, "\n")] = 0;
        printf("%s\n", line);
    }
    
    printf("=== End of Chat History ===\n\n");
    fclose(file);
    return true;
}

void printChatHistory() {
    loadChatHistory();
} 