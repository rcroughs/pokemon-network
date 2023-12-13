#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h> // added by me
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include "../commun/commun.h"

#include <pthread.h>


void handler_singint(int signum) {
    if (signum == SIGINT) {
        pthread_exit(0);
    }
}


int createConnection(char* ipAddress) {
    //struct connexion con;
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5555);

    inet_pton(AF_INET, ipAddress, &server_addr.sin_addr);

    if (connect(socket_fd, (struct sockaddr_in*) &server_addr, sizeof(server_addr)) == -1) {
        perror("Couldn't connect to the server");
        close(socket_fd);
        exit(1);
    }
    return socket_fd;
}

void sendFile(int socket_fd, char* path_to_image) {
    if (access(path_to_image, F_OK) == 0) {
        //printf("Path : %s\n\n", path_to_image);
        FILE *file = fopen(path_to_image, "rb");
        char buffer[20001];
        size_t bytes = fread(buffer, 1, 20001, file);
        write(socket_fd, buffer, bytes);
        fclose(file);
    } else {
        perror("This file does not exist");
    }
}

void killCommunication(int socket_fd) {
    write(socket_fd, "\0", 1);
}

void* catchResponse(void* socket_fd) {

    char response[999];
    int server_fd = *(int*)socket_fd;
    while(read(server_fd, response, 999) > 0) {
        printf("%s\n", response);

    }
}

int main(int argc, char* argv[]) {

    char serverIP[15] = "127.0.0.1";
    if (argc == 2) {
        strncpy(serverIP, argv[1], 15);
    }
    int server_fd = createConnection(serverIP);
    char path[1000];
    pthread_t thread;
    pthread_create(&thread, NULL, catchResponse, (void*)(&server_fd));
    while (fgets(path, sizeof(path), stdin) > 0) {

        int length = strlen(path);
        path[length-1] = '\0';

        sendFile(server_fd, path);
        memset(path, 0, sizeof(path));

    }
    killCommunication(server_fd);
    close(server_fd);
    return 0;
}