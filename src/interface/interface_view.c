#include "interface_view.h"

#include <stdio.h>

void printErrorMsg(const char *message) {
    printf(C_RED "ERRO: %s\n" C_RESET, message);
}

void printInterfaceHelp(void) {
    printf("\n" C_BOLD C_CYAN "================ TP2FS - Comandos ================\n" C_RESET);

    printf(C_BOLD "\nSistema:\n" C_RESET);
    printf("  help                              Mostra esta ajuda\n");
    printf("  info                              Mostra informacoes do disco simulado\n");
    printf("  verbose on|off                    Liga ou desliga modo verboso\n");
    printf("  exit                              Encerra modo interativo\n");

    printf(C_BOLD "\nDiretorios:\n" C_RESET);
    printf("  mkdir <caminho>                   Cria diretorio. Ex.: mkdir /docs\n");
    printf("  rmdir <caminho>                   Remove diretorio vazio\n");
    printf("  ls [caminho]                      Lista diretorio. Ex.: ls /docs\n");
    printf("  rename <caminho> <novo_nome>      Renomeia arquivo ou diretorio\n");

    printf(C_BOLD "\nArquivos:\n" C_RESET);
    printf("  touch <caminho>                   Cria arquivo vazio\n");
    printf("  write <caminho> <texto>           Escreve texto no arquivo\n");
    printf("  cat <caminho>                     Exibe conteudo do arquivo\n");
    printf("  rm <caminho>                      Remove arquivo\n");
    printf("  mv <arquivo> <diretorio_destino>  Move arquivo para outro diretorio\n");
    printf("  import <real> <simulado>          Importa arquivo real para o simulador\n");
    printf("  meta <caminho>                    Exibe metadados do i-node\n");

    printf(C_BOLD "\nExecucao por arquivo:\n" C_RESET);
    printf("  ./tp2_so batch <arquivo_disco> <arquivo_comandos> [-v]\n");

    printf(C_CYAN "==================================================\n\n" C_RESET);
}

void printBanner(const char *diskPath, int interactive, int verbose) {
    printf("\n" C_BOLD C_CYAN "==================================================\n" C_RESET);
    printf(C_BOLD "        TP2FS - Simulador de Sistema de Arquivos\n" C_RESET);
    printf(C_CYAN "==================================================\n" C_RESET);
    printf(" Disco virtual: %s\n", diskPath);
    printf(" Modo: %s\n", interactive ? "interativo" : "arquivo de entrada");
    printf(" Verboso: %s\n", verbose ? "ligado" : "desligado");
    printf("\nDigite 'help' para ver os comandos.\n");
    printf(C_CYAN "==================================================\n\n" C_RESET);
}

void printPrettyDiskInfo(VirtualDisk *disk) {
    if (disk == NULL) {
        printErrorMsg("disco invalido.");
        return;
    }

    printf("\n" C_BOLD C_CYAN "========== Informacoes do Disco ==========" C_RESET "\n\n");
    printf("Tamanho total:           %u bytes\n", disk->header.diskSize);
    printf("Tamanho do bloco:        %u bytes\n", disk->header.blockSize);
    printf("Total de blocos dados:   %u\n", disk->header.totalBlocks);
    printf("Blocos livres:           %u\n", disk->header.freeBlocks);
    printf("Blocos usados:           %u\n", disk->header.totalBlocks - disk->header.freeBlocks);

    printf("\nMapa interno:\n");
    printf("Inicio do bitmap:        byte %u\n", disk->header.bitmapStart);
    printf("Inicio da area de dados: byte %u\n", disk->header.dataStart);

    printf("\n" C_CYAN "==========================================\n\n" C_RESET);
}