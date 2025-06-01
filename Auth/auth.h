#ifndef AUTH_H
#define AUTH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <crypt.h>

#define MAX_USERNAME_LENGTH 32
#define MAX_PASSWORD_LENGTH 64
#define MAX_SESSION_TOKEN_LENGTH 64
#define MAX_SALT_LENGTH 16
#define USERS_DB_FILENAME "users.db"
#define SESSIONS_DB_FILENAME "sessions.db"
#define SESSION_TIMEOUT 3600 // 1 hour in seconds

typedef struct {
    char username[MAX_USERNAME_LENGTH];
    char password_hash[MAX_PASSWORD_LENGTH];
    char salt[MAX_SALT_LENGTH];
    time_t created_at;
    bool is_active;
} User;

typedef struct {
    char username[MAX_USERNAME_LENGTH];
    char session_token[MAX_SESSION_TOKEN_LENGTH];
    time_t created_at;
    time_t last_activity;
    bool is_active;
    int socket_fd;
} Session;

// Authentication functions
bool initializeAuth();
bool registerUser(const char* username, const char* password);
bool authenticateUser(const char* username, const char* password);
char* createSession(const char* username, int socket_fd);
bool validateSession(const char* session_token);
bool updateSessionActivity(const char* session_token);
bool logoutUser(const char* session_token);
void cleanupExpiredSessions();

// Utility functions
char* generateSalt();
char* hashPassword(const char* password, const char* salt);
char* generateSessionToken();
bool saveUser(const User* user);
bool loadUser(const char* username, User* user);
bool saveSession(const Session* session);
bool loadSession(const char* session_token, Session* session);
char* getUsernameFromSession(const char* session_token);

#endif // AUTH_H 