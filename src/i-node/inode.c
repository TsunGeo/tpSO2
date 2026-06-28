#include "./inode.h"
#include <string.h>

/**
 * @param iNodeTable Ponteiro para tabela de i-nodes.
 */
Inode* initializeInode(Inode* iNodeTable){
    iNodeTable = (Inode*)malloc(sizeof(Inode) * MAX_INODES);

    for(int i = 0; i < MAX_INODES; i++){
        iNodeTable[i].iNodeID = i+1;
        iNodeTable[i].isBeingUsed = 0;
        iNodeTable[i].quantBlocks = 0;
        
        for(int j = 0; j < DIRECT_POINTERS; j++){
            iNodeTable[i].blocks[j] = (uint32_t)-1; // Como o i-node ainda não está referenciando nenhum arquivo, todos os ponteiros diretos dele são definidos como -1 
                                                    // para melhor segurança do sistema, a medida que forem alocados blocos pelo i-node, eles serão referenciados em 
                                                    // cada posição do vetor.
        }
    }

    return iNodeTable;
}

/**
 * 
 * @param iNodeTable Tabela de i-nodes que abrigará o i-node alocado
 * @param FileType type of the iNode TODO: describe it better
 * 
 * @returns Index of the i-node alocated in the i-node Table
 */
int allocInode(Inode iNodeTable[], FileType type){ //FIXME: Check if the vector parameter is equal to pass the pointer of the first position
    for(int i = 0; i < MAX_INODES; i++){
        if(!iNodeTable[i].isBeingUsed){ // Not being used
            iNodeTable[i].iNodeID = i; // MUDANÇA
            iNodeTable[i].isBeingUsed = 1; //Now is being used


            iNodeTable[i].size = 0;
            iNodeTable[i].type = type;

            iNodeTable[i].quantBlocks = 0; // MUDANÇA
            for(int j = 0; j < DIRECT_POINTERS; j++){
                iNodeTable[i].blocks[j] = (uint32_t)-1; // MUDANÇA
            }

            time(&iNodeTable[i].creationDate);
            iNodeTable[i].modificationDate = iNodeTable[i].creationDate; // MUDANÇA
            iNodeTable[i].accessDate = iNodeTable[i].creationDate; // MUDANÇA

            return i;
        }
    }

    return -1; // there's no free i-node in the entire i-node table
}

/**
 * 
 * @param iNodeTable Tabela de i-nodes a ser acessada
 * @param iNodeIndex Índice do i-node a qual o bloco será alocado
 * @param blockIndex Índice do bloco a ser alocado
 */
int addBlockToInode(Inode iNodeTable[], int iNodeIndex, uint32_t blockIndex){
    Inode* inode = &iNodeTable[iNodeIndex]; // Ponteiro ajustado para o endereço do i-node que quero acessar

    if(inode->quantBlocks >= DIRECT_POINTERS){
        return 0;
    }

    inode->blocks[inode->quantBlocks] = blockIndex;
    time(&inode->modificationDate); //FIXME: possível problema com lógica de ponteiros

    inode->quantBlocks++;

    return 1;
}

/**
 * @param iNodeTable Tabela de i-nodes
 * @param iNodeIndex Índice do i-node
 * @param blockIndex Índice do bloco que será removido
 * 
 * @returns 1 case success;
 * @returns 0 case does not found block.
 */
int removeBlockFromInode(Inode iNodeTable[], int iNodeIndex, uint32_t blockIndex){
    Inode* inode = &iNodeTable[iNodeIndex]; // Ponteiro ajustado para o endereço do i-node que quero acessar

    for(int i = 0; i < inode->quantBlocks; i++){
        if(inode->blocks[i] == blockIndex){
            for(int j = i; j < inode->quantBlocks - 1; j++){
                inode->blocks[j] = inode->blocks[j + 1]; // Arrasta os blocos uma posição para trás a cada iteração.
            }

            time(&inode->modificationDate);

            inode->quantBlocks--;

            return 1; // success
        }
    }

    return 0; // Block not found
}

/**
 * @param iNodeTable Tabela de i-nodes
 * @param iNodeIndex Índice do i-node que será acessado
 * @param directPointerIndex Índice do ponteiro direto do i-node para o bloco a ser acessado
 * @param blockIndex Índice do bloco dentro do i-node que será acessado
 * 
 * @returns
 * @returns
 */
int getBlockFromInode(Inode iNodeTable[], int iNodeIndex, int directPointerIndex, uint32_t *blockIndex){
    Inode* inode = &iNodeTable[iNodeIndex]; // Ponteiro ajustado para o endereço do i-node que quero acessar

    if(directPointerIndex < 0 || directPointerIndex >= inode->quantBlocks){ // Valor inválido 
        return 0;
    }

    *blockIndex = inode->blocks[directPointerIndex];
    time(&inode->accessDate);

    return 1;
}

/**
 * @param disk Disco virtual que será acessado
 * @param iNodetable Tabela de i-nodes
 * @param iNodeIndex Índice do i-node
 * @param buffer Buffer onde os dados serão escritos
 * 
 * @returns 1 caso a informação passada seja escrita corretamente nos blocos;
 * @returns 0 caso a operação falhe.
 */
int iNodeReadData(VirtualDisk* disk, Inode iNodeTable[], int iNodeIndex, void* buffer){
    Inode* inode = &iNodeTable[iNodeIndex]; // Ponteiro ajustado para o endereço do i-node que quero acessar

    char* data = buffer;
    uint32_t offset = 0;

    //Aloca um bloco temporário para receber os dados
    void *tempBlock = malloc(disk->header.blockSize);
    if (tempBlock == NULL) {
        printf("Erro: falha ao alocar memoria temporaria para leitura de bloco\n");
        return 0;
    }

    /**
     * A cada iteração a função percorre os blocos no disco onde o dado está salvo, lendo-o completamente, a menos que a quantidade de bytes restante
     * não ultrapasse o tamanho do bloco. Nesse caso a leitura para assim que a quantidade de bytes se esgota. Dessa forma, o buffer é preenchido corretamente.
     */
    for(int i = 0; i < inode->quantBlocks; i++){
        uint32_t bytesToRead = disk->header.blockSize;

        if(i == inode->quantBlocks - 1){
            uint32_t remaining = inode->size - offset;

            if(remaining < bytesToRead){
                bytesToRead = remaining;
            }
        }

        if(readBlock(disk, inode->blocks[i], (data+offset), bytesToRead) == OPERATION_ERROR){
            return 0;
        }
        offset += bytesToRead;
    }

    //Libera memória alocada
    free(tempBlock);
    time(&inode->accessDate);

    return 1;
}

/**
 * @param disk Disco virtual previamente aberto
 * @param iNodeTable Tabela de i-nodes
 * @param iNodeIndex Índice do i-node que receberá os dados
 * @param buffer Conteúdo que será escrito
 * @param size Quantidade de bytes
 * 
 * @returns 1 caso a informação passada seja escrita corretamente nos blocos;
 * @returns 0 caso a operação falhe.
 */
int iNodeWriteData(VirtualDisk* disk, Inode iNodeTable[], int iNodeIndex, const void *buffer, uint32_t size){
    Inode* inode = &iNodeTable[iNodeIndex]; // Ponteiro ajustado para o endereço do i-node que quero acessar

    const char *data = buffer;
    uint32_t remaining = size;
    uint32_t offset = 0;

    
    /**
     * A cada iteração um bloco de disco cheio é passado, uma vez que sobre menos que um bloco para ser armazenado o valor dessa variavel passa a ser simplesmente a quantidade de bytes restante.
     * 
     * Exemplo:
     * Tamanho do bloco = 1000 bytes
     * Tamanho do arquivo = 2500 bytes
     * 
     * Iteração 1 e 2: bytesToWrite = 1000
     * Iteração 3:     bytesToWrite = 500
     * 
     * 3 i-nodes são alocados, os blocos no disco referenciados por esses i-nodes são preenchidos de forma que os dois primeiros são totalmente preenchidos e o último deixa metade do espaço livre.
     */
    while(remaining > 0){
        uint32_t blockIndex;

        //Aloca Bloco no disco
        if(allocateBlock(disk, &blockIndex) != OPERATION_OK){
            return 0;
        }

        // Atribui um i-node ao bloco
        if(!addBlockToInode(iNodeTable, iNodeIndex, blockIndex)){
            return 0;
        }

        //Variável local declarada unicamente para definir a quantidade de bytes restantes a serem armazenados no disco.
        uint32_t bytesToWrite = remaining > disk->header.blockSize ? disk->header.blockSize : remaining;

        if(writeBlock(disk, blockIndex, (data+offset), bytesToWrite) == OPERATION_ERROR){
            return 0;
        }

        // Atualiza dados de apoio
        remaining -= bytesToWrite;
        offset += bytesToWrite;
    }

    inode->size = size;
    time(&inode->modificationDate);

    return 1;
}

/**
 * @param iNodeTable Ponteiro para a tabela de i-nodes a qual o i-node será modificado
 * @param disk Ponteiro para o disco virtual onde os blocos estão localizados
 * @param index Índice do i-node alvo na tabela de i-nodes
 * 
 * @returns 1 Se o i-node e todos os blocos dentro do i-node tiveram sua referência de memória liberadas;
 * @returns 0 Caso a operação tenha falhado.
 */
int freeInode(Inode iNodeTable[], VirtualDisk* disk, int index){
    iNodeTable[index].isBeingUsed = 0;

    for(int i = 0; i < DIRECT_POINTERS; i++){
        if(freeBlock(disk, (uint32_t)i) == OPERATION_ERROR){
            return 0;
        }
    }

    return 1;
}


/**
 * @param date Data a ser exibida na interface do terminal
 */
static void printDate(time_t date) {
    char buffer[30];

    struct tm *timeInfo = localtime(&date);

    strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", timeInfo);

    printf("%s\n", buffer);
}

/**
 * @param iNode i-node que sera exibido
 */
static void printInode(Inode iNode){
    printf("\tId do i-node:       %d\n", iNode.iNodeID);
    printf("\tTipo do i-node:     ");
    
    if(iNode.type == TYPE_FILE){
        printf("File\n");
        printf("\tTamanho do arquivo: %d bytes\n", iNode.size);
    }
    else{
        printf("Directory.\n");
    }
    
    printf("\tData de criacao:    ");
    printDate(iNode.creationDate);
    
    printf("\tUltima modificacao: ");
    printDate(iNode.modificationDate);
    
    printf("\tUltimo acesso:      ");
    printDate(iNode.accessDate);
    
    printf("\tPonteiros diretos:\n");
    for(int j = 0; j < DIRECT_POINTERS; j++){
        printf("\t\tIndice do bloco %d: \n", iNode.blocks[j]);
    }
}

/**
 * @param iNodeTable Tabela de i-nodes que serão exibidas detalhadamente
 */
void printInodeTableRelatory(Inode iNodeTable[]){
    printf("Lista de i-nodes:\n");
    for(int i = 0; i < MAX_INODES; i++){
        if(!iNodeTable[i].isBeingUsed) continue;
        printInode(iNodeTable[i]);
    }
}

// Ambiente de teste.
// Remova o comentário para executar o teste completo
/*
int main(){
    Inode* iNodeTable = initializeInode(iNodeTable);
    VirtualDisk disk;

    if(createVirtualDisk("teste.disk", 1024 * 1024, 512) == OPERATION_ERROR){
        printf("Erro ao criar disco.\n");
        return 1;
    }

    if(openVirtualDisk(&disk, "teste.disk") != OPERATION_OK){
        printf("Erro ao abrir disco.\n");
        return 1;
    }

    printDiskInfo(&disk);

    printf("\n==============================\n");
    printf("TESTE DO TAD INODE\n");
    printf("==============================\n\n");

    for(int i = 0; i < 10; i++){

        printf("\n=============================================\n");
        printf("TESTANDO I-NODE %d\n", i + 1);
        printf("=============================================\n");

        // Aloca um novo inode
        int inodeIndex = allocInode(iNodeTable, TYPE_FILE);

        if(inodeIndex == -1){
            printf("Falha ao alocar i-node.\n");
            continue;
        }

        printf("I-node alocado no indice %d\n", inodeIndex);

        // Conteúdo diferente para cada inode
        char texto[128];

        sprintf(texto,
                "Este eh o conteudo do inode %d. Teste de leitura e escrita.",
                inodeIndex);

        // Escrita
        if(iNodeWriteData(
                &disk,
                iNodeTable,
                inodeIndex,
                texto,
                strlen(texto)+1))
        {
            printf("Escrita realizada com sucesso.\n");
        }
        else{
            printf("Falha na escrita.\n");
            continue;
        }

        // Relatório do inode
        printInodeTableRelatory(iNodeTable);

        // Teste do getBlockFromInode() 

        printf("\nPonteiros diretos:\n");

        for(int j=0;j<iNodeTable[inodeIndex].quantBlocks;j++){

            uint32_t bloco;

            if(getBlockFromInode(
                    iNodeTable,
                    inodeIndex,
                    j,
                    &bloco))
            {
                printf("Direto[%d] -> bloco %u\n",j,bloco);
            }
        }

        // Teste da leitura

        char leitura[128];

        memset(leitura,0,sizeof(leitura));

        if(iNodeReadData(
                &disk,
                iNodeTable,
                inodeIndex,
                leitura))
        {
            printf("\nConteudo lido:\n");
            printf("%s\n",leitura);

            if(strcmp(texto,leitura)==0){
                printf("Leitura validada.\n");
            }
            else{
                printf("ERRO: Conteudo diferente do escrito.\n");
            }
        }
        else{
            printf("Falha na leitura.\n");
        }

        // Teste da remoção de um bloco

        if(iNodeTable[inodeIndex].quantBlocks > 0){

            uint32_t bloco =
                iNodeTable[inodeIndex].blocks[0];

            printf("\nRemovendo bloco %u...\n",bloco);

            if(removeBlockFromInode(
                    iNodeTable,
                    inodeIndex,
                    bloco))
            {
                printf("Bloco removido.\n");

                printf("Quantidade atual de blocos: %d\n",
                       iNodeTable[inodeIndex].quantBlocks);
            }
            else{
                printf("Falha ao remover bloco.\n");
            }
        }

        printf("\nEstado final do inode:\n");

        printInodeTableRelatory(iNodeTable);
    }

    printf("\n==============================\n");
    printf("ESTADO FINAL DO DISCO\n");
    printf("==============================\n");

    printDiskInfo(&disk);

    closeVirtualDisk(&disk);

    free(iNodeTable);

    return 0;
}
*/