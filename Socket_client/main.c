#define _GNU_SOURCE
#include "../SocketUtil/socketutil.h"
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 4443

void init_openssl() {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
    EVP_cleanup();
}

SSL_CTX *create_context() {
    const SSL_METHOD *method = TLS_client_method();  
    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

void* listenAndPrint(void* arg) {
    SSL *ssl = (SSL*)arg;
    char buffer[1024];

    while (true) {
        ssize_t received = SSL_read(ssl, buffer, sizeof(buffer) - 1);
        if (received <= 0) {
            printf("Disconnected from server.\n");
            break;
        }
        buffer[received] = '\0';
        printf("%s\n", buffer);
        fflush(stdout);
    }

    return NULL;
}

bool handleAuthentication(SSL *ssl) {
    char buffer[1024];
    char input[256];

    // Menu
    ssize_t received = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    if (received <= 0) return false;
    buffer[received] = '\0';
    printf("%s", buffer);

    // Nhập lựa chọn
    if (fgets(input, sizeof(input), stdin) == NULL) return false;
    SSL_write(ssl, input, strlen(input));

    // Username
    received = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    if (received <= 0) return false;
    buffer[received] = '\0';
    printf("%s", buffer);

    if (fgets(input, sizeof(input), stdin) == NULL) return false;
    SSL_write(ssl, input, strlen(input));

    // Password
    received = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    if (received <= 0) return false;
    buffer[received] = '\0';
    printf("%s", buffer);

    if (fgets(input, sizeof(input), stdin) == NULL) return false;
    SSL_write(ssl, input, strlen(input));

    // Kết quả
    received = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    if (received <= 0) return false;
    buffer[received] = '\0';
    printf("%s", buffer);

    return strstr(buffer, "successful") != NULL;
}

int main() {
    init_openssl();
    SSL_CTX *ctx = create_context();

    int sockfd = createTCPIpv4Socket();
    struct sockaddr_in *addr = createIPv4Address("127.0.0.1", PORT);
    if (connect(sockfd, (struct sockaddr*)addr, sizeof(*addr)) != 0) {
        perror("connect");
        return 1;
    }

    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);
    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    printf("Connected to server with SSL.\n");

    if (!handleAuthentication(ssl)) {
        printf("Auth failed. Closing.\n");
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(sockfd);
        SSL_CTX_free(ctx);
        return 1;
    }

    printf("Type message (type 'exit' to quit):\n");

    pthread_t listener;
    pthread_create(&listener, NULL, listenAndPrint, ssl);
    pthread_detach(listener);

    char *line = NULL;
    size_t linesize = 0;
    while (true) {
        ssize_t count = getline(&line, &linesize, stdin);
        if (count <= 0) break;

        line[count - 1] = '\0';  // remove newline
        if (strcmp(line, "exit") == 0) break;

        if (SSL_write(ssl, line, strlen(line)) <= 0) {
            perror("send");
            break;
        }
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sockfd);
    SSL_CTX_free(ctx);
    free(line);
    return 0;
}
