#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024

int main(int argc, char **argv) {
    int socket_fd, port, client_length, msg_len;
    struct sockaddr_in server_addr, client_addr;
    char buf[BUFFER_SIZE];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }
    port = atoi(argv[1]);

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("Socket open error");
        return 1;
    }

    int optval = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int));

    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short) port);

    if (bind(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        close(socket_fd);
        perror("Bind error");
        return 1;
    }

    client_length = sizeof(client_addr);
    while (1) {
        msg_len = recvfrom(socket_fd, buf, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &client_length);
        if (msg_len < 0) {
            close(socket_fd);
            perror("Recvfrom error");
            return 1;
        }

        msg_len = sendto(socket_fd, buf, msg_len, 0, (struct sockaddr *) &client_addr, client_length);
        if (msg_len < 0) {
            close(socket_fd);
            perror("Sendto error");
            return 1;
        }
    }
    return 0;
}
