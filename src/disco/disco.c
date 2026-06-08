#include "disco.h"

#include <stdlib.h>
#include <string.h>

static int isPowerOfTwo(uint32_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

static uint32_t calculateBitmapSize(uint32_t totalBlocks) {
    return (totalBlocks + 7) / 8;
}

/* Garante que a particao tenha limites simples e blocos faceis de enderecar. */
static int validateDiskConfig(uint32_t diskSize, uint32_t blockSize) {
    if (diskSize < MIN_DISK_SIZE || diskSize > MAX_DISK_SIZE) {
        return OPERATION_ERROR;
    }

    if (blockSize < MIN_BLOCK_SIZE || blockSize > MAX_BLOCK_SIZE) {
        return OPERATION_ERROR;
    }

    if (!isPowerOfTwo(blockSize)) {
        return OPERATION_ERROR;
    }

    if (diskSize < blockSize * 4) {
        return OPERATION_ERROR;
    }

    return OPERATION_OK;
}

static long getBlockOffset(const VirtualDisk *disk, uint32_t blockIndex) {
    return (long)disk->header.dataStart + ((long)blockIndex * disk->header.blockSize);
}

/* Cada bit do bitmap representa um bloco: 0 para livre, 1 para ocupado. */
static int getBitmapValue(const unsigned char *bitmap, uint32_t blockIndex) {
    uint32_t byteIndex = blockIndex / 8;
    uint32_t bitIndex = blockIndex % 8;
    return (bitmap[byteIndex] >> bitIndex) & 1;
}

static void setBitmapValue(unsigned char *bitmap, uint32_t blockIndex, int value) {
    uint32_t byteIndex = blockIndex / 8;
    uint32_t bitIndex = blockIndex % 8;

    if (value == BLOCK_USED) {
        bitmap[byteIndex] = (unsigned char)(bitmap[byteIndex] | (1u << bitIndex));
    } else {
        bitmap[byteIndex] = (unsigned char)(bitmap[byteIndex] & ~(1u << bitIndex));
    }
}

int createVirtualDisk(const char *path, uint32_t diskSize, uint32_t blockSize) {
    FILE *file;
    VirtualDiskHeader header;
    unsigned char *bitmap;
    uint32_t totalBlocks;
    uint32_t metadataSize;

    if (validateDiskConfig(diskSize, blockSize) != OPERATION_OK) {
        return OPERATION_ERROR;
    }

    totalBlocks = diskSize / blockSize;
    metadataSize = (uint32_t)sizeof(VirtualDiskHeader) + calculateBitmapSize(totalBlocks);

    /* Reserva espaco para cabecalho e bitmap antes da area de dados. */
    while ((totalBlocks * blockSize) + metadataSize > diskSize) {
        totalBlocks--;
        metadataSize = (uint32_t)sizeof(VirtualDiskHeader) + calculateBitmapSize(totalBlocks);
    }

    memset(&header, 0, sizeof(header));
    memcpy(header.magic, DISK_MAGIC, strlen(DISK_MAGIC));
    header.diskSize = diskSize;
    header.blockSize = blockSize;
    header.totalBlocks = totalBlocks;
    header.freeBlocks = totalBlocks;
    header.bitmapStart = (uint32_t)sizeof(VirtualDiskHeader);
    header.bitmapSize = calculateBitmapSize(totalBlocks);
    header.dataStart = header.bitmapStart + header.bitmapSize;

    bitmap = calloc(header.bitmapSize, sizeof(unsigned char));
    if (bitmap == NULL) {
        return OPERATION_ERROR;
    }

    file = fopen(path, "wb+");
    if (file == NULL) {
        free(bitmap);
        return OPERATION_ERROR;
    }

    if (fwrite(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        free(bitmap);
        return OPERATION_ERROR;
    }

    if (fwrite(bitmap, header.bitmapSize, 1, file) != 1) {
        fclose(file);
        free(bitmap);
        return OPERATION_ERROR;
    }

    if (fseek(file, (long)diskSize - 1, SEEK_SET) != 0 || fputc('\0', file) == EOF) {
        fclose(file);
        free(bitmap);
        return OPERATION_ERROR;
    }

    fclose(file);
    free(bitmap);
    return OPERATION_OK;
}

int openVirtualDisk(VirtualDisk *disk, const char *path) {
    if (disk == NULL || path == NULL) {
        return OPERATION_ERROR;
    }

    memset(disk, 0, sizeof(*disk));
    disk->file = fopen(path, "rb+");
    if (disk->file == NULL) {
        return OPERATION_ERROR;
    }

    if (fread(&disk->header, sizeof(disk->header), 1, disk->file) != 1) {
        closeVirtualDisk(disk);
        return OPERATION_ERROR;
    }

    if (memcmp(disk->header.magic, DISK_MAGIC, strlen(DISK_MAGIC)) != 0) {
        closeVirtualDisk(disk);
        return OPERATION_ERROR;
    }

    disk->bitmap = malloc(disk->header.bitmapSize);
    if (disk->bitmap == NULL) {
        closeVirtualDisk(disk);
        return OPERATION_ERROR;
    }

    if (fseek(disk->file, disk->header.bitmapStart, SEEK_SET) != 0) {
        closeVirtualDisk(disk);
        return OPERATION_ERROR;
    }

    if (fread(disk->bitmap, disk->header.bitmapSize, 1, disk->file) != 1) {
        closeVirtualDisk(disk);
        return OPERATION_ERROR;
    }

    strncpy(disk->path, path, sizeof(disk->path) - 1);
    return OPERATION_OK;
}

void closeVirtualDisk(VirtualDisk *disk) {
    if (disk == NULL) {
        return;
    }

    if (disk->file != NULL) {
        fclose(disk->file);
        disk->file = NULL;
    }

    free(disk->bitmap);
    disk->bitmap = NULL;
}

int saveDiskMetadata(VirtualDisk *disk) {
    if (disk == NULL || disk->file == NULL || disk->bitmap == NULL) {
        return OPERATION_ERROR;
    }

    /* Persistir cabecalho e bitmap mantem o estado do disco entre execucoes. */
    if (fseek(disk->file, 0, SEEK_SET) != 0) {
        return OPERATION_ERROR;
    }

    if (fwrite(&disk->header, sizeof(disk->header), 1, disk->file) != 1) {
        return OPERATION_ERROR;
    }

    if (fseek(disk->file, disk->header.bitmapStart, SEEK_SET) != 0) {
        return OPERATION_ERROR;
    }

    if (fwrite(disk->bitmap, disk->header.bitmapSize, 1, disk->file) != 1) {
        return OPERATION_ERROR;
    }

    fflush(disk->file);
    return OPERATION_OK;
}

int allocateBlock(VirtualDisk *disk, uint32_t *blockIndex) {
    uint32_t index;

    if (disk == NULL || blockIndex == NULL || disk->bitmap == NULL || disk->header.freeBlocks == 0) {
        return OPERATION_ERROR;
    }

    for (index = 0; index < disk->header.totalBlocks; index++) {
        if (getBitmapValue(disk->bitmap, index) == BLOCK_FREE) {
            setBitmapValue(disk->bitmap, index, BLOCK_USED);
            disk->header.freeBlocks--;
            *blockIndex = index;
            return saveDiskMetadata(disk);
        }
    }

    return OPERATION_ERROR;
}

int freeBlock(VirtualDisk *disk, uint32_t blockIndex) {
    if (disk == NULL || disk->bitmap == NULL || blockIndex >= disk->header.totalBlocks) {
        return OPERATION_ERROR;
    }

    if (getBitmapValue(disk->bitmap, blockIndex) == BLOCK_FREE) {
        return OPERATION_ERROR;
    }

    setBitmapValue(disk->bitmap, blockIndex, BLOCK_FREE);
    disk->header.freeBlocks++;
    return saveDiskMetadata(disk);
}

int isBlockFree(const VirtualDisk *disk, uint32_t blockIndex) {
    if (disk == NULL || disk->bitmap == NULL || blockIndex >= disk->header.totalBlocks) {
        return OPERATION_ERROR;
    }

    return getBitmapValue(disk->bitmap, blockIndex) == BLOCK_FREE;
}

int readBlock(VirtualDisk *disk, uint32_t blockIndex, void *buffer, uint32_t bufferSize) {
    if (disk == NULL || disk->file == NULL || buffer == NULL || blockIndex >= disk->header.totalBlocks) {
        return OPERATION_ERROR;
    }

    if (bufferSize < disk->header.blockSize || isBlockFree(disk, blockIndex)) {
        return OPERATION_ERROR;
    }

    if (fseek(disk->file, getBlockOffset(disk, blockIndex), SEEK_SET) != 0) {
        return OPERATION_ERROR;
    }

    if (fread(buffer, disk->header.blockSize, 1, disk->file) != 1) {
        return OPERATION_ERROR;
    }

    return OPERATION_OK;
}

int writeBlock(VirtualDisk *disk, uint32_t blockIndex, const void *buffer, uint32_t bufferSize) {
    if (disk == NULL || disk->file == NULL || buffer == NULL || blockIndex >= disk->header.totalBlocks) {
        return OPERATION_ERROR;
    }

    /* Escrita so e permitida em blocos ja alocados por outro modulo. */
    if (bufferSize > disk->header.blockSize || isBlockFree(disk, blockIndex)) {
        return OPERATION_ERROR;
    }

    if (fseek(disk->file, getBlockOffset(disk, blockIndex), SEEK_SET) != 0) {
        return OPERATION_ERROR;
    }

    if (fwrite(buffer, bufferSize, 1, disk->file) != 1) {
        return OPERATION_ERROR;
    }

    fflush(disk->file);
    return OPERATION_OK;
}

void printDiskInfo(const VirtualDisk *disk) {
    if (disk == NULL) {
        return;
    }

    printf("Disco: %s\n", disk->path);
    printf("Tamanho total: %u bytes\n", disk->header.diskSize);
    printf("Tamanho do bloco: %u bytes\n", disk->header.blockSize);
    printf("Total de blocos de dados: %u\n", disk->header.totalBlocks);
    printf("Blocos livres: %u\n", disk->header.freeBlocks);
    printf("Blocos usados: %u\n", disk->header.totalBlocks - disk->header.freeBlocks);
    printf("Inicio do bitmap: byte %u\n", disk->header.bitmapStart);
    printf("Inicio dos dados: byte %u\n", disk->header.dataStart);
}
