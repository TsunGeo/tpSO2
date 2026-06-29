#include "diretorio.h"

/* FUNÇÕES AUXILIARES */

static int maxDirEntries(VirtualDisk *disk){
    // Para simplificação vamos usar apenas 1 bloco por diretório
    return disk->header.blockSize / sizeof(DirectoryEntry);
}

static void initEntries(DirectoryEntry entries[], int maxEntries){
    // Inicializa todas as entradas do diretório como vazias
    for(int i=0; i<maxEntries; i++){
        entries[i].name[0] = '\0';
        entries[i].inode = -1;
        entries[i].type = DIRECTORY;
        entries[i].used = 0;
    }
}

int loadDirectoryEntry(VirtualDisk *disk, Inode inodeTable[], int inodeDir, DirectoryEntry entries[]){
    // Carrega as entradas do diretório do disco para a memória
    Inode *inode = &inodeTable[inodeDir];
    int maxEntries = maxDirEntries(disk);

    initEntries(entries, maxEntries);

    if(inode->quantBlocks == 0){
        // Diretório vazio, não há blocos alocados
        return 1;
    }

    if(readBlock(disk, inode->blocks[0], entries, disk->header.blockSize) != OPERATION_OK){
        return 0;
    }

    time(&inode->accessDate);
    return 1;
    // Retorna 1 se conseguiu carregar, 0 caso contrário
}

int saveDirectory(VirtualDisk *disk, Inode inodeTable[], int inodeDir, DirectoryEntry entries[]){
    // Salva as entradas do diretório da memória para o disco
    // é usada sempre que uma entrada é adicionada, removida ou renomeada, pois muda o conteúdo do diretório
    Inode *inode = &inodeTable[inodeDir];

    if(inode->quantBlocks == 0){
        // Se não houver blocos alocados para o diretório, aloca um novo bloco
        uint32_t newBlck;

        if(allocateBlock(disk, &newBlck) != OPERATION_OK){
            return 0;
        }

        if(!addBlockToInode(inodeTable, inodeDir, newBlck)){
            freeBlock(disk, newBlck);
            return 0;
        }
    }

    if(writeBlock(disk, inode->blocks[0], entries, disk->header.blockSize) != OPERATION_OK){
        return 0;
    }

    // Atualiza o tamanho do diretório e a data de modificação
    inode->size = disk->header.blockSize;
    time(&inode->modificationDate);

    return 1;
}

int searchEntry(VirtualDisk *disk, Inode inodeTable[], int inodeDir, char *name){
    // Procurar uma entrada pelo nome no diretório especificado pelo inodeDir
    int maxEntries = maxDirEntries(disk);
    DirectoryEntry *entries = malloc(disk->header.blockSize);

    if(entries == NULL){
        return -1;
    }

    if(!loadDirectoryEntry(disk, inodeTable, inodeDir, entries)){
        // Falha ao carregar o diretório para a memória
        free(entries);
        return -1;
    }

    for(int i=0; i<maxEntries; i++){
        // Verifica se a entrada está em uso e se o nome corresponde
        // caso encontre, retorna o inode associado a essa entrada
        if(entries[i].used && strcmp(entries[i].name, name) == 0){
            int inodeFound = entries[i].inode;
            free(entries);
            return inodeFound;
        }
    }

    free(entries);
    return -1;
}

int addEntry(VirtualDisk *disk, Inode inodeTable[], int inodeDir, char *name, int newInode, FileType type){
    // Adiciona uma nova entrada ao diretório especificado pelo inodeDir
    int maxEntries = maxDirEntries(disk);
    DirectoryEntry *entries = malloc(disk->header.blockSize);

    if(entries == NULL){
        return 0;
    }

    if(!loadDirectoryEntry(disk, inodeTable, inodeDir, entries)){
        free(entries);
        return 0;
    }

    for(int i=0; i<maxEntries; i++){
        if(entries[i].used && strcmp(entries[i].name, name) == 0){
            free(entries);
            return 0;
        }
    }

    // Procura uma posição vazia para adicionar a nova entrada
    for(int i=0; i<maxEntries; i++){
        if(!entries[i].used){
            strncpy(entries[i].name, name, TAM_NOME-1);
            entries[i].name[TAM_NOME-1] = '\0'; // Garantir terminação nula
            entries[i].inode = newInode;
            entries[i].type = type;
            entries[i].used = 1;

            // Salva as alterações no diretório de volta para o disco
            int result = saveDirectory(disk, inodeTable, inodeDir, entries);

            free(entries);
            return result;
        }
    }

    free(entries);
    return 0;
}

int removeEntry(VirtualDisk *disk, Inode inodeTable[], int inodeDir, char *name){
    // Remove uma entrada do diretório especificado pelo inodeDir
    int maxEntries = maxDirEntries(disk);
    DirectoryEntry *entries = malloc(disk->header.blockSize);

    if(entries == NULL){
        return 0;
    }

    if(!loadDirectoryEntry(disk, inodeTable, inodeDir, entries)){
        free(entries);
        return 0;
    }

    for(int i=0; i<maxEntries; i++){
        if(entries[i].used && strcmp(entries[i].name, name) == 0){
            entries[i].used = 0; // Marca a entrada como vazia
            entries[i].name[0] = '\0'; // Limpa o nome
            entries[i].inode = -1; // Limpa o inode

            int result = saveDirectory(disk, inodeTable, inodeDir, entries);

            free(entries);
            return result;
        }
    }

    free(entries);
    return 0; // Entrada não encontrada
}

static int emptyDirectory(VirtualDisk *disk, Inode inodeTable[], int inodeDir){
    // verifica se o diretório especificado pelo inodeDir está vazio (apenas "." e ".." presentes)
    // logo, se pode ser removido
    int maxEntries = maxDirEntries(disk);
    DirectoryEntry *entries = malloc(disk->header.blockSize);

    if(entries == NULL){
        return 0;
    }

    if(!loadDirectoryEntry(disk, inodeTable, inodeDir, entries)){
        free(entries);
        return 0;
    }

    for(int i=0; i<maxEntries; i++){
        if(entries[i].used){
            if(strcmp(entries[i].name, ".") != 0 && strcmp(entries[i].name, "..") != 0){
                free(entries);
                return 0; // Encontrou uma entrada que não é "." ou ".."
            }
        }
    }

    free(entries);
    return 1; // Diretório está vazio
}

/* FUÇÕES PRINCIPAIS */

void createDirectory(VirtualDisk *disk, Inode inodeTable[], int inodeParent, char *name){
    if(disk == NULL || inodeTable == NULL || name == NULL){
        printf("Erro: parametros invalidos\n");
        return;
    }

    if(inodeTable[inodeParent].type != DIRECTORY){
        printf("Erro: inode pai nao e um diretorio\n");
        return;
    }

    if(searchEntry(disk, inodeTable, inodeParent, name) != -1){
        printf("Erro: ja existe uma entrada com o nome '%s' no diretorio\n", name);
        return;
    }

    int newInode = allocInode(inodeTable, DIRECTORY);

    if(newInode == -1){
        printf("Erro: nao foi possivel alocar um novo inode\n");
        return;
    }

    if(!addEntry(disk, inodeTable, newInode, ".", newInode, DIRECTORY)){
        printf("Erro: nao foi possivel adicionar a entrada '.'\n");
        freeInode(inodeTable, disk, newInode);
        return;
    }

    if(!addEntry(disk, inodeTable, newInode, "..", inodeParent, DIRECTORY)){
        printf("Erro: nao foi possivel adicionar a entrada '..'\n");
        freeInode(inodeTable, disk, newInode);
        return;
    }

    if(!addEntry(disk, inodeTable, inodeParent, name, newInode, DIRECTORY)){
        printf("Erro: nao foi possivel adicionar a entrada '%s'\n", name);
        freeInode(inodeTable, disk, newInode);
        return;
    }

    printf("Diretorio '%s' criado com sucesso\n", name);
}

void deleteDirectory(VirtualDisk *disk, Inode inodeTable[], int inodeParent, char *name){
    if(disk == NULL || inodeTable == NULL || name == NULL){
        printf("Erro: parametros invalidos\n");
        return;
    }

    if(inodeTable[inodeParent].type != DIRECTORY){
        printf("Erro: inode pai nao e um diretorio\n");
        return;
    }

    int inodeRemove = searchEntry(disk, inodeTable, inodeParent, name);

    if(inodeRemove == -1){
        printf("Erro: diretorio '%s' nao encontrado no diretorio pai\n", name);
        return;
    }

    if(inodeTable[inodeRemove].type != DIRECTORY){
        printf("Erro: a entrada '%s' nao e um diretorio\n", name);
        return;
    }

    if(!emptyDirectory(disk, inodeTable, inodeRemove)){
        printf("Erro: diretorio '%s' nao esta vazio\n", name);
        return;
    }

    if(!removeEntry(disk, inodeTable, inodeParent, name)){
        printf("Erro: falha ao remover a entrada '%s' do diretorio pai\n", name);
        return;
    }

    freeInode(inodeTable, disk, inodeRemove);
    printf("Diretorio '%s' apagado com sucesso\n", name);
}

void renameDirectory(VirtualDisk *disk, Inode inodeTable[],  int inodeParent, char *oldName, char *newName){
    if(disk == NULL || inodeTable == NULL || oldName == NULL || newName == NULL){
        printf("Erro: parametros invalidos\n");
        return;
    }

    if(inodeTable[inodeParent].type != DIRECTORY){
        printf("Erro: inode pai nao e um diretorio\n");
        return;
    }

    if(searchEntry(disk, inodeTable, inodeParent, newName) != -1){
        printf("Erro: ja existe uma entrada com o nome '%s' no diretorio\n", newName);
        return;
    }

    int maxEntries = maxDirEntries(disk);
    DirectoryEntry *entries = malloc(disk->header.blockSize);

    if(entries == NULL){
        printf("Erro: falha ao alocar memoria para entries do diretorio\n");
        return;
    }

    if(!loadDirectoryEntry(disk, inodeTable, inodeParent, entries)){
        printf("Erro: falha ao carregar o diretorio\n");
        free(entries);
        return;
    }

    for(int i=0; i<maxEntries; i++){
        if(entries[i].used && strcmp(entries[i].name, oldName) == 0){
            if(entries[i].type != DIRECTORY){
                printf("Erro: a entrada '%s' nao e um diretorio\n", oldName);
                free(entries);
                return;
            }

            strncpy(entries[i].name, newName, TAM_NOME-1);
            entries[i].name[TAM_NOME-1] = '\0';

            if(!saveDirectory(disk, inodeTable, inodeParent, entries)){
                printf("Erro: falha ao salvar o diretorio apos renomear\n");
                free(entries);
                return;
            }

            printf("Diretorio '%s' renomeado para '%s' com sucesso\n", oldName, newName);
            free(entries);
            return;
        }
    }

    printf("Erro: diretorio '%s' nao encontrado no diretorio pai\n", oldName);
    free(entries);
}

void listDirectoryContent(VirtualDisk *disk, Inode inodeTable[], int inodeDir){
    if(disk == NULL || inodeTable == NULL){
        printf("Erro: parametros invalidos\n");
        return;
    }

    if(inodeTable[inodeDir].type != DIRECTORY){
        printf("Erro: inode nao e um diretorio\n");
        return;
    }

    int maxEntries = maxDirEntries(disk);
    DirectoryEntry *entries = malloc(disk->header.blockSize);

    if(entries == NULL){
        printf("Erro: falha ao alocar memoria para entries do diretorio\n");
        return;
    }

    if(!loadDirectoryEntry(disk, inodeTable, inodeDir, entries)){
        printf("Erro: falha ao carregar o diretorio\n");
        free(entries);
        return;
    }

    printf("Conteudo do diretorio (inode %d):\n", inodeDir);
    for(int i=0; i<maxEntries; i++){
        if(entries[i].used){
            printf("Nome: %s | Inode: %d | Tipo: %s\n", entries[i].name, entries[i].inode,
                   (entries[i].type == DIRECTORY) ? "Diretorio" : "Arquivo");
        }
    }

    free(entries);
}
