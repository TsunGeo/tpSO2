#ifndef IMPORTACAO_H
#define IMPORTACAO_H

#include "../disco/disco.h"
#include "../i-node/inode.h"
#include "../arquivo/arquivo.h"
#include "../diretorio/diretorio.h"

int importRealFile(VirtualDisk *disk, Inode inodeTable[], int parentInode, const char *realFilePath, const char *simulatedFileName);

void printFileMetadata(Inode inodeTable[], int inodeIndex);

#endif