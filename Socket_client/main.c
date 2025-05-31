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

int main(){
   
    int socketFD = createTCPIpv4Socket();
    
    /* 127.0.0.1 is localhost address */
    struct sockaddr_in *address = createIPv4Address("127.0.0.1", 2000);
    
    int result = connect(socketFD, (struct sockaddr*)address, sizeof(*address));
    
    if(result == 0){
        printf("connection is successful\n");
    }

    char *name = NULL;
    size_t namesize = 0;
    printf("please enter your name?\n");
    ssize_t nameCount = getline(&name, &namesize, stdin);
    name[nameCount-1]=0;

    char *line = NULL;
    size_t linesize = 0;
    printf("type and we will send it to server\n");

    startListeningAndPrintMessagesOnNewThread(socketFD);

    char buffer[1024];

    while(true){

        ssize_t charCount = getline(&line, &linesize, stdin);
        line[charCount-1]=0;
        sprintf(buffer,"%s: %s", name, line);

        if(charCount > 0){
            if(strcmp(line, "exit") == 0){
                break;
            }
            ssize_t amountWasSent = send(socketFD, buffer, strlen(buffer), 0);
            if(amountWasSent < 0) {
                printf("Failed to send message\n");
            }
        }
    }

    close(socketFD);

    return 0;
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
            printf("%s\n", buffer);
        }
        if(amountWasReceived == 0){
            break;
        }
    }

    close(socketFD);
    return NULL;
}