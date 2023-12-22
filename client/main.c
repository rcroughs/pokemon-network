#include "network.h"
#include "thread.h"

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

void sighandler(int signum) {
  if (signum == SIGINT) {
    printf("Program interruption ; Waiting to receive all the responses to shut down the program\n");
  } else if (signum == SIGPIPE) {
    printf("Server disconnected\n");
    exit(0);
  }
}

void trigger_signal(int *interrupted) {
    *interrupted = 1;
}

int main(int argc, char *argv[]) {

  // Définition de la struct d'action
  struct sigaction sa;
  sa.sa_handler = sighandler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  // Masquage des signaux
  if (sigaction(SIGINT, &sa, NULL) == -1) {
      perror("signal() ; Couldn't define a signal handler for SIGINT");
  }
  if (sigaction(SIGPIPE, &sa, NULL) == -1) {
      perror("signal() ; Couldn't define a signal handler for SIGPIPE");
  }

  int interrupted = 0; // Passe à 1 en cas de signal pendant fgets() --> évite l'appel bloquant

  char serverIP[15] = "127.0.0.1";
  if (argc == 2) {
    strncpy(serverIP, argv[1], 15);
  }
  int server_fd = createConnection(serverIP);
  int imageSent = 0;
  int imageCatched = 0;
  int imagePrinted = 0;
  bool eof = false;
  char *response[256];
  for (int i = 0; i < 256; i++) {
    response[i] = NULL;
  }
  struct threadCatcherArgs args = {server_fd, &imageSent, &imageCatched, &eof,
                                   response};
  char path[1000];
  pthread_t threadCatcher;
  pthread_create(&threadCatcher, NULL, catchResponse, (void *)(&args));
  struct threadPrinterArgs params = {response, &eof, &imagePrinted, &imageSent};
  pthread_t threadPrinter;
  pthread_create(&threadPrinter, NULL, responsePrinter, (void *)(&params));
  while (!interrupted &&  fgets(path, sizeof(path), stdin) != NULL &&
         imageSent < 256) {

    if (errno == EINTR) {
      trigger_signal(&interrupted);
    }

    int length = strlen(path);
    path[length - 1] = '\0';

    if (sendFile(server_fd, path)) {
      imageSent++;
      usleep(50000); // Sleep 50000 µseconds (50ms) before fetching the next image
    }
    memset(path, 0, sizeof(path));
  }
  eof = true;
  if (imageSent == 256) {
    printf("You reached the sending limit of 256 images. Restart the client to get more...");
  }
  if (imageCatched == imageSent) {
    pthread_cancel(threadCatcher);
  } else {
    pthread_join(threadCatcher, NULL);
  }
  pthread_join(threadPrinter, NULL);
  for (int i = 0; i < imagePrinted; i++) {
    free(response[i]);
  }
  killCommunication(server_fd);
  return 0;
}