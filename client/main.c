#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>

int createConnection(char* ipAddress) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5555);
    inet_pton(AF_INET, ipAddress, &server_addr.sin_addr);
    connect(socket_fd, (struct sockaddr_in*) &server_addr, sizeof(server_addr));
    return socket_fd;
}

void sendFile(int socket_fd, char* path_to_image) {
    FILE* file = fopen(path_to_image, "rb");
    uint8_t buffer[30000];
    size_t bytes = fread(buffer, 1, 30000, file);
    write(socket_fd, buffer, bytes);
    fclose(file);
}

char* catchResponse(int socket_fd) {
    char buffer[999];
    read(socket_fd, buffer, 999);
    printf("%s", buffer);
    return buffer;
}

int main(int argc, char* argv[]) {
    char serverIP[15] = "127.0.0.1";
    int server_fd = createConnection(serverIP);
    sendFile(server_fd, "./img/1.bmp");
    catchResponse(server_fd);
    close(server_fd);
    return 0;
}