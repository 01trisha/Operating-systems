#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUF_SIZE 1024

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];
    int bytes_received;

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Create socket error");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Address error");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Server connection error");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server: %s:%d\n", SERVER_IP, PORT);

    while (1) {
        printf("Enter the message: ");
        fgets(buffer, BUF_SIZE, stdin);
        send(client_socket, buffer, strlen(buffer), 0);

        bytes_received = recv(client_socket, buffer, BUF_SIZE, 0);
        if (bytes_received <= 0) {
            fprintf(stderr, "Receive data error");
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        buffer[bytes_received] = '\0';
        printf("Server reply: %s", buffer);
    }

    close(client_socket);
    return 0;
}
