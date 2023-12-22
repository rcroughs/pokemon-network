#include "imageio.h"
#include <stdio.h>
#include <unistd.h>

void saveImage(char *buffer, int buffersize, char fileName[]) {
  FILE *file = fopen(fileName, "wb");
  fwrite(buffer, buffersize, 1, file);
  fclose(file);
}

int readImage(int socket_fd, char *buffer) {
  ssize_t imageSize = read(socket_fd, buffer, 20001);
  if (buffer[0] == '\0') {
    return -2;
  }
  if (imageSize >= 20000) {
    return -1;
  }
  return imageSize;
}
