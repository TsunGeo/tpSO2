#ifndef INODE_H
#define INODE_H

#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "../disco/disco.h"

#define MAX_INODES 100        // No nosso caso, vamos adotar uma política de tabela de I-nodes de tamanho fixo. Dessa forma, aqui está definido a constante
#define DIRECT_POINTERS 12    // Quantidade de ponteiros para blocos de arquivos em cada i-node

// Tipos de arquivos suportados pelo simulador
typedef enum FileType{
    TYPE_FILE,
    DIRECTORY
}FileType;

typedef struct Inode{
    int iNodeID;                  // ID do Inode
    int isBeingUsed;              // 0: não está sendo usado; 1: está sendo usado

    FileType type;                // Tipo: arquivo ou diretório
    int size;                     // Tamanho atual do arquivo em bytes

    time_t creationDate;          // Timestamp de criação
    time_t modificationDate;      // Timestamp de alteração de conteúdo
    time_t accessDate;            // Timestamp de leitura

    uint32_t blocks[DIRECT_POINTERS];  // Os ponteiros para blocos de arquivos serão representados pelos ponteiros de cada posição do vetor
    int quantBlocks;
}Inode;

void initializeInode(Inode* iNodeTable);
int allocInode(Inode iNodeTable[], FileType type);

void addBlockToInode(Inode iNodeTable[], int iNodeIndex, uint32_t blockIndex);
int removeBlockFromInode(Inode iNodeTable[], int iNodeIndex, uint32_t blockIndex);
int getBlockFromInode(Inode iNodeTable[], int iNodeIndex, int pointerIndex, uint32_t *blockIndex);

void iNodeReadData(VirtualDisk* disk, Inode iNodeTable[], int iNodeIndex, void* buffer);
void iNodeWriteData(VirtualDisk* disk, Inode iNodeTable[], int iNodeIndex, const void* buffer, uint32_t size);

void freeInode(Inode iNodeTable[], int index);


#endif // INODE_H