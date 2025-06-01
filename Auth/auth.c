#include "auth.h"

bool initializeAuth() {
    // Create users database file if it doesn't exist
    FILE *users_file = fopen(USERS_DB_FILENAME, "a");
    if (users_file == NULL) {
        printf("Error: Could not initialize users database\n");
        return false;
    }
    fclose(users_file);
    
    // Create sessions database file if it doesn't exist
    FILE *sessions_file = fopen(SESSIONS_DB_FILENAME, "a");
    if (sessions_file == NULL) {
        printf("Error: Could not initialize sessions database\n");
        return false;
    }
    fclose(sessions_file);
    
    // Clean up expired sessions on startup
    cleanupExpiredSessions();
    
    return true;
}

char* generateSalt() {
    static char salt[MAX_SALT_LENGTH];
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    
    srand(time(NULL) + getpid());
    
    for (int i = 0; i < MAX_SALT_LENGTH - 1; i++) {
        salt[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    salt[MAX_SALT_LENGTH - 1] = '\0';
    
    return salt;
}

char* hashPassword(const char* password, const char* salt) {
    static char hash[MAX_PASSWORD_LENGTH];
    char salt_prefix[MAX_SALT_LENGTH + 4];
    
    snprintf(salt_prefix, sizeof(salt_prefix), "$6$%s", salt);
    char* result = crypt(password, salt_prefix);
    
    if (result != NULL) {
        strncpy(hash, result, MAX_PASSWORD_LENGTH - 1);
        hash[MAX_PASSWORD_LENGTH - 1] = '\0';
    } else {
        hash[0] = '\0';
    }
    
    return hash;
}

char* generateSessionToken() {
    static char token[MAX_SESSION_TOKEN_LENGTH];
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    
    srand(time(NULL) + getpid() + rand());
    
    for (int i = 0; i < MAX_SESSION_TOKEN_LENGTH - 1; i++) {
        token[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    token[MAX_SESSION_TOKEN_LENGTH - 1] = '\0';
    
    return token;
}

bool saveUser(const User* user) {
    FILE *file = fopen(USERS_DB_FILENAME, "a");
    if (file == NULL) {
        return false;
    }
    
    fprintf(file, "%s|%s|%s|%ld|%d\n", 
            user->username, 
            user->password_hash, 
            user->salt,
            user->created_at,
            user->is_active ? 1 : 0);
    
    fclose(file);
    return true;
}

bool loadUser(const char* username, User* user) {
    FILE *file = fopen(USERS_DB_FILENAME, "r");
    if (file == NULL) {
        return false;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), file) != NULL) {
        char stored_username[MAX_USERNAME_LENGTH];
        char password_hash[MAX_PASSWORD_LENGTH];
        char salt[MAX_SALT_LENGTH];
        long created_at;
        int is_active;
        
        if (sscanf(line, "%31[^|]|%63[^|]|%15[^|]|%ld|%d", 
                   stored_username, password_hash, salt, &created_at, &is_active) == 5) {
            
            if (strcmp(stored_username, username) == 0) {
                strcpy(user->username, stored_username);
                strcpy(user->password_hash, password_hash);
                strcpy(user->salt, salt);
                user->created_at = created_at;
                user->is_active = is_active == 1;
                fclose(file);
                return true;
            }
        }
    }
    
    fclose(file);
    return false;
}

bool registerUser(const char* username, const char* password) {
    // Check if user already exists
    User existing_user;
    if (loadUser(username, &existing_user)) {
        printf("User %s already exists\n", username);
        return false;
    }
    
    // Create new user
    User new_user;
    strcpy(new_user.username, username);
    
    char* salt = generateSalt();
    strcpy(new_user.salt, salt);
    
    char* hash = hashPassword(password, salt);
    strcpy(new_user.password_hash, hash);
    
    new_user.created_at = time(NULL);
    new_user.is_active = true;
    
    return saveUser(&new_user);
}

bool authenticateUser(const char* username, const char* password) {
    User user;
    if (!loadUser(username, &user)) {
        return false;
    }
    
    if (!user.is_active) {
        return false;
    }
    
    char* hash = hashPassword(password, user.salt);
    return strcmp(hash, user.password_hash) == 0;
}

bool saveSession(const Session* session) {
    FILE *file = fopen(SESSIONS_DB_FILENAME, "a");
    if (file == NULL) {
        return false;
    }
    
    fprintf(file, "%s|%s|%ld|%ld|%d|%d\n",
            session->username,
            session->session_token,
            session->created_at,
            session->last_activity,
            session->is_active ? 1 : 0,
            session->socket_fd);
    
    fclose(file);
    return true;
}

bool loadSession(const char* session_token, Session* session) {
    FILE *file = fopen(SESSIONS_DB_FILENAME, "r");
    if (file == NULL) {
        return false;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), file) != NULL) {
        char username[MAX_USERNAME_LENGTH];
        char token[MAX_SESSION_TOKEN_LENGTH];
        long created_at, last_activity;
        int is_active, socket_fd;
        
        if (sscanf(line, "%31[^|]|%63[^|]|%ld|%ld|%d|%d",
                   username, token, &created_at, &last_activity, &is_active, &socket_fd) == 6) {
            
            if (strcmp(token, session_token) == 0) {
                strcpy(session->username, username);
                strcpy(session->session_token, token);
                session->created_at = created_at;
                session->last_activity = last_activity;
                session->is_active = is_active == 1;
                session->socket_fd = socket_fd;
                fclose(file);
                return true;
            }
        }
    }
    
    fclose(file);
    return false;
}

char* createSession(const char* username, int socket_fd) {
    static char token[MAX_SESSION_TOKEN_LENGTH];
    strcpy(token, generateSessionToken());
    
    Session session;
    strcpy(session.username, username);
    strcpy(session.session_token, token);
    session.created_at = time(NULL);
    session.last_activity = time(NULL);
    session.is_active = true;
    session.socket_fd = socket_fd;
    
    if (saveSession(&session)) {
        return token;
    }
    
    return NULL;
}

bool validateSession(const char* session_token) {
    Session session;
    if (!loadSession(session_token, &session)) {
        return false;
    }
    
    if (!session.is_active) {
        return false;
    }
    
    // Check if session has expired
    time_t current_time = time(NULL);
    if (current_time - session.last_activity > SESSION_TIMEOUT) {
        return false;
    }
    
    return true;
}

bool updateSessionActivity(const char* session_token) {
    // This is a simplified implementation
    // In a real system, you'd update the sessions file
    return validateSession(session_token);
}

char* getUsernameFromSession(const char* session_token) {
    static char username[MAX_USERNAME_LENGTH];
    Session session;
    
    if (loadSession(session_token, &session) && session.is_active) {
        strcpy(username, session.username);
        return username;
    }
    
    return NULL;
}

bool logoutUser(const char* session_token) {
    // In a real implementation, you'd mark the session as inactive
    // For simplicity, we'll just validate it exists
    return validateSession(session_token);
}

void cleanupExpiredSessions() {
    // This would remove expired sessions from the database
    // Simplified implementation for now
    printf("Cleaning up expired sessions...\n");
} 