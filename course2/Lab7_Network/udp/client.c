#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024

int main(int argc, char **argv) {
    int socket_fd, port, msg_len;
    struct sockaddr_in server_addr;
    struct hostent *server;
    char *hostname;
    char buffer[BUFFER_SIZE];

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <hostname> <port> <message>\n", argv[0]);
        return 1;
    }
    hostname = argv[1];
    port = atoi(argv[2]);
    char *message = argv[3];

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("Socket open error");
        return 1;
    }

    server = gethostbyname(hostname);
    if (server == NULL) {
        close(socket_fd);
        fprintf(stderr, "Error, this host %s isn't exist \n", hostname);
        return 1;
    }

    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr, (char *) &server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(port);

    msg_len = sendto(socket_fd, message, strlen(message), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (msg_len < 0) {
        close(socket_fd);
        perror("Sendto error");
        return 1;
    }

    socklen_t serverlen = sizeof(server_addr);
    msg_len = recvfrom(socket_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &server_addr, &serverlen);
    if (msg_len < 0) {
        close(socket_fd);
        perror("Recvfrom error");
        return 1;
    }
    buffer[msg_len] = '\0';
    printf("Server reply: %s\n", buffer);
    close(socket_fd);
    return 0;
}
