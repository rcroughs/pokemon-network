//===-- serveur/server.h - Single connection between the server and a client ----------*- C -*-===//
//
// This file is ment to be used by the server to handle a connection between a client and the server
// on a sigle thread
//
//===------------------------------------------------------------------------------------------===//

#include <netinet/in.h>
#include <sys/socket.h>

/*
 * Structure reprensenting the server parameters
 * @params fileDescriptor: file descriptor of the socket
 * @params address: server socket address
 */
struct serverParams {
  int fileDescriptor;
  struct sockaddr_in address;
};

/*
 * Sets up the server socket on the 5555 port through TCP
 * @return serverParams structure containing the file descriptor and the socket
 * address
 */
struct serverParams createServer();

/*
 * Handle connection between the server and one client
 * @params args: integer representing the file descriptor of the socket with the client
 * @return NULL
 */
void *handleConnection(void *args);