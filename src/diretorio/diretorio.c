#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "diretorio.h"

/* FUNÇÕES AUXILIARES */

static int maxEntradasDiretorio(VirtualDisk *disk){
    // Para simplificação vamos usar apenas 1 bloco por diretório
    return disk->header.blockSize / sizeof(Diretorio);
}

static void inicializarEntradas(Diretorio entradas[], int maxEntradas){
    // Inicializa todas as entradas do diretório como vazias
    for(int i=0; i<maxEntradas; i++){
        entradas[i].nome[0] = '\0';
        entradas[i].inode = -1;
        entradas[i].type = DIRECTORY;
        entradas[i].usado = 0;
    }
}

static int carregarDiretorio(VirtualDisk *disk, Inode inodeTable[], int inodeDir, Diretorio entradas[]){
    // Carrega as entradas do diretório do disco para a memória
    Inode *inode = &inodeTable[inodeDir];
    int maxEntradas = maxEntradasDiretorio(disk);

    inicializarEntradas(entradas, maxEntradas);

    if(inode->quantBlocks == 0){
        // Diretório vazio, não há blocos alocados
        return 1;
    }

    if(readBlock(disk, inode->blocks[0], entradas, disk->header.blockSize) != OPERATION_OK){
        return 0;
    }

    time(&inode->accessDate);
    return 1;
    // Retorna 1 se conseguiu carregar, 0 caso contrário
}

static int salvarDiretorio(VirtualDisk *disk, Inode inodeTable[], int inodeDir, Diretorio entradas[]){
    // Salva as entradas do diretório da memória para o disco
    // é usada sempre que uma entrada é adicionada, removida ou renomeada, pois muda o conteúdo do diretório
    Inode *inode = &inodeTable[inodeDir];

    if(inode->quantBlocks == 0){
        // Se não houver blocos alocados para o diretório, aloca um novo bloco
        uint32_t novoBloco;

        if(allocateBlock(disk, &novoBloco) != OPERATION_OK){
            return 0;
        }

        addBlockToInode(inodeTable, inodeDir, novoBloco);
    }

    if(writeBlock(disk, inode->blocks[0], entradas, disk->header.blockSize) != OPERATION_OK){
        return 0;
    }

    // Atualiza o tamanho do diretório e a data de modificação
    inode->size = disk->header.blockSize;
    time(&inode->modificationDate);

    return 1;
}

static int buscarEntrada(VirtualDisk *disk, Inode inodeTable[], int inodeDir, char *nome){
    // Procurar uma entrada pelo nome no diretório especificado pelo inodeDir
    int maxEntradas = maxEntradasDiretorio(disk);
    Diretorio *entradas = malloc(disk->header.blockSize);

    if(entradas == NULL){
        return -1;
    }

    if(!carregarDiretorio(disk, inodeTable, inodeDir, entradas)){
        // Falha ao carregar o diretório para a memória
        free(entradas);
        return -1;
    }

    for(int i=0; i<maxEntradas; i++){
        // Verifica se a entrada está em uso e se o nome corresponde
        // caso encontre, retorna o inode associado a essa entrada
        if(entradas[i].usado && strcmp(entradas[i].nome, nome) == 0){
            int inodeEncontrado = entradas[i].inode;
            free(entradas);
            return inodeEncontrado;
        }
    }

    free(entradas);
    return -1;
}

static int adicionarEntrada(VirtualDisk *disk, Inode inodeTable[], int inodeDir, char *nome, int inodeNovo, FileType type){
    // Adiciona uma nova entrada ao diretório especificado pelo inodeDir
    int maxEntradas = maxEntradasDiretorio(disk);
    Diretorio *entradas = malloc(disk->header.blockSize);

    if(entradas == NULL){
        return 0;
    }

    if(!carregarDiretorio(disk, inodeTable, inodeDir, entradas)){
        free(entradas);
        return 0;
    }

    for(int i=0; i<maxEntradas; i++){
        if(entradas[i].usado == 0 && strcmp(entradas[i].nome, nome) == 0){
            free(entradas);
            return 0;
        }
    }

    // Procura uma posição vazia para adicionar a nova entrada
    for(int i=0; i<maxEntradas; i++){
        if(!entradas[i].usado){
            strncpy(entradas[i].nome, nome, TAM_NOME-1);
            entradas[i].nome[TAM_NOME-1] = '\0'; // Garantir terminação nula
            entradas[i].inode = inodeNovo;
            entradas[i].type = type;
            entradas[i].usado = 1;

            // Salva as alterações no diretório de volta para o disco
            int resultado = salvarDiretorio(disk, inodeTable, inodeDir, entradas);

            free(entradas);
            return resultado;
        }
    }

    free(entradas);
    return 0;
}

static int removerEntrada(VirtualDisk *disk, Inode inodeTable[], int inodeDir, char *nome){
    // Remove uma entrada do diretório especificado pelo inodeDir
    int maxEntradas = maxEntradasDiretorio(disk);
    Diretorio *entradas = malloc(disk->header.blockSize);

    if(entradas == NULL){
        return 0;
    }

    if(!carregarDiretorio(disk, inodeTable, inodeDir, entradas)){
        free(entradas);
        return 0;
    }

    for(int i=0; i<maxEntradas; i++){
        if(entradas[i].usado && strcmp(entradas[i].nome, nome) == 0){
            entradas[i].usado = 0; // Marca a entrada como vazia
            entradas[i].nome[0] = '\0'; // Limpa o nome
            entradas[i].inode = -1; // Limpa o inode

            int resultado = salvarDiretorio(disk, inodeTable, inodeDir, entradas);

            free(entradas);
            return resultado;
        }
    }

    free(entradas);
    return 0; // Entrada não encontrada
}

static int diretorioVazio(VirtualDisk *disk, Inode inodeTable[], int inodeDir){
    // verifica se o diretório especificado pelo inodeDir está vazio (apenas "." e ".." presentes)
    // logo, se pode ser removido
    int maxEntradas = maxEntradasDiretorio(disk);
    Diretorio *entradas = malloc(disk->header.blockSize);

    if(entradas == NULL){
        return 0;
    }

    if(!carregarDiretorio(disk, inodeTable, inodeDir, entradas)){
        free(entradas);
        return 0;
    }

    for(int i=0; i<maxEntradas; i++){
        if(entradas[i].usado){
            if(strcmp(entradas[i].nome, ".") != 0 && strcmp(entradas[i].nome, "..") != 0){
                free(entradas);
                return 0; // Encontrou uma entrada que não é "." ou ".."
            }
        }
    }

    free(entradas);
    return 1; // Diretório está vazio
}

/* FUÇÕES PRINCIPAIS */

void criarDiretorio(VirtualDisk *disk, Inode inodeTable[], int inodePai, char *nome){
    if(disk == NULL || inodeTable == NULL || nome == NULL){
        printf("Erro: parâmetros inválidos.\n");
        return;
    }

    if(inodeTable[inodePai].type != DIRECTORY){
        printf("Erro: inode pai não é um diretório.\n");
        return;
    }

    if(buscarEntrada(disk, inodeTable, inodePai, nome) != -1){
        printf("Erro: já existe uma entrada com o nome '%s' no diretório.\n", nome);
        return;
    }

    int novoInode = allocInode(inodeTable, DIRECTORY);

    if(novoInode == -1){
        printf("Erro: não foi possível alocar um novo inode pois não existe nenhum livre.\n");
        return;
    }

    if(!adicionarEntrada(disk, inodeTable, novoInode, ".", novoInode, DIRECTORY)){
        printf("Erro: não foi possível adicionar a entrada '.' no novo diretório.\n");
        freeInode(inodeTable, novoInode);
        return;
    }

    if(!adicionarEntrada(disk, inodeTable, novoInode, "..", inodePai, DIRECTORY)){
        printf("Erro: não foi possível adicionar a entrada '..' no novo diretório.\n");
        freeInode(inodeTable, novoInode);
        return;
    }

    if(!adicionarEntrada(disk, inodeTable, inodePai, nome, novoInode, DIRECTORY)){
        printf("Erro: não foi possível adicionar a entrada '%s' no diretório pai.\n", nome);
        freeInode(inodeTable, novoInode);
        return;
    }

    printf("Diretório '%s' criado com sucesso.\n", nome);
}

void apagarDiretorio(VirtualDisk *disk, Inode inodeTable[], int inodePai, char *nome){
    if(disk == NULL || inodeTable == NULL || nome == NULL){
        printf("Erro: parâmetros inválidos.\n");
        return;
    }

    if(inodeTable[inodePai].type != DIRECTORY){
        printf("Erro: inode pai não é um diretório.\n");
        return;
    }

    int inodeRemover = buscarEntrada(disk, inodeTable, inodePai, nome);

    if(inodeRemover == -1){
        printf("Erro: diretório '%s' não encontrado no diretório pai.\n", nome);
        return;
    }

    if(inodeTable[inodeRemover].type != DIRECTORY){
        printf("Erro: a entrada '%s' não é um diretório.\n", nome);
        return;
    }

    if(!diretorioVazio(disk, inodeTable, inodeRemover)){
        printf("Erro: diretório '%s' não está vazio.\n", nome);
        return;
    }

    if(!removerEntrada(disk, inodeTable, inodePai, nome)){
        printf("Erro: falha ao remover a entrada '%s' do diretório pai.\n", nome);
        return;
    }

    freeInode(inodeTable, inodeRemover);
    printf("Diretório '%s' apagado com sucesso.\n", nome);
}

void renomearDiretorio(VirtualDisk *disk, Inode inodeTable[],  int inodePai, char *nomeAntigo, char *novoNome){
    if(disk == NULL || inodeTable == NULL || nomeAntigo == NULL || novoNome == NULL){
        printf("Erro: parâmetros inválidos.\n");
        return;
    }

    if(inodeTable[inodePai].type != DIRECTORY){
        printf("Erro: inode pai não é um diretório.\n");
        return;
    }

    if(buscarEntrada(disk, inodeTable, inodePai, novoNome) != -1){
        printf("Erro: já existe uma entrada com o nome '%s' no diretório.\n", novoNome);
        return;
    }

    int maxEntradas = maxEntradasDiretorio(disk);
    Diretorio *entradas = malloc(disk->header.blockSize);

    if(entradas == NULL){
        printf("Erro: falha ao alocar memória para entradas do diretório.\n");
        return;
    }

    if(!carregarDiretorio(disk, inodeTable, inodePai, entradas)){
        printf("Erro: falha ao carregar o diretório.\n");
        free(entradas);
        return;
    }

    for(int i=0; i<maxEntradas; i++){
        if(entradas[i].usado && strcmp(entradas[i].nome, nomeAntigo) == 0){
            if(entradas[i].type != DIRECTORY){
                printf("Erro: a entrada '%s' não é um diretório.\n", nomeAntigo);
                free(entradas);
                return;
            }

            strncpy(entradas[i].nome, novoNome, TAM_NOME-1);
            entradas[i].nome[TAM_NOME-1] = '\0';

            if(!salvarDiretorio(disk, inodeTable, inodePai, entradas)){
                printf("Erro: falha ao salvar o diretório após renomear.\n");
                free(entradas);
                return;
            }

            printf("Diretório '%s' renomeado para '%s' com sucesso.\n", nomeAntigo, novoNome);
            free(entradas);
        }
    }

    printf("Erro: diretório '%s' não encontrado no diretório pai.\n", nomeAntigo);
    free(entradas);
}

void listarConteudoDiretorio(VirtualDisk *disk, Inode inodeTable[], int inodeDir){
    if(disk == NULL || inodeTable == NULL){
        printf("Erro: parâmetros inválidos.\n");
        return;
    }

    if(inodeTable[inodeDir].type != DIRECTORY){
        printf("Erro: inode não é um diretório.\n");
        return;
    }

    int maxEntradas = maxEntradasDiretorio(disk);
    Diretorio *entradas = malloc(disk->header.blockSize);

    if(entradas == NULL){
        printf("Erro: falha ao alocar memória para entradas do diretório.\n");
        return;
    }

    if(!carregarDiretorio(disk, inodeTable, inodeDir, entradas)){
        printf("Erro: falha ao carregar o diretório.\n");
        free(entradas);
        return;
    }

    printf("Conteúdo do diretório (inode %d):\n", inodeDir);
    for(int i=0; i<maxEntradas; i++){
        if(entradas[i].usado){
            printf("Nome: %s | Inode: %d | Tipo: %s\n", entradas[i].nome, entradas[i].inode,
                   (entradas[i].type == DIRECTORY) ? "Diretório" : "Arquivo");
        }
    }

    free(entradas);
}

/* TESTES UNITÁRIOS */

static void imprimirSeparador(char *titulo) {
    printf("\n==============================\n");
    printf("%s\n", titulo);
    printf("==============================\n");
}

int main() {
    VirtualDisk disk;
    Inode inodeTable[MAX_INODES];

    initializeInode(inodeTable);

    if (createVirtualDisk("disco_teste.bin", 1024 * 1024, 4096) != OPERATION_OK) {
        printf("ERRO: nao foi possivel criar o disco virtual.\n");
        return 1;
    }

    if (openVirtualDisk(&disk, "disco_teste.bin") != OPERATION_OK) {
        printf("ERRO: nao foi possivel abrir o disco virtual.\n");
        return 1;
    }

    int raiz = allocInode(inodeTable, DIRECTORY);

    if (raiz == -1) {
        printf("ERRO: nao foi possivel criar o inode raiz.\n");
        closeVirtualDisk(&disk);
        return 1;
    }

    imprimirSeparador("TESTE 1: criar diretorios na raiz");

    criarDiretorio(&disk, inodeTable, raiz, "docs");
    criarDiretorio(&disk, inodeTable, raiz, "imagens");
    criarDiretorio(&disk, inodeTable, raiz, "trabalhos");

    printf("\nResultado esperado:\n");
    printf("Diretorio 'docs' criado com sucesso.\n");
    printf("Diretorio 'imagens' criado com sucesso.\n");
    printf("Diretorio 'trabalhos' criado com sucesso.\n");

    imprimirSeparador("TESTE 2: listar conteudo da raiz");

    listarConteudoDiretorio(&disk, inodeTable, raiz);

    printf("\nResultado esperado na listagem:\n");
    printf("docs       DIR\n");
    printf("imagens    DIR\n");
    printf("trabalhos  DIR\n");

    imprimirSeparador("TESTE 3: tentar criar diretorio repetido");

    criarDiretorio(&disk, inodeTable, raiz, "docs");

    printf("\nResultado esperado:\n");
    printf("Erro: ja existe uma entrada com esse nome.\n");

    imprimirSeparador("TESTE 4: renomear diretorio");

    renomearDiretorio(&disk, inodeTable, raiz, "docs", "documentos");

    printf("\nResultado esperado:\n");
    printf("Diretorio renomeado de 'docs' para 'documentos'.\n");

    printf("\nListagem depois de renomear:\n");
    listarConteudoDiretorio(&disk, inodeTable, raiz);

    printf("\nResultado esperado na listagem:\n");
    printf("documentos DIR\n");
    printf("imagens    DIR\n");
    printf("trabalhos  DIR\n");
    printf("E o nome 'docs' nao deve mais aparecer.\n");

    imprimirSeparador("TESTE 5: tentar renomear para nome ja existente");

    renomearDiretorio(&disk, inodeTable, raiz, "imagens", "trabalhos");

    printf("\nResultado esperado:\n");
    printf("Erro: ja existe uma entrada com o novo nome.\n");

    imprimirSeparador("TESTE 6: apagar diretorio vazio");

    apagarDiretorio(&disk, inodeTable, raiz, "imagens");

    printf("\nResultado esperado:\n");
    printf("Diretorio 'imagens' apagado com sucesso.\n");

    printf("\nListagem depois de apagar:\n");
    listarConteudoDiretorio(&disk, inodeTable, raiz);

    printf("\nResultado esperado na listagem:\n");
    printf("documentos DIR\n");
    printf("trabalhos  DIR\n");
    printf("E o nome 'imagens' nao deve mais aparecer.\n");

    imprimirSeparador("TESTE 7: tentar apagar diretorio inexistente");

    apagarDiretorio(&disk, inodeTable, raiz, "naoExiste");

    printf("\nResultado esperado:\n");
    printf("Erro: diretorio nao encontrado.\n");

    imprimirSeparador("TESTE 8: criar subdiretorio e tentar apagar pai nao vazio");

    criarDiretorio(&disk, inodeTable, raiz, "projetos");

    int inodeProjetos = buscarEntrada(&disk, inodeTable, raiz, "projetos");

    if (inodeProjetos != -1) {
        criarDiretorio(&disk, inodeTable, inodeProjetos, "tp2");
    }

    apagarDiretorio(&disk, inodeTable, raiz, "projetos");

    printf("\nResultado esperado:\n");
    printf("Diretorio 'projetos' criado com sucesso.\n");
    printf("Diretorio 'tp2' criado com sucesso.\n");
    printf("Erro: diretorio nao esta vazio.\n");

    imprimirSeparador("FIM DOS TESTES");

    closeVirtualDisk(&disk);

    return 0;
}