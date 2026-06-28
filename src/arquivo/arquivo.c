#include "arquivo.h"
#include "../diretorio/diretorio.h"

static int maxDirEntries(VirtualDisk *disk){
    return disk->header.blockSize / sizeof(DirectoryEntry);
}

void createFile(VirtualDisk *disk, Inode inodeTable[], int parentInode, char *name){
    if(disk == NULL || inodeTable == NULL || name == NULL){
        printf("Erro: Parametros invalidos\n");
        return;
    }

    //Confere se o diretorio pai realmente é um diretorio
    if (inodeTable[parentInode].type != DIRECTORY){
        printf("Erro: Diretorio invalido\n");
        return;
    }

    /* Verifica se já existe um arquivo/diretório com esse nome */
    if (searchEntry(disk, inodeTable, parentInode, name) != -1){
        printf("Erro: '%s' ja exite\n", name);
        return;
    }
    /* Aloca um novo inode */
    int newInode = allocInode(inodeTable, TYPE_FILE);

    if (newInode == -1){
        printf("Erro: Nao ha inode livre\n");
        return;
    }
    
    /* Adiciona o arquivo ao diretório pai */
    if (!addEntry(disk, inodeTable, parentInode, name, newInode,TYPE_FILE))
    {
        printf("Erro: Nao foi possivel criar o arquivo '%s'\n", name);
        freeInode(inodeTable, disk, newInode);
        return;
    }

    printf("Arquivo '%s' criado com sucesso\n", name);
} 

void deleteFile(VirtualDisk *disk, Inode inodeTable[], int parentInode, char *name){
    if (disk == NULL || inodeTable == NULL || name == NULL){
        printf("Erro: parametros invalidos\n");
        return;
    }

    if (inodeTable[parentInode].type != DIRECTORY){
        printf("Erro: inode pai nao e um diretorio\n");
        return;
    }

    int inodeRemove = searchEntry(disk, inodeTable, parentInode, name);

    if (inodeRemove == -1) {
        printf("Erro: arquivo '%s' nao encontrado\n", name);
        return;
    }

    if (inodeTable[inodeRemove].type != TYPE_FILE){
        printf("Erro: '%s' nao e um arquivo\n", name);
        return;
    }
    /* Libera todos os blocos utilizados pelo arquivo */
    for (int i = 0; i < inodeTable[inodeRemove].quantBlocks; i++) {

        if (inodeTable[inodeRemove].blocks[i] != (uint32_t)-1) {

            if (freeBlock(disk,inodeTable[inodeRemove].blocks[i]) != OPERATION_OK)
            {
                printf("Aviso: nao foi possivel liberar o bloco %u\n",
                       inodeTable[inodeRemove].blocks[i]);
            }
            inodeTable[inodeRemove].blocks[i] = (uint32_t)-1;
        }
    }

    inodeTable[inodeRemove].quantBlocks = 0;
    inodeTable[inodeRemove].size = 0;

    if (!removeEntry(disk, inodeTable, parentInode, name)) {
        printf("Erro: nao foi possivel remover a entrada do diretorio\n");
        return;
    }

    freeInode(inodeTable, disk, inodeRemove);

    printf("Arquivo '%s' removido com sucesso\n", name);
}

void renameFile(VirtualDisk *disk, Inode inodeTable[], int parentInode, char *oldName, char *newName){
    if (disk == NULL || inodeTable == NULL || oldName == NULL || newName == NULL) {
        printf("Erro: parametros invalidos\n");
        return;
    }

    if (inodeTable[parentInode].type != DIRECTORY){ 
        printf("Erro: inode pai nao e um diretorio\n");
        return;
    }

    /* Verifica se o novo nome já existe */
    if (searchEntry(disk, inodeTable, parentInode, newName) != -1) {
        printf("Erro: ja existe um arquivo ou diretorio chamado '%s'\n", newName);
        return;
    }

    DirectoryEntry *entries = malloc(disk->header.blockSize);

    if (entries == NULL){
        printf("Erro: falha ao alocar memoria\n");
        return;
    }

    if (!loadDirectoryEntry(disk, inodeTable, parentInode, entries)) {
        printf("Erro: falha ao carregar o diretorio\n");
        free(entries);
        return;
    }
    int maxEntries = maxDirEntries(disk);
    for (int i = 0; i < maxEntries; i++) {

        if (entries[i].used && strcmp(entries[i].name, oldName) == 0) {

            if (entries[i].type != TYPE_FILE) {
                printf("Erro: '%s' nao e um arquivo\n", oldName);
                free(entries);
                return;
            }

            strncpy(entries[i].name, newName, TAM_NOME - 1);
            entries[i].name[TAM_NOME - 1] = '\0';

            if (!saveDirectory(disk, inodeTable, parentInode, entries)) {
                printf("Erro: nao foi possivel salvar o diretorio\n");
                free(entries);
                return;
            }

            printf("Arquivo '%s' renomeado para '%s' com sucesso\n", oldName, newName);

            free(entries);
            return;
        }
    }

    printf("Erro: arquivo '%s' nao encontrado.\n", oldName);
    free(entries);
}

void moveFile(VirtualDisk *disk, Inode inodeTable[], int sourceDir, int destinationDir, char *name){
    if (disk == NULL || inodeTable == NULL || name == NULL){
        printf("Erro: parametros invalidos\n");
        return;
    }

    if (inodeTable[sourceDir].type != DIRECTORY || inodeTable[destinationDir].type != DIRECTORY){ 
        printf("Erro: um dos inodes informados nao e um diretorio\n");
        return;
    }

    int inodeFile = searchEntry(disk, inodeTable, sourceDir, name);

    if (inodeFile == -1){ 
        printf("Erro: arquivo '%s' nao encontrado\n", name);
        return;
    }

    if (inodeTable[inodeFile].type != TYPE_FILE){
        printf("Erro: '%s' nao e um arquivo\n", name);
        return;
    }

    if (searchEntry(disk, inodeTable, destinationDir, name) != -1){ 
        printf("Erro: ja existe um arquivo com esse nome no diretorio de destino\n");
        return;
    }

    if (!addEntry(disk, inodeTable, destinationDir, name, inodeFile, TYPE_FILE)) {
        printf("Erro: nao foi possivel adicionar o arquivo ao diretorio de destino\n");
        return;
    }

    if (!removeEntry(disk, inodeTable, sourceDir, name)) {
        printf("Erro: nao foi possivel remover o arquivo do diretorio de origem\n");
        removeEntry(disk, inodeTable, destinationDir, name);
        return;
    }

    time(&inodeTable[inodeFile].modificationDate);

    printf("Arquivo '%s' movido com sucesso\n", name);
}

void writeFile(VirtualDisk *disk, Inode inodeTable[], int parentInode, char *name, const void *buffer, uint32_t size){
    if (disk == NULL || inodeTable == NULL || name == NULL || buffer == NULL){
        printf("Erro: parametros invalidos\n");
        return;
    }

    if (inodeTable[parentInode].type != DIRECTORY){
        printf("Erro: inode pai nao e um diretorio\n");
        return;
    }

    int inodeFile = searchEntry(disk, inodeTable, parentInode, name);

    if (inodeFile == -1){ 
        printf("Erro: arquivo '%s' nao encontrado\n", name);
        return;
    }

    if (inodeTable[inodeFile].type != TYPE_FILE){ 
        printf("Erro: '%s' nao e um arquivo\n", name);
        return;
    }

    /* Libera blocos antigos caso o arquivo já possua conteúdo */
    for (int i = 0; i < inodeTable[inodeFile].quantBlocks; i++) {

        if (inodeTable[inodeFile].blocks[i] != (uint32_t)-1) {
            freeBlock(disk, inodeTable[inodeFile].blocks[i]);
            inodeTable[inodeFile].blocks[i] = (uint32_t)-1;
        }
    }

    inodeTable[inodeFile].quantBlocks = 0;

    if (iNodeWriteData(disk, inodeTable, inodeFile, buffer, size)) {
        printf("Dados gravados com sucesso no arquivo '%s'\n", name);
    } else {
        printf("Erro: nao foi possivel gravar os dados no arquivo '%s'\n", name);
    }
}

void readFile(VirtualDisk *disk, Inode inodeTable[], int parentInode, char *name, void *buffer){
    if (disk == NULL || inodeTable == NULL || name == NULL || buffer == NULL){
        printf("Erro: parametros invalidos\n");
        return;
    }

    if (inodeTable[parentInode].type != DIRECTORY){
        printf("Erro: inode pai nao e um diretorio\n");
        return;
    }

    int inodeFile = searchEntry(disk, inodeTable,parentInode, name);

    if (inodeFile == -1) {
        printf("Erro: arquivo '%s' nao encontrado\n", name);
        return;
    }

    if (inodeTable[inodeFile].type != TYPE_FILE) {
        printf("Erro: '%s' nao e um arquivo\n", name);
        return;
    }

    iNodeReadData(disk, inodeTable, inodeFile, buffer);
    printf("Arquivo '%s' lido com sucesso\n", name);
}

void displayFile(VirtualDisk *disk, Inode inodeTable[], int parentInode, char *name){
    if (disk == NULL || inodeTable == NULL || name == NULL) {
        printf("Erro: parametros invalidos\n");
        return;
    }

    int inodeFile = searchEntry(disk, inodeTable, parentInode, name);

    if (inodeFile == -1) {
        printf("Erro: arquivo '%s' nao encontrado\n", name);
        return;
    }

    if (inodeTable[inodeFile].type != TYPE_FILE) {
        printf("Erro: '%s' nao e um arquivo\n", name);
        return;
    }

    char *buffer = malloc(inodeTable[inodeFile].size + 1);

    if (buffer == NULL){
        printf("Erro: falha ao alocar memoria\n");
        return;
    }
    
    iNodeReadData(disk, inodeTable, inodeFile, buffer);

    buffer[inodeTable[inodeFile].size] = '\0';

    printf("\n===== Conteudo do arquivo =====\n");
    printf("%s\n", buffer);
    printf("===============================\n");

    free(buffer);
}