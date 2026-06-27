#include "importacao.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

int importRealFile(VirtualDisk *disk, Inode inodeTable[], int parentInode, const char *realFilePath, const char *simulatedFileName) {
    if (disk == NULL || inodeTable == NULL || realFilePath == NULL || simulatedFileName == NULL) {
        printf("Erro: parametros invalidos para importacao.\n");
        return OPERATION_ERROR;
    }

    // Obter informações do arquivo real usando stat
    struct stat st;
    if (stat(realFilePath, &st) != 0) {
        printf("Erro: nao foi possivel obter informacoes do arquivo real '%s'.\n", realFilePath);
        return OPERATION_ERROR;
    }

    // Validar limitações de tamanho do arquivo simulado (DIRECT_POINTERS)
    uint32_t blockSize = disk->header.blockSize;
    uint32_t maxFileSize = DIRECT_POINTERS * blockSize;
    if ((uint32_t)st.st_size > maxFileSize) {
        printf("Erro: O arquivo real (%ld bytes) excede o tamanho maximo permitido para um arquivo simulado (%u bytes, limitados a %d blocos de %u bytes).\n",
               (long)st.st_size, maxFileSize, DIRECT_POINTERS, blockSize);
        return OPERATION_ERROR;
    }

    // Validar se há blocos livres suficientes no disco virtual simulado
    uint32_t blocksNeeded = (st.st_size > 0) ? (st.st_size + blockSize - 1) / blockSize : 0;
    if (blocksNeeded > disk->header.freeBlocks) {
        printf("Erro: Nao ha blocos livres suficientes no disco simulado. Necessarios: %u, disponiveis: %u.\n",
               blocksNeeded, disk->header.freeBlocks);
        return OPERATION_ERROR;
    }

    // Abrir o arquivo real e ler seu conteúdo para a memória
    FILE *realFile = fopen(realFilePath, "rb");
    if (realFile == NULL) {
        printf("Erro: nao foi possivel abrir o arquivo real '%s'.\n", realFilePath);
        return OPERATION_ERROR;
    }

    void *buffer = malloc(st.st_size > 0 ? st.st_size : 1);
    if (buffer == NULL) {
        printf("Erro: falha ao alocar memoria para o buffer de importacao.\n");
        fclose(realFile);
        return OPERATION_ERROR;
    }

    if (st.st_size > 0) {
        size_t readBytes = fread(buffer, 1, st.st_size, realFile);
        if (readBytes != (size_t)st.st_size) {
            printf("Erro: falha ao ler o conteudo do arquivo real.\n");
            free(buffer);
            fclose(realFile);
            return OPERATION_ERROR;
        }
    } else {
        ((char *)buffer)[0] = '\0';
    }
    fclose(realFile);

    // Verificar se já existe uma entrada com esse nome no diretório pai simulado
    int existingInode = searchEntry(disk, inodeTable, parentInode, (char *)simulatedFileName);
    if (existingInode != -1) {
        printf("Aviso: o arquivo simulado '%s' ja existe. Sobrescrevendo...\n", simulatedFileName);
        deleteFile(disk, inodeTable, parentInode, (char *)simulatedFileName);
    }

    // Criar o arquivo no sistema de arquivos simulado
    createFile(disk, inodeTable, parentInode, (char *)simulatedFileName);

    // Buscar o inode alocado para o novo arquivo simulado
    int newInode = searchEntry(disk, inodeTable, parentInode, (char *)simulatedFileName);
    if (newInode == -1) {
        printf("Erro: falha ao encontrar o arquivo recem-criado no diretorio simulado.\n");
        free(buffer);
        return OPERATION_ERROR;
    }

    // Escrever os dados no arquivo simulado
    iNodeWriteData(disk, inodeTable, newInode, buffer, (uint32_t)st.st_size);
    inodeTable[newInode].size = (uint32_t)st.st_size;

    // Sobrescrever as datas do Inode simulado com os metadados do arquivo real obtidos via stat
    inodeTable[newInode].modificationDate = st.st_mtime;
    inodeTable[newInode].accessDate = st.st_atime;
#ifdef __linux__
    inodeTable[newInode].creationDate = st.st_ctime;
#else
    inodeTable[newInode].creationDate = st.st_mtime;
#endif

    free(buffer);
    printf("Arquivo real '%s' importado com sucesso para '%s' (%ld bytes).\n", realFilePath, simulatedFileName, (long)st.st_size);
    return OPERATION_OK;
}

void printFileMetadata(Inode inodeTable[], int inodeIndex) {
    if (inodeIndex < 0 || inodeIndex >= MAX_INODES || !inodeTable[inodeIndex].isBeingUsed) {
        printf("Erro: Inode %d invalido ou nao utilizado.\n", inodeIndex);
        return;
    }

    Inode *inode = &inodeTable[inodeIndex];
    char cTimeStr[30], mTimeStr[30], aTimeStr[30];
    struct tm *tm_info;

    tm_info = localtime(&inode->creationDate);
    if (tm_info != NULL) {
        strftime(cTimeStr, sizeof(cTimeStr), "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        strcpy(cTimeStr, "N/A");
    }

    tm_info = localtime(&inode->modificationDate);
    if (tm_info != NULL) {
        strftime(mTimeStr, sizeof(mTimeStr), "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        strcpy(mTimeStr, "N/A");
    }

    tm_info = localtime(&inode->accessDate);
    if (tm_info != NULL) {
        strftime(aTimeStr, sizeof(aTimeStr), "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        strcpy(aTimeStr, "N/A");
    }

    printf("--- Metadados do Inode %d (Arquivo Simulado) ---\n", inodeIndex);
    printf("Tipo: %s\n", (inode->type == TYPE_FILE) ? "Arquivo" : "Diretorio");
    printf("Tamanho: %d bytes\n", inode->size);
    printf("Blocos alocados: %d\n", inode->quantBlocks);
    printf("Ponteiros de bloco: ");
    for (int i = 0; i < inode->quantBlocks; i++) {
        if (inode->blocks[i] != (uint32_t)-1) {
            printf("%u ", inode->blocks[i]);
        }
    }
    printf("\n");
    printf("Data de Criacao: %s\n", cTimeStr);
    printf("Data de Modificacao: %s\n", mTimeStr);
    printf("Data de Ultimo Acesso: %s\n", aTimeStr);
    printf("-----------------------------------------------\n");
}
