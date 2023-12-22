#include "server.h"
#include <pthread.h>
#include <unistd.h>

int main() {
  struct serverParams server = createServer();
  size_t addrlen = sizeof(server.address);
  while (1) {
    int new_socket =
        accept(server.fileDescriptor, (struct sockaddr *)&server.address,
               (socklen_t *)&addrlen);
    pthread_t new_thread;
    pthread_create(&new_thread, NULL, handleConnection, (void *)&new_socket);
  }
  close(server.fileDescriptor);
  return 0;
}
