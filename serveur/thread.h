//===-- serveur/thread.h - Concurrent image search ----------*- C -*-===//
//
// This file is ment to be used by the server to compare images concurrently
// using threads
//
//===----------------------------------------------------------------===//

#include <stdint.h>

/*
* Structure containing the result of the closest hash found by a thread.
* @member filePath: path of the closest image
* @member distance: distance between the image passed and the closest image
*/
struct queryResults {
  char *filePath;
  int distance;
};

/*
* Structure containing the parameters passed to a thread.
* @member fileList: list of files to compare
* @member baseHash: hash of the image to compare
*/
struct CompareThreadParams {
    struct FileList* fileList;
    uint64_t baseHash;
};

/*
* Structure representing an image to compare
* @member buffer: buffer of the image
* @member bufferSize: size of the buffer
* @member socket_fd: socket between the client and the server
* @member img_id: id of the image (used to send the result)
*/
struct imgArgs {
  char *buffer;
  int bufferSize;
  int socket_fd;
  char img_id;
};

/*
 * Send the result of the image id
 * @params socket_fd: Socket between client and server
 * @params mostSimilarImage: most similar image to the one passed
 * @params distance: distance between the image passed and the most similar
 * image
 */
void sendResults(int socket_fd, char mostSimilarImage[], int distance, char img_id);

/*
 * Compare images passed on a thread (runned concurrently).
 * @params params: parameters passed to the thread
 * @return void
 */
void *threadCompareImages(void *params);

/*
 * Compare images passed to a directory to find the best matching image.
 * @params buffer: Raw image (char type array)
 * @params buffersize: size of the image (int)
 * @return res: queryResults type datastruct containing the best distance found
 * (with it's linked image).
 */
struct queryResults findBestMatchingImage(char buffer[], int bufsize);

/*
 * Launch the query to find the best matching image on a new thread.
 * @params args: imgArgs type datastruct containing the buffer, the size of the
 * buffer, the socket between the client and the server and the id of the image
 */
void* launchQuery(void *args);