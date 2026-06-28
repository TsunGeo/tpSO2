#include "interface.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config/config.h"
#include "disco/disco.h"
#include "i-node/inode.h"
#include "diretorio/diretorio.h"
#include "arquivo/arquivo.h"
#include "importacao/importacao.h"
#include "interface_utils.h"
#include "interface_view.h"

#define INPUT_SIZE 1024
#define PATH_SIZE 512


static int executeCommand(VirtualDisk *disk, Inode inodeTable[], char *line, int *verbose, int interactive) {
    char original[INPUT_SIZE];
    char *command;
    char *arg1;
    char *arg2;

    trimLine(line);
    if (line[0] == '\0' || line[0] == '#') {
        return 1;
    }

    snprintf(original, sizeof(original), "%s", line);

    command = strtok(line, " \t");
    if (command == NULL) {
        return 1;
    }

    if (*verbose) {
        printf(C_CYAN "[narrador]" C_RESET " Executando comando: %s\n", original);
    }

    if (strcmp(command, "help") == 0) {
        printInterfaceHelp();
        return 1;
    }

    if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
        return 0;
    }

    if (strcmp(command, "info") == 0) {
        printPrettyDiskInfo(disk);
        return 1;
    }

    if (strcmp(command, "verbose") == 0) {
        arg1 = strtok(NULL, " \t");
        if (arg1 == NULL) {
            printf("Modo narrador: %s\n", *verbose ? "ligado" : "desligado");
        } else if (strcmp(arg1, "on") == 0) {
            *verbose = 1;
            printf(C_GREEN "OK: modo narrador ligado.\n" C_RESET);
        } else if (strcmp(arg1, "off") == 0) {
            *verbose = 0;
            printf(C_GREEN "OK: modo narrador desligado.\n" C_RESET);
        } else {
            printf(C_YELLOW "Uso: verbose on|off\n" C_RESET);
        }
        return 1;
    }

    if (strcmp(command, "ls") == 0) {
        int inodeDir;
        arg1 = strtok(NULL, " \t");
        inodeDir = resolvePath(disk, inodeTable, arg1 == NULL ? "/" : arg1);
        if (inodeDir == -1) {
            printf("Erro: diretorio nao encontrado.\n");
        } else {
            listDirectoryContent(disk, inodeTable, inodeDir);
        }
        return 1;
    }

    if (strcmp(command, "mkdir") == 0 || strcmp(command, "rmdir") == 0 || strcmp(command, "touch") == 0 || strcmp(command, "rm") == 0 || strcmp(command, "cat") == 0 || strcmp(command, "meta") == 0) {
        char name[TAM_NOME];
        int parent;
        int inodeIndex;

        arg1 = strtok(NULL, " \t");
        if (arg1 == NULL) {
            printf("Erro: informe um caminho.\n");
            return 1;
        }

        if (strcmp(command, "cat") == 0) {
            parent = resolveParent(disk, inodeTable, arg1, name, sizeof(name));
            if (parent != -1) {
                displayFile(disk, inodeTable, parent, name);
            }
            return 1;
        }

        if (strcmp(command, "meta") == 0) {
            inodeIndex = resolvePath(disk, inodeTable, arg1);
            if (inodeIndex == -1) {
                printf("Erro: caminho nao encontrado.\n");
            } else {
                printFileMetadata(inodeTable, inodeIndex);
            }
            return 1;
        }

        parent = resolveParent(disk, inodeTable, arg1, name, sizeof(name));
        if (parent == -1) {
            return 1;
        }

        if (strcmp(command, "mkdir") == 0) {
            createDirectory(disk, inodeTable, parent, name);
        } else if (strcmp(command, "rmdir") == 0) {
            deleteDirectory(disk, inodeTable, parent, name);
        } else if (strcmp(command, "touch") == 0) {
            createFile(disk, inodeTable, parent, name);
        } else if (strcmp(command, "rm") == 0) {
            deleteFile(disk, inodeTable, parent, name);
        }
        return 1;
    }

    if (strcmp(command, "write") == 0) {
        char name[TAM_NOME];
        int parent;
        char *text;

        arg1 = strtok(NULL, " \t");
        text = strtok(NULL, "");

        if (arg1 == NULL || text == NULL) {
            printf("Uso: write <caminho> <texto>\n");
            return 1;
        }

        removeOuterQuotes(text);
        parent = resolveParent(disk, inodeTable, arg1, name, sizeof(name));
        if (parent != -1) {
            writeFile(disk, inodeTable, parent, name, text, (uint32_t)strlen(text));
        }
        return 1;
    }

    if (strcmp(command, "rename") == 0) {
        char name[TAM_NOME];
        int parent;
        int target;

        arg1 = strtok(NULL, " \t");
        arg2 = strtok(NULL, " \t");
        if (arg1 == NULL || arg2 == NULL) {
            printf("Uso: rename <caminho> <novo_nome>\n");
            return 1;
        }

        parent = resolveParent(disk, inodeTable, arg1, name, sizeof(name));
        if (parent == -1) {
            return 1;
        }

        target = searchEntry(disk, inodeTable, parent, name);
        if (target == -1) {
            printf("Erro: caminho nao encontrado.\n");
        } else if (inodeTable[target].type == DIRECTORY) {
            renameDirectory(disk, inodeTable, parent, name, arg2);
        } else {
            renameFile(disk, inodeTable, parent, name, arg2);
        }
        return 1;
    }

    if (strcmp(command, "mv") == 0) {
        char name[TAM_NOME];
        int sourceParent;
        int destination;

        arg1 = strtok(NULL, " \t");
        arg2 = strtok(NULL, " \t");
        if (arg1 == NULL || arg2 == NULL) {
            printf("Uso: mv <arquivo> <diretorio_destino>\n");
            return 1;
        }

        sourceParent = resolveParent(disk, inodeTable, arg1, name, sizeof(name));
        destination = resolvePath(disk, inodeTable, arg2);
        if (sourceParent == -1 || destination == -1 || inodeTable[destination].type != DIRECTORY) {
            printf("Erro: origem ou destino invalido.\n");
        } else {
            moveFile(disk, inodeTable, sourceParent, destination, name);
        }
        return 1;
    }

    if (strcmp(command, "import") == 0) {
        char name[TAM_NOME];
        int parent;

        arg1 = strtok(NULL, " \t");
        arg2 = strtok(NULL, " \t");
        if (arg1 == NULL || arg2 == NULL) {
            printf("Uso: import <arquivo_real> <caminho_simulado>\n");
            return 1;
        }

        parent = resolveParent(disk, inodeTable, arg2, name, sizeof(name));
        if (parent != -1) {
            importRealFile(disk, inodeTable, parent, arg1, name);
        }
        return 1;
    }

    printf("Comando desconhecido: %s\n", command);
    if (interactive) {
        printf("Digite 'help' para ver os comandos.\n");
    }
    return 1;
}

static int runInterface(const char *diskPath, FILE *input, int interactive, int verboseInitial) {
    VirtualDisk disk;
    Inode inodeTable[MAX_INODES];
    char line[INPUT_SIZE];
    int verbose = verboseInitial;

    if (openVirtualDisk(&disk, diskPath) != OPERATION_OK) {
        printf("Erro ao abrir disco simulado '%s'. Crie primeiro com: ./tp2_so init %s <tamanho_disco> <tamanho_bloco>\n", diskPath, diskPath);
        return 1;
    }

    if (initRoot(&disk, inodeTable) == -1) {
        printf("Erro ao inicializar diretorio raiz. Tente usar tamanho de bloco maior, como 1024.\n");
        closeVirtualDisk(&disk);
        return 1;
    }

    printBanner(diskPath, interactive, verbose);

    while (1) {
        if (interactive) {
            printf(C_BOLD C_BLUE "tp2fs" C_RESET C_CYAN "> " C_RESET);
            fflush(stdout);
        }

        if (fgets(line, sizeof(line), input) == NULL) {
            break;
        }

        if (!interactive) {
            char displayLine[INPUT_SIZE];

            strncpy(displayLine, line, sizeof(displayLine) - 1);
            displayLine[sizeof(displayLine) - 1] = '\0';
            trimLine(displayLine);

            if (displayLine[0] != '\0' && displayLine[0] != '#') {
                printf("\n" C_CYAN "--------------------------------------------------\n" C_RESET);
                printf(C_BOLD C_BLUE "tp2fs" C_RESET C_CYAN "> " C_RESET "%s\n", displayLine);
            }
        }

        if (!executeCommand(&disk, inodeTable, line, &verbose, interactive)) {
            break;
        }
    }

    closeVirtualDisk(&disk);
    return 0;
}

int runInteractiveMode(const char *diskPath, int verboseInitial) {
    return runInterface(diskPath, stdin, 1, verboseInitial);
}

int runBatchMode(const char *diskPath, const char *commandsPath, int verboseInitial) {
    FILE *commands = fopen(commandsPath, "r");
    int result;

    if (commands == NULL) {
        printf(C_RED "ERRO: nao foi possivel abrir o arquivo de comandos '%s'.\n" C_RESET, commandsPath);
        return 1;
    }

    printf("\n" C_BOLD C_CYAN "========== Execucao por arquivo ==========" C_RESET "\n");
    printf("Arquivo de comandos: %s\n", commandsPath);
    printf("Disco virtual:       %s\n", diskPath);
    printf("Modo narrador:        %s\n", verboseInitial ? "ligado" : "desligado");
    printf(C_CYAN "==========================================\n" C_RESET);

    result = runInterface(diskPath, commands, 0, verboseInitial);

    fclose(commands);
    return result;
}
