#include "../SocketUtil/socketutil.h"
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

struct AcceptedSocket
{
    int acceptedSocketFD;
    struct sockaddr_in address;
    int error;
    bool acceptedSuccessfully;
};

struct AcceptedSocket * acceptIncomingConnection(int serverSocketFD);

void receiveAndPrintIncomingData(int socketFD);

void startAcceptingIncomingConnections();

void acceptNewConnectionAndReceiveAndPrintData(int serverSocketFD);

void sendReceivedMessageToTheOtherClients(char *buffer, int socketFD);

void receiveAndPrintIncomingDataOnSeperateThread(struct AcceptedSocket *pSocket);

struct AcceptedSocket acceptedSocket[10];
int acceptedSocketsCount = 0;

void startAcceptingIncomingConnections(int serverSocketFD){
    
    while(true){
        struct AcceptedSocket *clientSocket = acceptIncomingConnection(serverSocketFD);
        acceptedSocket[acceptedSocketsCount++] = *clientSocket;

        receiveAndPrintIncomingDataOnSeperateThread(clientSocket);
    }

}

void* receiveAndPrintIncomingDataWrapper(void* arg) {
    int socketFD = (int)(intptr_t)arg;
    receiveAndPrintIncomingData(socketFD);
    return NULL;
}

void receiveAndPrintIncomingDataOnSeperateThread(struct AcceptedSocket *pSocket){
        pthread_t id;
        pthread_create(&id, NULL, receiveAndPrintIncomingDataWrapper, (void*)(intptr_t)pSocket->acceptedSocketFD);
        
    }

void receiveAndPrintIncomingData(int socketFD) {
    char buffer[1024];
    
    while(true){
        ssize_t amountWasReceived = recv(socketFD, buffer, 1024, 0);

        if(amountWasReceived > 0){
            buffer[amountWasReceived] = 0;
            printf("%s\n", buffer);

            sendReceivedMessageToTheOtherClients(buffer, socketFD);
        }
        if(amountWasReceived == 0){
            break;
        }
    }

    close(socketFD);
}

void sendReceivedMessageToTheOtherClients(char *buffer, int socketFD){
    for(int i = 0; i < acceptedSocketsCount; i++){
        if(acceptedSocket[i].acceptedSocketFD != socketFD){
            send(acceptedSocket[i].acceptedSocketFD, buffer, strlen(buffer), 0);
        }
    }
}

struct AcceptedSocket * acceptIncomingConnection(int serverSocketFD){
    struct sockaddr_in clientAddress;
    socklen_t clientAddressSize = sizeof(struct sockaddr_in);
    int clientSocketFD = accept(serverSocketFD, (struct sockaddr*)&clientAddress, &clientAddressSize);
        
    struct AcceptedSocket *acceptedSocket = malloc(sizeof(struct AcceptedSocket));
    acceptedSocket->acceptedSocketFD = clientSocketFD;
    acceptedSocket->address = clientAddress;
    acceptedSocket->acceptedSuccessfully = clientSocketFD >0;

    if(!acceptedSocket->acceptedSuccessfully){
        acceptedSocket->error = clientSocketFD;
    }

    return acceptedSocket;
}

int main(){

    int serverSocketFD = createTCPIpv4Socket();

    struct sockaddr_in *serverAddress = createIPv4Address("", 2000);

    int result = bind(serverSocketFD, (struct sockaddr*)serverAddress, sizeof(*serverAddress));

    if(result == 0){
        printf("socket was bound successfully\n");
    }

    int listenResult = listen(serverSocketFD, 10);

    if(listenResult == 0){
        printf("socket was listened successfully\n");
    }

    startAcceptingIncomingConnections(serverSocketFD);
    
    shutdown(serverSocketFD, SHUT_RDWR);

    return 0;   
}