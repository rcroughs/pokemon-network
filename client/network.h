//===-- client/network.h - Network handling ----------------------------*- C -*-===//
//
// This file is ment to be used by the client to create a connection with the server
// though a socket (TCP) and to send images to the server
//
//===---------------------------------------------------------------------------===//

#include <stdbool.h>

/*
 * Create connection with the server
 * @param ipAddress: the ip address of the server
 * @return: the socket file descriptor
 */
int createConnection(char *ipAddress);

/*
 * Send an image on a socket from a filepath
 * @param socket_fd: the socket file descriptor
 * @param path_to_image: the path to the image to send
 * @return: true if the image was sent, false otherwise
 */
bool sendFile(int socket_fd, char *path_to_image);

/*
 * Kill the communication between the client and the server
 * @param socket_fd: the socket file descriptor
 * @return: void
 */
void killCommunication(int socket_fd);