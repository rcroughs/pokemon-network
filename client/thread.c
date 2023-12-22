#include "thread.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

void *catchResponse(void *args) {
  struct threadCatcherArgs *params = (struct threadCatcherArgs *)args;
  char response[999];
  int bufferLength;

  sigset_t set;
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGPIPE);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  while (!*params->eof || *params->imageCatched != *params->imageSent) {
    bufferLength = read(params->socket_fd, response, 999);
    if (bufferLength > 0) {
      if (response[0] == CHAR_MAX) {
        printf("%s\n", response + 1);
      } else {
        params->responses[(int)response[0]] =
            malloc(bufferLength * sizeof(char));
        strncpy(params->responses[(int)response[0]], response + 1,
                bufferLength);
      }
      *(params->imageCatched) += 1;
    }
  }
  pthread_sigmask(SIG_UNBLOCK, &set, NULL);
  return NULL;
}

void *responsePrinter(void *args) {
  struct threadPrinterArgs *params = (struct threadPrinterArgs *)args;
  while (*params->imagePrinted < 256 &&
         (*params->imagePrinted != *params->imageSent || !*params->eof)) {
    if (params->responses[*params->imagePrinted] != NULL) {
      printf("%s\n", params->responses[*params->imagePrinted]);
      *params->imagePrinted += 1;
    }
    usleep(100000);
  }
  return NULL;
}