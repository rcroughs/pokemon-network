#include "network.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int createConnection(char *ipAddress) {
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(5555);

  inet_pton(AF_INET, ipAddress, &server_addr.sin_addr);

  if (connect(socket_fd, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) == -1) {
    perror("Couldn't connect to the server");
    close(socket_fd);
    exit(1);
  }
  return socket_fd;
}

bool sendFile(int socket_fd, char *path_to_image) {
  if (access(path_to_image, F_OK) == 0) {
    FILE *file = fopen(path_to_image, "rb");
    char buffer[20001];
    size_t bytes = fread(buffer, 1, 20001, file);
    write(socket_fd, buffer, bytes);
    fclose(file);
    return true;
  } else {
    printf("No similar image found (no comparison could be performed "
           "successfully).");
    return false;
  }
}

/*
 * Kill the communication between the client and the server
 */
void killCommunication(int socket_fd) {
  write(socket_fd, "\0", 1);
  close(socket_fd);
}