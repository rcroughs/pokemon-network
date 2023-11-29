/**
 * Un main avec quelques exemples pour vous aider. Vous pouvez
 * bien entendu modifier ce fichier comme bon vous semble.
 **/

#include <stdio.h>
#include <inttypes.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include "../commun/commun.h"

#include "imgdist.h"

struct serverParams {
    int fileDescriptor;
    struct sockaddr_in address;
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

void sendResponse(int socket_fd, char* message) {
    write(socket_fd, message, strlen(message));
}

/*
 * Take a socket file descriptor, wait for an image, and save it in the newPathFile
 * @params int socked_fd: file descriptor of the socket used for the communication between the server and the client
 */
int readImage(int socket_fd, char* buffer) {
    ssize_t imageSize = read(socket_fd, buffer, 20000);
    FILE* file = fopen("test.bmp", "wb");
    fwrite(buffer, imageSize, 1, file);
    fclose(file);
    if (imageSize >= 20000) {
        return -1;
    }
    return imageSize;
}


int main(int argc, char* argv[]) {
    struct serverParams server = createServer();
    while(1) {
        size_t addrlen = sizeof(server.address);
        int new_socket = accept(server.fileDescriptor, (struct sockaddr_in *) &server.address, &addrlen);
        char* buffer[20000];
        if (readImage(new_socket, buffer) < 0) {
            sendResponse(new_socket, "Your image exceed 20Kb");
        } else {
            sendResponse(new_socket, "Image bien recue");
        }
        close(new_socket);
    }
    close(server.fileDescriptor);

    ExempleSignaux(); // Le reste du code est issu de squelette
    return 0;
}

static volatile sig_atomic_t signalRecu = 0;
void SignalHandler(int sig) {
   signalRecu = 1;
}

void ExempleSignaux(void) {
   /// Exemple gestion de signaux (cf Annexe de l'énoncé & corrigé du projet 1) ///
   
   // Forcer l'interruption des appels systèmes lors de la réception de SIGINT
   struct sigaction action;
   action.sa_handler = SignalHandler;
   sigemptyset(&action.sa_mask);

   if (sigaction(SIGINT, &action, NULL) < 0) {
      perror("sigaction()");
      return;
   }
   
   
   // Gestion idéale (court et sans risque d'accès concurrents) d'un signal
   // (cf SignalHandler() également).
   printf("Signal recu : %d.\n", signalRecu);
   raise(SIGINT);
   printf("Signal recu : %d.\n", signalRecu);
   
   
   // Bloquer des signaux pour le thread courant
   sigset_t set;
    
   sigemptyset(&set);        // Ensemble vide de signaux
   sigaddset(&set, SIGINT);  // Ajouter le signal SIGINT
   sigaddset(&set, SIGUSR1); // Ajouter le signal SIGUSR1
    
   if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
      perror("pthread_sigmask()");
      return;
   }
   
   /// ///
}