#include "thread.h"

#include "imageio.h"
#include "imgdist.h"
#include "filelist.h"
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void sendResults(int socket_fd, char mostSimilarImage[], int distance,
                 char img_id) {
  char response[999];
  // printf("\nDistance = %d\n", distance);
  if (distance == -1) {
    response[0] = CHAR_MAX;
    sprintf(response + 1, "No similar image found (no comparison could be "
                          "performed successfully).");
  } else {
    response[0] = img_id;
    sprintf(response + 1,
            "Most similar image found: '%s' with a distance of %d.",
            mostSimilarImage, distance);
  }
  write(socket_fd, response, 999);
}

void *threadCompareImages(void *params) {

  struct CompareThreadParams *threadParams =
      (struct CompareThreadParams *)params;

  struct FileList *fileList = threadParams->fileList;
  uint64_t baseImage_hash = threadParams->baseHash;

  // Initialisation de la structure contenant les résultats.
  struct queryResults *result = malloc(sizeof(struct queryResults));
  result->distance = INT_MAX;
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
      printf("%d", currentDist);
      free(result->filePath);
      result->filePath = strdup(fileList->fileNames[i]);
    }
  }
  // On retourne la structure de résultat obtenue ;
  pthread_exit(result);
}

struct queryResults findBestMatchingImage(char buffer[], int bufsize) {

  pthread_t thread_1, thread_2;
  // Thread_1 -> Images Paires
  // Thread_2 -> Images Impaires

  struct queryResults res;
  // Structure à return contenant le nom de l'image et la
  // valeur de son hash percéptif avec celle contenue dans buffer.
  res.distance = UINT_MAX;
  res.filePath = NULL;

  const char *directory = "img"; // [!] Le dossier /img DOIT se trouver dans le
                                 // dossier serveur. (sinon utiliser ../)
  struct FileList fileList = listFiles(directory);

  if (fileList.count > 0) {

    // Traitement par thread pour chacun des fichiers dans fileList ;
    // On va diviser la FileList en deux sous-FileList pour que chacun des
    // Threads aie du travail.

    struct FileList fileListThread1;
    fileListThread1.count = 0;
    fileListThread1.fileNames = NULL;
    struct FileList fileListThread2;
    fileListThread2.count = 0;
    fileListThread2.fileNames = NULL;

    for (int i = 0; i < fileList.count; i++) {
      if (i % 2 == 0) {
        fileListThread1.count++;
        fileListThread1.fileNames = realloc(
            fileListThread1.fileNames, fileListThread1.count * sizeof(char *));
        fileListThread1.fileNames[fileListThread1.count - 1] =
            strdup(fileList.fileNames[i]);
      } else {
        fileListThread2.count++;
        fileListThread2.fileNames = realloc(
            fileListThread2.fileNames, fileListThread2.count * sizeof(char *));
        fileListThread2.fileNames[fileListThread2.count - 1] =
            strdup(fileList.fileNames[i]);
      }
    }

    // On hash l'image à comparer afin de ne pas répéter inutilement son
    // traitement ;
    uint64_t baseImage_hash;
    if (PHashRaw(buffer, bufsize, &baseImage_hash)) {

      // Structure des paramètres à passer à threadCompareImage
      struct CompareThreadParams threadParams1 = {.fileList = &fileListThread1,
                                                  .baseHash = baseImage_hash};
      struct CompareThreadParams threadParams2 = {.fileList = &fileListThread2,
                                                  .baseHash = baseImage_hash};

      // Lancement des Threads 1 et 2;
      if (pthread_create(&thread_1, NULL, threadCompareImages,
                         &threadParams1) != 0) {
        perror("Error when creating Thread 1. (phtread_create() error)");
      }
      if (pthread_create(&thread_2, NULL, threadCompareImages,
                         &threadParams2) != 0) {
        perror("Error when creating Thread 2. (phtread_create() error)");
      }

      // On récupère les résultats des threads ;

      struct queryResults *subResult1, *subResult2;

      if (pthread_join(thread_1, (void **)&subResult1) != 0) {
        perror("Error joining Thread 1. (pthread_join() error)");
        exit(1);
      }
      if (pthread_join(thread_2, (void **)&subResult2) != 0) {
        perror("Error joining Thread 2. (pthread_join() error)");
        exit(1);
      }

      // Finalement, on compare les résultats et on utilise les données de celui
      // qui aura trouvé la plus petite distance entre l'image de base et une
      // image donnée stockée dans .filePath

      if (subResult1->distance < subResult2->distance) {
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
      res.distance =
          -1; // En cas de chemin non valide ou d'erreur d'ouverture du fichier
    }

  } else {
    printf("No image to compare in %s dir.\n", directory);
  }
  freeFileList(&fileList); // En tout dernier lieu, on libère l'espace alloué à
                           // la liste globale des fichiers ;
  return res;
}

void* launchQuery(void *args) {
  struct imgArgs image = *(struct imgArgs *)args;
  struct queryResults result = findBestMatchingImage(image.buffer, image.bufferSize);
  sendResults(image.socket_fd, result.filePath, result.distance, image.img_id);
  free(result.filePath);
  pthread_exit(0);
}