#include "./inode.h"

/**
 * 
 */
void initializeInode(Inode* iNodeTable){
    iNodeTable = (Inode*)malloc(sizeof(Inode) * MAX_INODES);

    for(int i = 0; i < MAX_INODES; i++){
        iNodeTable[i].isBeingUsed = 0;
        iNodeTable[i].quantBlocks = 0;
        
        for(int j = 0; j < DIRECT_POINTERS; j++){
            iNodeTable[i].blocks[j] = (uint32_t)-1; // Como o i-node ainda não está referenciando nenhum arquivo, todos os ponteiros diretos dele são definidos como -1 
                                                    // para melhor segurança do sistema, a medida que forem alocados blocos pelo i-node, eles serão referenciados em 
                                                    // cada posição do vetor.
        }
    }
}

/**
 * 
 * @param iNodeTable Target i-node table that the i-node will be allocated
 * @param FileType type of the iNode TODO: describe it better
 * 
 * @returns Index of the i-node alocated in the i-node Table
 */
int allocInode(Inode iNodeTable[], FileType type){ //FIXME: Check if the vector parameter is equal to pass the pointer of the first position
    for(int i = 0; i < MAX_INODES; i++){
        if(!iNodeTable[i].isBeingUsed){ // Not being used
            iNodeTable[i].isBeingUsed = 1; //Now is being used

            iNodeTable[i].size = 0;
            iNodeTable[i].type = type;
            time(&iNodeTable[i].creationDate);

            return i;
        }
    }

    return -1; // there's no free i-node in the entire i-node table
}

/**
 * 
 * @param iNodeTable
 * @param iNodeIndex
 * @param blockIndex
 */
void addBlockToInode(Inode iNodeTable[], int iNodeIndex, uint32_t blockIndex){
    Inode* inode = &iNodeTable[iNodeIndex]; // Ponteiro ajustado para o endereço do i-node que quero acessar

    inode->blocks[inode->quantBlocks] = blockIndex;
    time(&inode->modificationDate); //FIXME: possível problema com lógica de ponteiros

    inode->quantBlocks++;
}

/**
 * @param iNodeTable
 * @param iNodeIndex
 * @param blockIndex
 * 
 * @returns 1 case success, 0 case does not found block
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
 * @param iNodeTable
 * @param iNodeIndex
 * @param pointerIndex
 * @param blockIndex
 * 
 * @returns 
 */
int getBlockFromInode(Inode iNodeTable[], int iNodeIndex, int pointerIndex, uint32_t *blockIndex){
    Inode* inode = &iNodeTable[iNodeIndex]; // Ponteiro ajustado para o endereço do i-node que quero acessar

    if(pointerIndex < 0 || pointerIndex >= inode->quantBlocks){ // Invalid value 
        return 0;
    }

    *blockIndex = inode->blocks[pointerIndex];
    time(&inode->accessDate);

    return 1;
}

/**
 * 
 */
static int getBlockOffset(){

    //posição do bloco = início dos dados + (bloco * tamanho do bloco)
}

/**
 * @param disk Disco virtual que será acessado
 * @param iNodetable Tabela de i-nodes
 * @param iNodeIndex Índice do i-node
 * @param buffer Buffer onde os dados serão escritos
 */
void iNodeReadData(VirtualDisk* disk, Inode iNodeTable[], int iNodeIndex, void* buffer){
    Inode* inode = &iNodeTable[iNodeIndex]; // Ponteiro ajustado para o endereço do i-node que quero acessar

    char* data = buffer;
    uint32_t offset = 0;

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

        readBlock(disk, inode->blocks[i], (data+offset), bytesToRead);
        offset += bytesToRead;
    }

    time(&inode->accessDate);
}

/**
 * @param disk Disco virtual previamente aberto
 * @param iNodeTable Tabela de i-nodes
 * @param iNodeIndex Índice do i-node que receberá os dados
 * @param buffer conteúdo que será escrito
 * @param size quantidade de bytes
 * 
 * 
 */
void iNodeWriteData(VirtualDisk* disk, Inode iNodeTable[], int iNodeIndex, const void *buffer, uint32_t size){
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
            return;
        }

        // Atribui um i-node ao bloco
        addBlockToInode(iNodeTable, iNodeIndex, blockIndex);

        //Variável local declarada unicamente para definir a quantidade de bytes restantes a serem armazenados no disco.
        uint32_t bytesToWrite = remaining > disk->header.blockSize ? disk->header.blockSize : remaining;

        writeBlock(disk, blockIndex, (data+offset), bytesToWrite);

        // Atualiza dados de apoio
        remaining -= bytesToWrite;
        offset += bytesToWrite;
    }

    inode->size = size;
    time(&inode->modificationDate);
}

/**
 * Não sei se essa função é realmente necessária
 */
void freeInode(Inode iNodeTable[], int index){
    iNodeTable[index].isBeingUsed = 0;

    /* Free the direct pointers is necessary?
    for(int i = 0; i < DIRECT_POINTERS; i++){
        iNodeTable[index].blocks[i] = -1;
    }
    */
}