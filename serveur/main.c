#include <stdio.h>
#include <inttypes.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include "../commun/commun.h"
#include <limits.h>
#include <stdlib.h>

#include "imgdist.h"

/*
 * Struture représentant notre serveur
 */
struct serverParams {
    int fileDescriptor;
    struct sockaddr_in address;
};

struct queryResults {
    char* filePath;
    int distance;
};

struct imgArgs {
    char* buffer;
    int bufferSize;
    int socket_fd;
};

/*
* Structure représentant la liste des images dans img (+ le nombre)
*/
struct FileList {
    char** fileNames;
    int count;
};

/*
* Stucture représentant les paramètres passés dans la fonction a exécuter avec le thread.
*/
struct CompareThreadParams {
    struct FileList* fileList;
    uint64_t baseHash;
};


/*
* List files in given directory. [!] Directory should only be accessible server-side.
* @params directory: Directory which will be searched.
* @return fileList structure containing the number of files and their paths in an array.
*/
struct FileList listFiles(const char* directory) {
    struct FileList fileList;
    fileList.fileNames = NULL;
    fileList.count = 0;

    DIR* dir = opendir(directory);
    if (dir == NULL) {
        perror("opendir() error.");
        return fileList;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            fileList.count++;
            fileList.fileNames = realloc(fileList.fileNames, fileList.count * sizeof(char*));

            // On doit construire le chemin complet pour pouvoir utiliser directement les path dans la fonction de hash ;
            char* fullPath = malloc(strlen(directory) + 1 + strlen(entry->d_name) + 1);
            strcpy(fullPath, directory);
            strcat(fullPath, "/");
            strcat(fullPath, entry->d_name);

            fileList.fileNames[fileList.count - 1] = fullPath;
        }
    }

    closedir(dir);
    return fileList;
}

/*
* Free the memory used by given list.
* @params fileList : structure containing the number of files and their paths in an array. 
*/
void freeFileList(struct FileList* fileList) {
    for (int i = 0; i < fileList->count; i++) {
        free(fileList->fileNames[i]);
    }
    free(fileList->fileNames);
}

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

/*
 * Send the result of the image id
 * @params socket_fd: Socket between client and server
 * @params mostSimilarImage: most similar image to the one passed
 * @params distance: distance between the image passed and the most similar image
 */
void sendResults(int socket_fd, char mostSimilarImage[], int distance) {
    char response[999];
    // printf("\nDistance = %d\n", distance);
    if (distance == -1) {
        sprintf(response, "No similar image found (no comparison could be performed successfully).");
    } else {
        sprintf(response, "Most similar image found: '%s' with a distance of %d", mostSimilarImage, distance);
    }
    write(socket_fd, response, 999);
}

void* threadCompareImages(void* params) {

    struct CompareThreadParams* threadParams = (struct CompareThreadParams*) params;

    struct FileList* fileList = threadParams->fileList;
    uint64_t baseImage_hash = threadParams->baseHash;

    // Initialisation de la structure contenant les résultats.
    struct queryResults* result = malloc(sizeof(struct queryResults));
    result->distance = UINT_MAX;
    result->filePath = NULL;

    for (int i = 0; i < fileList->count; i++) {

        // Traitement pour chacune des images ;

        uint64_t currentHash;
        PHash(fileList->fileNames[i], &currentHash);
        int currentDist = DistancePHash(currentHash, baseImage_hash);

        // On compare la distance courrante à la meilleure jusque là
        // puis on update les données si une meilleure est trouvée ;

        if (currentDist <= result->distance) {
            result->distance = currentDist;
            free(result->filePath);
            result->filePath = strdup(fileList->fileNames[i]);
        }

    }
    // On retourne la structure de résultat obtenue ;
    pthread_exit(result);

}


/*
 * Compare images using 2 threads.
 * @params buffer: Raw image (char type array)
 * @params buffersize: size of the image (int)
 * @return res: queryResults type datastruct containing the best distance found (with it's linked image).
 */
struct queryResults compareImages(char buffer[], int bufsize) {

    pthread_t thread_1, thread_2;
    // Thread_1 -> Images Paires
    // Thread_2 -> Images Impaires

    struct queryResults res;
    // Structure à return contenant le nom de l'image et la
    // valeur de son hash percéptif avec celle contenue dans buffer.
    res.distance = UINT_MAX;
    res.filePath = NULL;

    const char* directory = "./img";  // [!] Le dossier /img DOIT se trouver dans le dossier serveur. (sinon utiliser ../)
    struct FileList fileList = listFiles(directory);

    if (fileList.count > 0) {
        
        // Traitement par thread pour chacun des fichiers dans fileList ;
        // On va diviser la FileList en deux sous-FileList pour que chacun des Threads aie du travail.

        struct FileList fileListThread1; fileListThread1.count = 0; fileListThread1.fileNames = NULL;
        struct FileList fileListThread2; fileListThread2.count = 0; fileListThread2.fileNames = NULL;

        for (int i = 0; i < fileList.count; i++) {
            if (i % 2 == 0) {
                fileListThread1.count++;
                fileListThread1.fileNames = realloc(fileListThread1.fileNames, fileListThread1.count * sizeof(char*));
                fileListThread1.fileNames[fileListThread1.count - 1] = strdup(fileList.fileNames[i]);
            } else {
                fileListThread2.count++;
                fileListThread2.fileNames = realloc(fileListThread2.fileNames, fileListThread2.count * sizeof(char*));
                fileListThread2.fileNames[fileListThread2.count - 1] = strdup(fileList.fileNames[i]);
            }
        }

        // On hash l'image à comparer afin de ne pas répéter inutilement son traitement ;
        uint64_t baseImage_hash;
        if (PHashRaw(buffer, bufsize, &baseImage_hash)) {

            // Structure des paramètres à passer à threadCompareImage
            struct CompareThreadParams threadParams1 = {.fileList = &fileListThread1, .baseHash = baseImage_hash};
            struct CompareThreadParams threadParams2 = {.fileList = &fileListThread2, .baseHash = baseImage_hash};

            // Lancement des Threads 1 et 2;
            if (pthread_create(&thread_1, NULL, threadCompareImages, &threadParams1) != 0) {
                perror("Error when creating Thread 1. (phtread_create() error)");
            }
            if (pthread_create(&thread_2, NULL, threadCompareImages, &threadParams2) != 0) {
                perror("Error when creating Thread 2. (phtread_create() error)");
            }

            // On récupère les résultats des threads ;

            struct queryResults* subResult1, *subResult2;

            if (pthread_join(thread_1, (void**)&subResult1) != 0) {
                perror("Error joining Thread 1. (pthread_join() error)");
                exit(1);
            }
            if (pthread_join(thread_2, (void**)&subResult2) != 0) {
                perror("Error joining Thread 2. (pthread_join() error)");
                exit(1);
            }

            // Finalement, on compare les résultats et on utilise les données de celui qui aura trouvé
            // la plus petite distance entre l'image de base et une image donnée stockée dans .filePath

            if (subResult1->distance <= subResult2->distance) {
                res.distance = subResult1->distance;
                res.filePath = subResult1->filePath;
                free(subResult2->filePath); // free parce que ne sera plus utilisé apres
            } else {
                res.distance = subResult2->distance;
                res.filePath = subResult2->filePath;
                free(subResult1->filePath);
            }

            // ... et on oublie pas de libérer l'espace alloué !

            free(subResult1);
            free(subResult2);

            freeFileList(&fileListThread1);
            freeFileList(&fileListThread2);

        } else {
            res.distance = -1;  // En cas de chemin non valide ou d'erreur d'ouverture du fichier
        }

    } else {
        printf("No image to compare in %s dir.\n", directory);
    }
    freeFileList(&fileList);  // En tout dernier lieu, on libère l'espace alloué à la liste globale des fichiers ;
    return res;
}

void* threadExec(void* params) {
    struct imgArgs* image = (struct imgArgs*) params;
    struct queryResults result = compareImages(image->buffer, image->bufferSize);
    sendResults(image->socket_fd, result.filePath, result.distance);
    free(result.filePath);
    pthread_exit(0);
}


/*
 * For testing purposes, you can save an image
 * @params buffer: Raw image
 * @params buffersize: size of the image
 * @params fileName: name of the file to save
 * @returns void
 */
void saveImage(char* buffer, int buffersize, char fileName[]) {
    FILE* file = fopen(fileName, "wb");
    fwrite(buffer, buffersize, 1, file);
    fclose(file);
}

/*
 * Read an image taken through a socket (max: 20Kb)
 * @params socket_fd: file descriptor of the socket used for the communication between the server and the client
 * @params buffer: buffer for the image
 * @returns imageSize, -1 if the file exceed 20Kb
 */
int readImage(int socket_fd, char buffer[]) {
    ssize_t imageSize = read(socket_fd, buffer, 20001);
    if (buffer[0] == '\0') {
        return -2;
    }
    if (imageSize >= 20000) {
        return -1;
    }
    return imageSize;
}

/*
 * Handle connection between the server and one client
 */
void handleConnection(int socket) {
    int communicationStatus = 1;
    while (communicationStatus) {
        char buffer[20001];
        int imageSize = readImage(socket, buffer);
        if (imageSize == -1) {
            char err[] = "The image you passed exceed 20Kb";
            write(socket, err, strlen(err));
        } else if (imageSize == -2) {
            communicationStatus = 0;
        } else {
            pthread_t new_thread;
            struct imgArgs args;
            args.buffer = &buffer;
            args.bufferSize = imageSize;
            args.socket_fd = socket;
            pthread_create(&new_thread, NULL, threadExec, &args);
        }
    }
}


int main(int argc, char* argv[]) {
    struct serverParams server = createServer();
    size_t addrlen = sizeof(server.address);
    while (1) {
        int new_socket = accept(server.fileDescriptor, (struct sockaddr_in *) &server.address, &addrlen);
        handleConnection(new_socket);
        close(new_socket);
    }
    close(server.fileDescriptor);
    return 0;
}
