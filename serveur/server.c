#include "server.h"
#include "imageio.h"
#include "thread.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct serverParams createServer() {
  struct serverParams server;
  server.fileDescriptor = socket(AF_INET, SOCK_STREAM, 0);

  server.address.sin_family = AF_INET;
  server.address.sin_addr.s_addr = INADDR_ANY;
  server.address.sin_port = htons(5555);

  if (bind(server.fileDescriptor, (struct sockaddr *)&server.address,
           sizeof(server.address)) < 0) {
    perror("Bind failed");
    close(server.fileDescriptor);
    exit(1);
  }
  listen(server.fileDescriptor, 10);
  return server;
}

void *handleConnection(void *args) {
  int socket = *(int *)args;
  int communicationStatus = 1;
  char img_id = 0;
  while (communicationStatus) {
    char buffer[20001];
    int imageSize = readImage(socket, buffer);
    if (imageSize == -1) {
      char err[] = "The image you passed exceed 20Kb";
      write(socket, err, strlen(err));
    } else if (imageSize == -2) {
      communicationStatus = 0;
    } else {
      pthread_t new_thread;
      struct imgArgs args;
      args.buffer = buffer;
      args.bufferSize = imageSize;
      args.socket_fd = socket;
      args.img_id = img_id;
      pthread_create(&new_thread, NULL, launchQuery, &args);
      img_id++;
    }
  }
  close(socket);
  return NULL;
}