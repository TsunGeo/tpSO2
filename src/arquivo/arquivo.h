#ifndef ARQUIVO_H
#define ARQUIVO_H

#include <string.h>
#include "../i-node/inode.h"

/*Cria um novo arquivo no diretorio*/
void createFile(VirtualDisk *disk, Inode inodeTable[], int parentInode, char *name);

/*Remove arquivo do diretorio informado*/
void deleteFile(VirtualDisk *disk, Inode inodeTable[], int parentInode, char *name);

/*Renomeia um arquivo*/
void renameFile(VirtualDisk *disk, Inode inodeTable[], int parentInode, char *oldName, char *newName);

/*Move um arquivo entre diretorios*/
void moveFile(VirtualDisk *disk, Inode inodeTable[], int sourceDirectory, int destinationDirectory, char *name);
 
/*Escreve dados no arquivo*/
void writeFile(VirtualDisk *disk, Inode inodeTable[], int parentInode, char *name, const void *buffer, uint32_t size);

/*Le os dados armazenados no arquivo*/
void readFile(VirtualDisk *disk, Inode inodeTable[], int parentInode, char *name, void *buffer);

/*Exibe o conteúdo armazenado no arquivo*/
void displayFile(VirtualDisk *disk, Inode inodeTable[], int parentInode, char *name);

#endif