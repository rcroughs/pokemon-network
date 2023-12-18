#include <arpa/inet.h>
#include <limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void handler_singint(int signum) {
  if (signum == SIGINT) {
    pthread_exit(0);
  }
}

struct threadCatcherArgs {
  int socket_fd;
  int *imageSent;
  int *imageCatched;
  bool *eof;
  char **responses;
};

struct threadPrinterArgs {
  char **responses;
  bool *eof;
  int *imagePrinted;
  int *imageSent;
};

/*
 * Create connection with the server
 */
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

/*
 * Send an image on a socket from a filepath
 */
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

/*
 * Catches the responses coming from the socket
 */
void *catchResponse(void *args) {
  struct threadCatcherArgs *params = (struct threadCatcherArgs *)args;
  char response[999];
  int bufferLength;
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
  return NULL;
}

/*
 * Prints the responses in the right order
 */
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

int main(int argc, char *argv[]) {
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
  struct threadCatcherArgs args = {server_fd, &imageSent, &imageCatched, &eof,
                                   response};
  char path[1000];
  pthread_t threadCatcher;
  pthread_create(&threadCatcher, NULL, catchResponse, (void *)(&args));
  struct threadPrinterArgs params = {response, &eof, &imagePrinted, &imageSent};
  pthread_t threadPrinter;
  pthread_create(&threadPrinter, NULL, responsePrinter, (void *)(&params));
  while (fgets(path, sizeof(path), stdin) != NULL && imageSent < 256) {

    int length = strlen(path);
    path[length - 1] = '\0';

    if (sendFile(server_fd, path)) {
      imageSent++;
      usleep(50000); // Sleep 500000 µseconds (500ms) before fetching the next image
    }
    memset(path, 0, sizeof(path));
  }
  eof = true;
  if (imageSent == 256) {
    printf("You reached the sending limit of 256 images. Restart the client to "
           "get more...");
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