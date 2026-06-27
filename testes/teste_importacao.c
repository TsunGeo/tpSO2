#include "../src/importacao/importacao.h"
#include "../src/arquivo/arquivo.h"
#include "../src/diretorio/diretorio.h"
#include "../src/i-node/inode.h"

void testeImportacao(VirtualDisk *disk, Inode inodeTable[], int rootInode) {
    printf("\n========== TESTE DE IMPORTACAO DE ARQUIVO REAL ==========\n");

    /* 1. Prepara o arquivo real temporário */
    const char *realPath = "arquivo_real_teste.txt";
    FILE *f = fopen(realPath, "w");
    if (f == NULL) {
        printf("Erro: nao foi possivel criar o arquivo temporario de teste.\n");
        return;
    }
    fprintf(f, "Este e o conteudo do arquivo real que foi importado com sucesso para o disco virtual!");
    fclose(f);

    /* 2. Cria a pasta de destino dentro do disco virtual */
    createDirectory(disk, inodeTable, rootInode, "backup_import");
    int backupInode = searchEntry(disk, inodeTable, rootInode, "backup_import");
    if (backupInode == -1) {
        printf("Erro: nao foi possivel criar o diretorio de destino.\n");
        remove(realPath);
        return;
    }

    /* 3. Importa */
    printf("Importando '%s' para a pasta 'backup_import' simulada...\n", realPath);
    if (importRealFile(disk, inodeTable, backupInode, realPath, "importado.txt") != OPERATION_OK) {
        printf("Erro: importacao falhou.\n");
        remove(realPath);
        return;
    }

    /* 4. Exibe conteúdo e metadados */
    int importedInode = searchEntry(disk, inodeTable, backupInode, "importado.txt");
    if (importedInode != -1) {
        displayFile(disk, inodeTable, backupInode, "importado.txt");
        printFileMetadata(inodeTable, importedInode);
    }

    /* 5. Limpa o arquivo temporário do SO real */
    remove(realPath);

    printf("========== FIM DO TESTE DE IMPORTACAO ==========\n");
}