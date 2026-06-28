#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config/config.h"
#include "disco/disco.h"
#include "interface/interface.h"

static void printHelp(void) {
    printf("Uso:\n");
    printf("  ./tp2_so init <arquivo_disco> <tamanho_disco> <tamanho_bloco>\n");
    printf("  ./tp2_so info <arquivo_disco>\n");
    printf("  ./tp2_so shell <arquivo_disco> [-v]\n");
    printf("  ./tp2_so batch <arquivo_disco> <arquivo_comandos> [-v]\n");
    printf("\nExemplo:\n");
    printf("  ./tp2_so init disco.bin 1048576 4096\n");
    printf("  ./tp2_so shell disco.bin -v\n");
    printf("  ./tp2_so batch disco.bin testes/comandos.txt -v\n");
}

static uint32_t parseUint(const char *text) {
    return (uint32_t)strtoul(text, NULL, 10);
}

static int hasVerboseFlag(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            return 1;
        }
    }
    return 0;
}

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
            printf("Erro ao criar disco simulado. Verifique tamanho do disco e tamanho do bloco.\n");
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

    if (strcmp(argv[1], "shell") == 0) {
        if (argc < 3 || argc > 4) {
            printHelp();
            return 1;
        }
        return runInteractiveMode(argv[2], hasVerboseFlag(argc, argv));
    }

    if (strcmp(argv[1], "batch") == 0) {
        if (argc < 4 || argc > 5) {
            printHelp();
            return 1;
        }
        return runBatchMode(argv[2], argv[3], hasVerboseFlag(argc, argv));
    }

    printHelp();
    return 1;
}
