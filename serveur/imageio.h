//===-- serveur/imageio.h - Images manipulation ----------*- C -*-===//
//
// This file is ment to be used by the server to read and save images
//
//===-------------------------------------------------------------===//


/*
 * For testing purposes, you can save an image
 * @params buffer: Raw image
 * @params buffersize: size of the image
 * @params fileName: name of the file to save
 * @returns void
 */
void saveImage(char *buffer, int buffersize, char fileName[]);


/*
 * Read an image taken through a socket (max: 20Kb)
 * @params socket_fd: file descriptor of the socket used for the communication
 * between the server and the client
 * @params buffer: buffer for the image
 * @returns imageSize, -1 if the file exceed 20Kb
 *                     -2 if to the client disconnected
 *                     >0 if the image was read (size of the image)
 */ 
int readImage(int socket_fd, char *buffer);