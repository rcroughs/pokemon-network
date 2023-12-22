#include "filelist.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct FileList listFiles(const char *directory) {
  struct FileList fileList;
  fileList.fileNames = NULL;
  fileList.count = 0;

  DIR *dir = opendir(directory);
  if (dir == NULL) {
    perror("opendir() error.");
    return fileList;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_REG) {
      fileList.count++;
      fileList.fileNames =
          realloc(fileList.fileNames, fileList.count * sizeof(char *));

      // On doit construire le chemin complet pour pouvoir utiliser directement
      // les path dans la fonction de hash ;
      char *fullPath =
          malloc(strlen(directory) + 1 + strlen(entry->d_name) + 1);
      strcpy(fullPath, directory);
      strcat(fullPath, "/");
      strcat(fullPath, entry->d_name);

      fileList.fileNames[fileList.count - 1] = fullPath;
    }
  }

  closedir(dir);
  return fileList;
}

void freeFileList(struct FileList *fileList) {
  for (int i = 0; i < fileList->count; i++) {
    free(fileList->fileNames[i]);
  }
  free(fileList->fileNames);
}