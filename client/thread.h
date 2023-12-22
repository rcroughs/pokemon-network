//===-- client/thread.h - Thread handling ----------*- C -*-===//
//
// This file is ment to be used by the client to handle the threads (catching the responses and
// printing them in the right order)
//
//===-------------------------------------------------------===//

#include "stdbool.h"

/*
 * Parameters for the thread that catches the responses of the server
 * @member socket_fd: the socket file descriptor
 * @member imageSent: the number of images sent
 * @member imageCatched: the number of images catched
 * @member eof: true if the client reached an eof in the stdin, false by default
 * @member responses: the responses of the server
 */
struct threadCatcherArgs {
  int socket_fd;
  int *imageSent;
  int *imageCatched;
  bool *eof;
  char **responses;
};

/*
 * Parameters for the thread that prints the responses of the server
 * @member responses: the responses of the server
 * @member eof: true if the client reached an eof in the stdin, false by default
 * @member imagePrinted: the number of images printed
 * @member imageSent: the number of images sent
 */
struct threadPrinterArgs {
  char **responses;
  bool *eof;
  int *imagePrinted;
  int *imageSent;
};

/*
 * Catches the responses coming from the socket and stores them in the responses array
 * @param args: the parameters of the thread (struct threadCatcherArgs)
 * @return: NULL
 */
void *catchResponse(void *args);

/*
 * Prints the responses in the right order 
 * @param args: the parameters of the thread (struct threadPrinterArgs)
 * @return: NULL
 */
void *responsePrinter(void *args);