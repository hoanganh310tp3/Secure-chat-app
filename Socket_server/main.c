#include "../SocketUtil/socketutil.h"
#include "../MessageDB/messagedb.h"
#include "../Auth/auth.h"
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 4443
#define MAX_CLIENTS 10

SSL_CTX *context = NULL;

void init_openssl() {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
    EVP_cleanup();
}

SSL_CTX *create_context() {
    const SSL_METHOD *method = TLS_server_method();
    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

void configure_context(SSL_CTX *ctx) {
    if (SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    if (!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr, "Private key does not match the public certificate\n");
        exit(EXIT_FAILURE);
    }
}

struct AuthenticatedSocket {
    int acceptedSocketFD;
    struct sockaddr_in address;
    bool acceptedSuccessfully;
    bool isAuthenticated;
    char username[MAX_USERNAME_LENGTH];
    char sessionToken[MAX_SESSION_TOKEN_LENGTH];
    SSL *ssl;
};

struct AuthenticatedSocket authenticatedSockets[MAX_CLIENTS];
int authenticatedSocketsCount = 0;

// Forward declarations
struct AuthenticatedSocket *acceptIncomingConnection(int serverSocketFD, SSL_CTX *context);
void receiveAndPrintIncomingData(struct AuthenticatedSocket *authSocket);
void receiveAndPrintIncomingDataOnSeparateThread(struct AuthenticatedSocket *clientSocket);
void sendChatHistoryToNewClient(struct AuthenticatedSocket *clientSocket);
bool handleAuthentication(struct AuthenticatedSocket *clientSocket);
void sendReceivedMessageToTheOtherClients(char *buffer, struct AuthenticatedSocket *sender);

void startAcceptingIncomingConnections(int serverSocketFD) {
    while (true) {
        struct AuthenticatedSocket *clientSocket = acceptIncomingConnection(serverSocketFD, context);
        if (clientSocket == NULL) continue;

        if (handleAuthentication(clientSocket)) {
            if (authenticatedSocketsCount < MAX_CLIENTS) {
                authenticatedSockets[authenticatedSocketsCount++] = *clientSocket;
            } else {
                char msg[] = "Server full. Connection rejected.\n";
                SSL_write(clientSocket->ssl, msg, strlen(msg));
                close(clientSocket->acceptedSocketFD);
                SSL_free(clientSocket->ssl);
                free(clientSocket);
                continue;
            }

            char welcomeMsg[256];
            snprintf(welcomeMsg, sizeof(welcomeMsg), "Welcome %s! You are now connected to the chat.\n", clientSocket->username);
            SSL_write(clientSocket->ssl, welcomeMsg, strlen(welcomeMsg));

            sendChatHistoryToNewClient(clientSocket);
            receiveAndPrintIncomingDataOnSeparateThread(clientSocket);
        } else {
            printf("Authentication failed for client\n");
            close(clientSocket->acceptedSocketFD);
            SSL_free(clientSocket->ssl);
            free(clientSocket);
        }
    }
}

bool handleAuthentication(struct AuthenticatedSocket *clientSocket) {
    char buffer[1024];
    char username[MAX_USERNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    char choice[10];

    char authMenu[] = "=== Secure Chat Authentication ===\n1. Login\n2. Register\nPlease choose (1 or 2): ";
    SSL_write(clientSocket->ssl, authMenu, strlen(authMenu));

    ssize_t received = SSL_read(clientSocket->ssl, buffer, sizeof(buffer) - 1);
    if (received <= 0) return false;

    buffer[received] = '\0';
    sscanf(buffer, "%s", choice);

    if (strcmp(choice, "1") == 0) {
        SSL_write(clientSocket->ssl, "Username: ", 10);
        received = SSL_read(clientSocket->ssl, buffer, sizeof(buffer) - 1);
        if (received <= 0) return false;
        buffer[received] = '\0';
        sscanf(buffer, "%s", username);

        SSL_write(clientSocket->ssl, "Password: ", 10);
        received = SSL_read(clientSocket->ssl, buffer, sizeof(buffer) - 1);
        if (received <= 0) return false;
        buffer[received] = '\0';
        sscanf(buffer, "%s", password);

        if (authenticateUser(username, password)) {
            char *sessionToken = createSession(username, clientSocket->acceptedSocketFD);
            if (sessionToken) {
                strcpy(clientSocket->username, username);
                strcpy(clientSocket->sessionToken, sessionToken);
                clientSocket->isAuthenticated = true;
                SSL_write(clientSocket->ssl, "Login successful!\n", 18);
                return true;
            }
        }
        SSL_write(clientSocket->ssl, "Login failed! Invalid credentials.\n", 35);
        return false;

    } else if (strcmp(choice, "2") == 0) {
        SSL_write(clientSocket->ssl, "Choose username: ", 17);
        received = SSL_read(clientSocket->ssl, buffer, sizeof(buffer) - 1);
        if (received <= 0) return false;
        buffer[received] = '\0';
        sscanf(buffer, "%s", username);

        SSL_write(clientSocket->ssl, "Choose password: ", 17);
        received = SSL_read(clientSocket->ssl, buffer, sizeof(buffer) - 1);
        if (received <= 0) return false;
        buffer[received] = '\0';
        sscanf(buffer, "%s", password);

        if (registerUser(username, password)) {
            char *sessionToken = createSession(username, clientSocket->acceptedSocketFD);
            if (sessionToken) {
                strcpy(clientSocket->username, username);
                strcpy(clientSocket->sessionToken, sessionToken);
                clientSocket->isAuthenticated = true;
                SSL_write(clientSocket->ssl, "Registration successful! You are now logged in.\n", 48);
                return true;
            }
        }
        SSL_write(clientSocket->ssl, "Registration failed! Username might already exist.\n", 52);
        return false;
    }

    SSL_write(clientSocket->ssl, "Invalid choice!\n", 16);
    return false;
}

void sendChatHistoryToNewClient(struct AuthenticatedSocket *clientSocket) {
    FILE *file = fopen(DB_FILENAME, "r");
    if (!file) return;

    SSL_write(clientSocket->ssl, "=== Chat History ===\n", 22);
    char line[MAX_MESSAGE_LENGTH + MAX_TIMESTAMP_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        SSL_write(clientSocket->ssl, line, strlen(line));
    }
    SSL_write(clientSocket->ssl, "=== End of History ===\n", 24);
    fclose(file);
}

void *receiveAndPrintIncomingDataWrapper(void *arg) {
    struct AuthenticatedSocket *authSocket = (struct AuthenticatedSocket *)arg;
    receiveAndPrintIncomingData(authSocket);
    return NULL;
}

void receiveAndPrintIncomingDataOnSeparateThread(struct AuthenticatedSocket *pSocket) {
    pthread_t id;
    pthread_create(&id, NULL, receiveAndPrintIncomingDataWrapper, (void *)pSocket);
    pthread_detach(id);
}

void receiveAndPrintIncomingData(struct AuthenticatedSocket *authSocket) {
    char buffer[1024];
    while (true) {
        ssize_t received = SSL_read(authSocket->ssl, buffer, sizeof(buffer) - 1);
        if (received <= 0) break;

        buffer[received] = '\0';

        if (!validateSession(authSocket->sessionToken)) {
            SSL_write(authSocket->ssl, "Session expired. Please reconnect.\n", 35);
            break;
        }

        updateSessionActivity(authSocket->sessionToken);

        char formattedMessage[1024 + MAX_USERNAME_LENGTH + 5];
        snprintf(formattedMessage, sizeof(formattedMessage), "%s: %s", authSocket->username, buffer);

        printf("%s\n", formattedMessage);
        saveMessage(formattedMessage);
        sendReceivedMessageToTheOtherClients(formattedMessage, authSocket);
    }

    logoutUser(authSocket->sessionToken);
    SSL_shutdown(authSocket->ssl);
    close(authSocket->acceptedSocketFD);
    SSL_free(authSocket->ssl);
}

void sendReceivedMessageToTheOtherClients(char *buffer, struct AuthenticatedSocket *sender) {
    for (int i = 0; i < authenticatedSocketsCount; ++i) {
        struct AuthenticatedSocket *target = &authenticatedSockets[i];
        if (target->isAuthenticated && target->acceptedSocketFD != sender->acceptedSocketFD) {
            char messageWithNewline[1024 + MAX_USERNAME_LENGTH + 10];
            snprintf(messageWithNewline, sizeof(messageWithNewline), "%s\n", buffer);
            SSL_write(target->ssl, messageWithNewline, strlen(messageWithNewline));
        }
    }
}

struct AuthenticatedSocket *acceptIncomingConnection(int serverSocketFD, SSL_CTX *context) {
    struct sockaddr_in clientAddress;
    socklen_t clientAddressSize = sizeof(clientAddress);
    int clientSocketFD = accept(serverSocketFD, (struct sockaddr *)&clientAddress, &clientAddressSize);
    if (clientSocketFD < 0) return NULL;

    SSL *ssl = SSL_new(context);
    SSL_set_fd(ssl, clientSocketFD);
    if (SSL_accept(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        close(clientSocketFD);
        SSL_free(ssl);
        return NULL;
    }

    struct AuthenticatedSocket *acceptedSocket = malloc(sizeof(struct AuthenticatedSocket));
    acceptedSocket->acceptedSocketFD = clientSocketFD;
    acceptedSocket->address = clientAddress;
    acceptedSocket->acceptedSuccessfully = true;
    acceptedSocket->isAuthenticated = false;
    acceptedSocket->ssl = ssl;
    memset(acceptedSocket->username, 0, MAX_USERNAME_LENGTH);
    memset(acceptedSocket->sessionToken, 0, MAX_SESSION_TOKEN_LENGTH);
    return acceptedSocket;
}

int main() {
    init_openssl();
    context = create_context();
    configure_context(context);

    if (!initializeAuth()) {
        printf("Failed to initialize authentication system. Exiting.\n");
        return 1;
    }
    if (!initializeDatabase()) {
        printf("Failed to initialize database. Exiting.\n");
        return 1;
    }

    printf("Secure Chat Server starting on port %d...\n", PORT);
    loadChatHistory();

    int serverSocketFD = createTCPIpv4Socket();
    struct sockaddr_in *serverAddress = createIPv4Address("", PORT);
    if (bind(serverSocketFD, (struct sockaddr *)serverAddress, sizeof(*serverAddress)) != 0) {
        perror("Bind failed");
        return 1;
    }

    if (listen(serverSocketFD, MAX_CLIENTS) != 0) {
        perror("Listen failed");
        return 1;
    }

    printf("Server is ready to accept authenticated connections...\n");
    startAcceptingIncomingConnections(serverSocketFD);

    shutdown(serverSocketFD, SHUT_RDWR);
    cleanup_openssl();
    return 0;
}
