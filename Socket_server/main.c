#include "../SocketUtil/socketutil.h"
#include "../MessageDB/messagedb.h"
#include "../Auth/auth.h"
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

struct AuthenticatedSocket
{
    int acceptedSocketFD;
    struct sockaddr_in address;
    int error;
    bool acceptedSuccessfully;
    bool isAuthenticated;
    char username[MAX_USERNAME_LENGTH];
    char sessionToken[MAX_SESSION_TOKEN_LENGTH];
};

struct AuthenticatedSocket * acceptIncomingConnection(int serverSocketFD);

void receiveAndPrintIncomingData(int socketFD);

void startAcceptingIncomingConnections();

void acceptNewConnectionAndReceiveAndPrintData(int serverSocketFD);

void sendReceivedMessageToTheOtherClients(char *buffer, int socketFD);

void receiveAndPrintIncomingDataOnSeperateThread(struct AuthenticatedSocket *pSocket);

void sendChatHistoryToNewClient(int socketFD);

bool handleAuthentication(struct AuthenticatedSocket *clientSocket);

void sendAuthenticationPrompt(int socketFD);

struct AuthenticatedSocket authenticatedSockets[10];
int authenticatedSocketsCount = 0;

void startAcceptingIncomingConnections(int serverSocketFD){
    
    while(true){
        struct AuthenticatedSocket *clientSocket = acceptIncomingConnection(serverSocketFD);
        
        // Handle authentication before allowing chat access
        if (handleAuthentication(clientSocket)) {
            authenticatedSockets[authenticatedSocketsCount++] = *clientSocket;
            
            // Send welcome message and chat history
            char welcomeMsg[256];
            snprintf(welcomeMsg, sizeof(welcomeMsg), "Welcome %s! You are now connected to the chat.\n", clientSocket->username);
            send(clientSocket->acceptedSocketFD, welcomeMsg, strlen(welcomeMsg), 0);
            
            sendChatHistoryToNewClient(clientSocket->acceptedSocketFD);
        receiveAndPrintIncomingDataOnSeperateThread(clientSocket);
        } else {
            printf("Authentication failed for client\n");
            close(clientSocket->acceptedSocketFD);
            free(clientSocket);
        }
    }
}

bool handleAuthentication(struct AuthenticatedSocket *clientSocket) {
    char buffer[1024];
    char username[MAX_USERNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    char choice[10];
    
    // Send authentication menu
    char authMenu[] = "=== Secure Chat Authentication ===\n"
                     "1. Login\n"
                     "2. Register\n"
                     "Please choose (1 or 2): ";
    
    send(clientSocket->acceptedSocketFD, authMenu, strlen(authMenu), 0);
    
    // Receive choice
    ssize_t received = recv(clientSocket->acceptedSocketFD, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) return false;
    
    buffer[received] = '\0';
    sscanf(buffer, "%s", choice);
    
    if (strcmp(choice, "1") == 0) {
        // Login process
        char loginPrompt[] = "Username: ";
        send(clientSocket->acceptedSocketFD, loginPrompt, strlen(loginPrompt), 0);
        
        received = recv(clientSocket->acceptedSocketFD, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) return false;
        buffer[received] = '\0';
        sscanf(buffer, "%s", username);
        
        char passwordPrompt[] = "Password: ";
        send(clientSocket->acceptedSocketFD, passwordPrompt, strlen(passwordPrompt), 0);
        
        received = recv(clientSocket->acceptedSocketFD, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) return false;
        buffer[received] = '\0';
        sscanf(buffer, "%s", password);
        
        if (authenticateUser(username, password)) {
            char* sessionToken = createSession(username, clientSocket->acceptedSocketFD);
            if (sessionToken) {
                strcpy(clientSocket->username, username);
                strcpy(clientSocket->sessionToken, sessionToken);
                clientSocket->isAuthenticated = true;
                
                char successMsg[] = "Login successful!\n";
                send(clientSocket->acceptedSocketFD, successMsg, strlen(successMsg), 0);
                return true;
            }
        }
        
        char failMsg[] = "Login failed! Invalid credentials.\n";
        send(clientSocket->acceptedSocketFD, failMsg, strlen(failMsg), 0);
        return false;
        
    } else if (strcmp(choice, "2") == 0) {
        // Registration process
        char usernamePrompt[] = "Choose username: ";
        send(clientSocket->acceptedSocketFD, usernamePrompt, strlen(usernamePrompt), 0);
        
        received = recv(clientSocket->acceptedSocketFD, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) return false;
        buffer[received] = '\0';
        sscanf(buffer, "%s", username);
        
        char passwordPrompt[] = "Choose password: ";
        send(clientSocket->acceptedSocketFD, passwordPrompt, strlen(passwordPrompt), 0);
        
        received = recv(clientSocket->acceptedSocketFD, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) return false;
        buffer[received] = '\0';
        sscanf(buffer, "%s", password);
        
        if (registerUser(username, password)) {
            char* sessionToken = createSession(username, clientSocket->acceptedSocketFD);
            if (sessionToken) {
                strcpy(clientSocket->username, username);
                strcpy(clientSocket->sessionToken, sessionToken);
                clientSocket->isAuthenticated = true;
                
                char successMsg[] = "Registration successful! You are now logged in.\n";
                send(clientSocket->acceptedSocketFD, successMsg, strlen(successMsg), 0);
                return true;
            }
        }
        
        char failMsg[] = "Registration failed! Username might already exist.\n";
        send(clientSocket->acceptedSocketFD, failMsg, strlen(failMsg), 0);
        return false;
    }
    
    char invalidMsg[] = "Invalid choice!\n";
    send(clientSocket->acceptedSocketFD, invalidMsg, strlen(invalidMsg), 0);
    return false;
}

void sendChatHistoryToNewClient(int socketFD) {
    FILE *file = fopen(DB_FILENAME, "r");
    if (file == NULL) {
        return; // No history to send
    }
    
    char line[MAX_MESSAGE_LENGTH + MAX_TIMESTAMP_LENGTH];
    char historyHeader[] = "=== Chat History ===\n";
    char historyFooter[] = "=== End of History ===\n";
    
    send(socketFD, historyHeader, strlen(historyHeader), 0);
    
    while (fgets(line, sizeof(line), file) != NULL) {
        send(socketFD, line, strlen(line), 0);
    }
    
    send(socketFD, historyFooter, strlen(historyFooter), 0);
    fclose(file);
}

void* receiveAndPrintIncomingDataWrapper(void* arg) {
    int socketFD = (int)(intptr_t)arg;
    receiveAndPrintIncomingData(socketFD);
    return NULL;
}

void receiveAndPrintIncomingDataOnSeperateThread(struct AuthenticatedSocket *pSocket){
        pthread_t id;
        pthread_create(&id, NULL, receiveAndPrintIncomingDataWrapper, (void*)(intptr_t)pSocket->acceptedSocketFD);
    }

void receiveAndPrintIncomingData(int socketFD) {
    char buffer[1024];
    
    // Find the authenticated socket for this FD
    struct AuthenticatedSocket *authSocket = NULL;
    for (int i = 0; i < authenticatedSocketsCount; i++) {
        if (authenticatedSockets[i].acceptedSocketFD == socketFD) {
            authSocket = &authenticatedSockets[i];
            break;
        }
    }
    
    if (!authSocket || !authSocket->isAuthenticated) {
        close(socketFD);
        return;
    }
    
    while(true){
        ssize_t amountWasReceived = recv(socketFD, buffer, 1024, 0);

        if(amountWasReceived > 0){
            buffer[amountWasReceived] = 0;
            
            // Validate session before processing message
            if (!validateSession(authSocket->sessionToken)) {
                char sessionExpired[] = "Session expired. Please reconnect.\n";
                send(socketFD, sessionExpired, strlen(sessionExpired), 0);
                break;
            }
            
            // Update session activity
            updateSessionActivity(authSocket->sessionToken);
            
            // Format message with username
            char formattedMessage[1024 + MAX_USERNAME_LENGTH + 10];
            snprintf(formattedMessage, sizeof(formattedMessage), "%s: %s", authSocket->username, buffer);
            
            printf("%s\n", formattedMessage);

            // Save message to database
            saveMessage(formattedMessage);

            sendReceivedMessageToTheOtherClients(formattedMessage, socketFD);
        }
        if(amountWasReceived == 0){
            break;
        }
    }

    // Logout user when disconnecting
    logoutUser(authSocket->sessionToken);
    close(socketFD);
}

void sendReceivedMessageToTheOtherClients(char *buffer, int socketFD){
    for(int i = 0; i < authenticatedSocketsCount; i++){
        if(authenticatedSockets[i].acceptedSocketFD != socketFD && authenticatedSockets[i].isAuthenticated){
            // Gửi tin nhắn với newline để xuống dòng
            char messageWithNewline[1024 + MAX_USERNAME_LENGTH + 12];
            snprintf(messageWithNewline, sizeof(messageWithNewline), "%s\n", buffer);
            send(authenticatedSockets[i].acceptedSocketFD, messageWithNewline, strlen(messageWithNewline), 0);
        }
    }
}

struct AuthenticatedSocket * acceptIncomingConnection(int serverSocketFD){
    struct sockaddr_in clientAddress;
    socklen_t clientAddressSize = sizeof(struct sockaddr_in);
    int clientSocketFD = accept(serverSocketFD, (struct sockaddr*)&clientAddress, &clientAddressSize);
        
    struct AuthenticatedSocket *acceptedSocket = malloc(sizeof(struct AuthenticatedSocket));
    acceptedSocket->acceptedSocketFD = clientSocketFD;
    acceptedSocket->address = clientAddress;
    acceptedSocket->acceptedSuccessfully = clientSocketFD > 0;
    acceptedSocket->isAuthenticated = false;
    memset(acceptedSocket->username, 0, MAX_USERNAME_LENGTH);
    memset(acceptedSocket->sessionToken, 0, MAX_SESSION_TOKEN_LENGTH);

    if(!acceptedSocket->acceptedSuccessfully){
        acceptedSocket->error = clientSocketFD;
    }

    return acceptedSocket;
}

int main(){

    // Initialize authentication system
    if (!initializeAuth()) {
        printf("Failed to initialize authentication system. Exiting.\n");
        return 1;
    }

    // Initialize database
    if (!initializeDatabase()) {
        printf("Failed to initialize database. Exiting.\n");
        return 1;
    }

    // Load and display chat history
    printf("Secure Chat Server starting...\n");
    loadChatHistory();

    int serverSocketFD = createTCPIpv4Socket();

    struct sockaddr_in *serverAddress = createIPv4Address("", 2000);

    int result = bind(serverSocketFD, (struct sockaddr*)serverAddress, sizeof(*serverAddress));

    if(result == 0){
        printf("socket was bound successfully\n");
    }

    int listenResult = listen(serverSocketFD, 10);

    if(listenResult == 0){
        printf("socket was listened successfully\n");
        printf("Server is ready to accept authenticated connections...\n");
    }

    startAcceptingIncomingConnections(serverSocketFD);
    
    shutdown(serverSocketFD, SHUT_RDWR);

    return 0;   
}