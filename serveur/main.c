#include <stdio.h>
#include <inttypes.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include "../commun/commun.h"

#include "imgdist.h"

/*
 * Struture représentant notre serveur
 */
struct serverParams {
    int fileDescriptor;
    struct sockaddr_in address;
};

struct queryResults {
    char* filePath;
    unsigned int distance;
};

struct imgArgs {
    char* buffer;
    int bufferSize;
    int socket_fd;
};

void ExempleSignaux(void);

/*
 * Sets up the server socket on the 5555 port through TCP
 * @return serverParams structure containing the file descriptor and the socket address
 */
struct serverParams createServer() {
    struct serverParams server;
    server.fileDescriptor = socket(AF_INET, SOCK_STREAM, 0);

    server.address.sin_family = AF_INET;
    server.address.sin_addr.s_addr = INADDR_ANY;
    server.address.sin_port = htons(5555);

    bind(server.fileDescriptor, (struct sockaddr *) &server.address, sizeof(server.address));
    listen(server.fileDescriptor, 10);
    return server;
}

/*
 * Send the result of the image id
 * @params socket_fd: Socket between client and server
 * @params mostSimilarImage: most similar image to the one passed
 * @params distance: distance between the image passed and the most similar image
 */
void sendResults(int socket_fd, char mostSimilarImage[], int distance) {
    char response[999];
    sprintf(response, "Most similar image found: '%s' with a distance of %d", mostSimilarImage, distance);
    write(socket_fd, response, 999);
}

/*
 * Compare images (must be completed)
 */
struct queryResults compareImages(char buffer[], int bufsize) {
    uint64_t hash1, hash2;
    char path[] = "img/3.bmp";
    struct queryResults res;
    PHashRaw(buffer, bufsize, &hash2);
    PHash(path, &hash1);
    res.distance = DistancePHash(hash1, hash2);
    res.filePath = path;
    return res;
}


void* threadExec(void* params) {
    struct imgArgs* image = (struct imgArgs*) params;
    struct queryResults result = compareImages(image->buffer, image->bufferSize);
    sendResults(image->socket_fd, result.filePath, result.distance);
    pthread_exit(0);
}


/*
 * For testing purposes, you can save an image
 * @params buffer: Raw image
 * @params buffersize: size of the image
 * @params fileName: name of the file to save
 * @returns void
 */
void saveImage(char* buffer, int buffersize, char fileName[]) {
    FILE* file = fopen(fileName, "wb");
    fwrite(buffer, buffersize, 1, file);
    fclose(file);
}

/*
 * Read an image taken through a socket (max: 20Kb)
 * @params socket_fd: file descriptor of the socket used for the communication between the server and the client
 * @params buffer: buffer for the image
 * @returns imageSize, -1 if the file exceed 20Kb
 */
int readImage(int socket_fd, char buffer[]) {
    ssize_t imageSize = read(socket_fd, buffer, 20001);
    if (buffer[0] == '\0') {
        return -2;
    }
    if (imageSize >= 20000) {
        return -1;
    }
    return imageSize;
}

/*
 * Handle connection between the server and one client
 */
void handleConnection(int socket) {
    int communicationStatus = 1;
    while (communicationStatus) {
        char buffer[20001];
        int imageSize = readImage(socket, &buffer);
        if (imageSize == -1) {
            char err[] = "The image you passed exceed 20Kb";
            write(socket, err, strlen(err));
        } else if (imageSize == -2) {
            communicationStatus = 0;
        } else {
            pthread_t new_thread;
            struct imgArgs args;
            args.buffer = &buffer;
            args.bufferSize = imageSize;
            args.socket_fd = socket;
            pthread_create(&new_thread, NULL, threadExec, &args);
        }
    }
}


int main(int argc, char* argv[]) {
    struct serverParams server = createServer();
    size_t addrlen = sizeof(server.address);
    while (1) {
        int new_socket = accept(server.fileDescriptor, (struct sockaddr_in *) &server.address, &addrlen);
        handleConnection(new_socket);
        close(new_socket);
    }
    close(server.fileDescriptor);
    return 0;
}