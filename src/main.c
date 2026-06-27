#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config/config.h"
#include "disco/disco.h"
#include "i-node/inode.h"
#include "diretorio/diretorio.h"
#include "arquivo/arquivo.h"
#include "importacao/importacao.h"


static void printHelp(void) {
    printf("Uso:\n");
    printf("  ./tp2_so init <arquivo_disco> <tamanho_disco> <tamanho_bloco>\n");
    printf("  ./tp2_so info <arquivo_disco>\n");
    printf("  ./tp2_so alloc <arquivo_disco>\n");
    printf("  ./tp2_so free <arquivo_disco> <indice_bloco>\n");
    printf("  ./tp2_so testfile <arquivo_disco>\n");
}

static uint32_t parseUint(const char *text) {
    return (uint32_t)strtoul(text, NULL, 10);
}

/* Interface temporaria para testar o modulo de disco antes dos outros modulos. */
int main(int argc, char *argv[]) {
    VirtualDisk disk;

    if (argc < 2) {
        printHelp();
        return 1;
    }

    if (strcmp(argv[1], "init") == 0) {
        if (argc != 5) {
            printHelp();
            return 1;
        }

        if (createVirtualDisk(argv[2], parseUint(argv[3]), parseUint(argv[4])) != OPERATION_OK) {
            printf("Erro ao criar disco simulado.\n");
            return 1;
        }

        printf("Disco simulado criado com sucesso.\n");
        return 0;
    }

    if (strcmp(argv[1], "info") == 0) {
        if (argc != 3 || openVirtualDisk(&disk, argv[2]) != OPERATION_OK) {
            printf("Erro ao abrir disco simulado.\n");
            return 1;
        }

        printDiskInfo(&disk);
        closeVirtualDisk(&disk);
        return 0;
    }

    if (strcmp(argv[1], "alloc") == 0) {
        uint32_t blockIndex;

        if (argc != 3 || openVirtualDisk(&disk, argv[2]) != OPERATION_OK) {
            printf("Erro ao abrir disco simulado.\n");
            return 1;
        }

        if (allocateBlock(&disk, &blockIndex) != OPERATION_OK) {
            printf("Erro ao alocar bloco.\n");
            closeVirtualDisk(&disk);
            return 1;
        }

        printf("Bloco alocado: %u\n", blockIndex);
        closeVirtualDisk(&disk);
        return 0;
    }

    if (strcmp(argv[1], "free") == 0) {
        if (argc != 4 || openVirtualDisk(&disk, argv[2]) != OPERATION_OK) {
            printf("Erro ao abrir disco simulado.\n");
            return 1;
        }

        if (freeBlock(&disk, parseUint(argv[3])) != OPERATION_OK) {
            printf("Erro ao liberar bloco.\n");
            closeVirtualDisk(&disk);
            return 1;
        }

        printf("Bloco liberado com sucesso.\n");
        closeVirtualDisk(&disk);
        return 0;
    }

    if (strcmp(argv[1], "testfile") == 0) {

    if (argc != 3 || openVirtualDisk(&disk, argv[2]) != OPERATION_OK) {
        printf("Erro ao abrir disco simulado.\n");
        return 1;
    }

    Inode inodeTable[MAX_INODES];

    initializeInode(inodeTable);

    int root = allocInode(inodeTable, DIRECTORY);

    if (root == -1) {
        printf("Erro ao criar diretorio raiz.\n");
        closeVirtualDisk(&disk);
        return 1;
    }

    printf("\n========== TESTE DO MODULO DE ARQUIVOS ==========\n");

    /* Cria um arquivo */
    createFile(&disk, inodeTable, root, "teste.txt");

    /* Escreve no arquivo */
    char texto[] = "Trabalho Pratico de Sistemas Operacionais";

    writeFile(&disk, inodeTable, root, "teste.txt", texto, strlen(texto) + 1);

    /* Leitura */

    char buffer[100];

    readFile(&disk, inodeTable, root, "teste.txt", buffer);

    printf("\nConteudo lido:\n%s\n", buffer);

    /* Exibe usando a função */

    displayFile(&disk, inodeTable, root, "teste.txt");

    /* Renomeia */

    renameFile(&disk, inodeTable, root, "teste.txt", "arquivo.txt");

    /* Move */

    createDirectory(&disk, inodeTable, root, "backup");

    int backup = searchEntry(&disk, inodeTable, root, "backup");

    if (backup != -1) {

        moveFile(&disk, inodeTable, root, backup, "arquivo.txt");
    }

    /* Remove */

    deleteFile(&disk, inodeTable, backup, "arquivo.txt");

   void testeImportacao(VirtualDisk *disk, Inode inodeTable[], int rootInode);

    printf("\n========== FIM DOS TESTES ==========\n");

    closeVirtualDisk(&disk);

    return 0;
    }
    printHelp();
    return 1;
}
