//===-- serveur/filelist.h - Get files of a directory ----------*- C -*-===//
//
// This file is ment to be used by the server to get the list of files in a
// given directory
//
//===-------------------------------------------------------------===//


/*
 * Structure représentant la liste des images dans img (+ le nombre)
 * @member fileNames: tableau de chaînes de caractères contenant les noms des
 * fichiers
 * @member count: nombre de fichiers dans le tableau
 */
struct FileList {
  char **fileNames;
  int count;
};

/*
 * List files in given directory. [!] Directory should only be accessible
 * server-side.
 * @params directory: Directory which will be searched.
 * @return fileList structure containing the number of files and their paths in
 * an array.
 */
struct FileList listFiles(const char *directory);

/*
 * Free the memory used by given list.
 * @params fileList : structure containing the number of files and their paths
 * in an array.
 */
void freeFileList(struct FileList *fileList);