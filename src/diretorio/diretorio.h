#ifndef DIRETORIO_H
#define DIRETORIO_H

#include "../i-node/inode.h"
#define TAM_NOME 255

typedef struct{
    char nome[TAM_NOME]; // nome do arquivo ou diretório
    int inode; // inode ligado a esse nome
    FileType type; // FILE ou DIRECTORY
    int usado; // 0 = posição vazia, 1 = posição ocupada
} Diretorio;

void criarDiretorio(VirtualDisk *disk, Inode inodeTable[], int inodePai, char *nome);
void apagarDiretorio(VirtualDisk *disk, Inode inodeTable[], int inodePai, char *nome);
void renomearDiretorio(VirtualDisk *disk, Inode inodeTable[],  int inodePai, char *nomeAntigo, char *novoNome);
void listarConteudoDiretorio(VirtualDisk *disk, Inode inodeTable[], int inodeDir);


#endif // DIRETORIO_H