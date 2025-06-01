#define _GNU_SOURCE  // Add this for getline function
#include "../SocketUtil/socketutil.h"
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

// Function declarations
void startListeningAndPrintMessagesOnNewThread(int socketFD);
void* listenAndPrint(void* arg);
bool handleAuthentication(int socketFD);

int main(){
   
    int socketFD = createTCPIpv4Socket();
    
    /* 127.0.0.1 is localhost address */
    struct sockaddr_in *address = createIPv4Address("127.0.0.1", 2000);
    
    int result = connect(socketFD, (struct sockaddr*)address, sizeof(*address));
    
    if(result == 0){
        printf("Connection to server successful\n");
    } else {
        printf("Failed to connect to server\n");
        return 1;
    }

    // Handle authentication
    if (!handleAuthentication(socketFD)) {
        printf("Authentication failed. Disconnecting.\n");
        close(socketFD);
        return 1;
    }

    printf("Type your messages (type 'exit' to quit):\n");

    startListeningAndPrintMessagesOnNewThread(socketFD);

    char *line = NULL;
    size_t linesize = 0;

    while(true){
        ssize_t charCount = getline(&line, &linesize, stdin);
        if (charCount > 0) {
            line[charCount-1] = 0; // Remove newline
            
            if(strcmp(line, "exit") == 0){
                break;
            }
            
            ssize_t amountWasSent = send(socketFD, line, strlen(line), 0);
            if(amountWasSent < 0) {
                printf("Failed to send message\n");
                break;
            }
        }
    }

    close(socketFD);
    free(line);

    return 0;
}

bool handleAuthentication(int socketFD) {
    char buffer[1024];
    char input[256];
    
    // Receive authentication menu
    ssize_t received = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) return false;
    
    buffer[received] = '\0';
    printf("%s", buffer);
    
    // Get user choice
    if (fgets(input, sizeof(input), stdin) == NULL) return false;
    send(socketFD, input, strlen(input), 0);
    
    // Handle username prompt
    received = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) return false;
    
    buffer[received] = '\0';
    printf("%s", buffer);
    
    if (fgets(input, sizeof(input), stdin) == NULL) return false;
    send(socketFD, input, strlen(input), 0);
    
    // Handle password prompt
    received = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) return false;
    
    buffer[received] = '\0';
    printf("%s", buffer);
    
    // Hide password input (simple version)
    if (fgets(input, sizeof(input), stdin) == NULL) return false;
    send(socketFD, input, strlen(input), 0);
    
    // Receive authentication result
    received = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) return false;
    
    buffer[received] = '\0';
    printf("%s", buffer);
    
    // Check if authentication was successful
    if (strstr(buffer, "successful") != NULL) {
        return true;
    }
    
    return false;
}

void startListeningAndPrintMessagesOnNewThread(int socketFD){
    pthread_t id;
    pthread_create(&id, NULL, listenAndPrint, (void*)(intptr_t)socketFD);
}

void* listenAndPrint(void* arg){
    int socketFD = (int)(intptr_t)arg;
    char buffer[1024];
    
    while(true){
        ssize_t amountWasReceived = recv(socketFD, buffer, 1024, 0);

        if(amountWasReceived > 0){
            buffer[amountWasReceived] = 0;
            printf("%s", buffer);
            fflush(stdout);
        }
        if(amountWasReceived == 0){
            printf("Disconnected from server\n");
            break;
        }
    }

    return NULL;
}