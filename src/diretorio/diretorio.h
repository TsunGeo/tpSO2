#ifndef DIRETORIO_H
#define DIRETORIO_H

#include "../i-node/inode.h"
#define TAM_NOME 255

typedef struct{
    char name[TAM_NOME]; // nome do arquivo ou diretório
    int inode; // inode ligado a esse nome
    FileType type; // FILE ou DIRECTORY
    int used; // 0 = posição vazia, 1 = posição ocupada
} DirectoryEntry;

void createDirectory(VirtualDisk *disk, Inode inodeTable[], int inodeParent, char *name);
void deleteDirectory(VirtualDisk *disk, Inode inodeTable[], int inodeParent, char *name);
void renameDirectory(VirtualDisk *disk, Inode inodeTable[],  int inodeParent, char *oldName, char *newName);
void listDirectoryContent(VirtualDisk *disk, Inode inodeTable[], int inodeDir);


#endif // DIRETORIO_H