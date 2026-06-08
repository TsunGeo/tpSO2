#ifndef DISCO_H
#define DISCO_H

#include <stdint.h>
#include <stdio.h>

#include "../config/config.h"

typedef struct {
    char magic[DISK_MAGIC_SIZE];
    uint32_t diskSize;
    uint32_t blockSize;
    uint32_t totalBlocks;
    uint32_t freeBlocks;
    uint32_t bitmapStart;
    uint32_t bitmapSize;
    uint32_t dataStart;
} VirtualDiskHeader;

/* Representa um disco aberto em memoria enquanto o simulador esta rodando. */
typedef struct {
    FILE *file;
    char path[256];
    VirtualDiskHeader header;
    unsigned char *bitmap;
} VirtualDisk;

int createVirtualDisk(const char *path, uint32_t diskSize, uint32_t blockSize);
int openVirtualDisk(VirtualDisk *disk, const char *path);
void closeVirtualDisk(VirtualDisk *disk);

int allocateBlock(VirtualDisk *disk, uint32_t *blockIndex);
int freeBlock(VirtualDisk *disk, uint32_t blockIndex);
int isBlockFree(const VirtualDisk *disk, uint32_t blockIndex);

int readBlock(VirtualDisk *disk, uint32_t blockIndex, void *buffer, uint32_t bufferSize);
int writeBlock(VirtualDisk *disk, uint32_t blockIndex, const void *buffer, uint32_t bufferSize);

void printDiskInfo(const VirtualDisk *disk);
int saveDiskMetadata(VirtualDisk *disk);

#endif
