# Estrutura do Disco Simulado

O modulo de disco e responsavel por representar a particao virtual do sistema de arquivos. A particao e armazenada em um arquivo binario, permitindo que as informacoes principais do sistema sejam preservadas entre execucoes do programa.

## Decisoes de projeto

O disco simulado foi organizado em tres partes principais:

1. Cabecalho do disco.
2. Bitmap de blocos livres.
3. Area de blocos de dados.

O cabecalho guarda as informacoes gerais da particao, como tamanho total do disco, tamanho de cada bloco, quantidade total de blocos, quantidade de blocos livres e posicoes internas do bitmap e da area de dados.

O bitmap foi escolhido para controlar o espaco livre porque e uma estrutura simples e eficiente para indicar se cada bloco esta livre ou ocupado. Cada bit representa um bloco de dados. O valor `0` indica bloco livre e o valor `1` indica bloco ocupado.

## Limites definidos

Foram definidos limites iniciais para evitar configuracoes invalidas:

- tamanho minimo do disco: 64 KB;
- tamanho maximo do disco: 64 MB;
- tamanho minimo do bloco: 64 bytes;
- tamanho maximo do bloco: 4096 bytes;
- o tamanho do bloco deve ser potencia de 2.

Esses limites podem ser ajustados depois, mas ja permitem testar o simulador com discos pequenos e medios sem deixar o gerenciamento muito pesado.

## Funcionamento

Ao criar um disco, o programa recebe o caminho do arquivo, o tamanho total da particao e o tamanho do bloco. Depois disso, calcula quantos blocos de dados cabem no arquivo, reservando antes o espaco necessario para o cabecalho e para o bitmap.

Quando um bloco e alocado, o sistema procura o primeiro bit livre no bitmap, marca esse bloco como ocupado e atualiza a quantidade de blocos livres no cabecalho. Quando um bloco e liberado, o bit correspondente volta para livre e o contador de blocos livres aumenta.

As alteracoes no cabecalho e no bitmap sao salvas no arquivo do disco simulado, garantindo que o estado do disco continue correto depois que o programa for fechado e aberto novamente.

## Funcoes principais

- `createVirtualDisk`: cria a particao virtual.
- `openVirtualDisk`: abre um disco simulado existente.
- `closeVirtualDisk`: fecha o arquivo do disco e libera memoria.
- `allocateBlock`: procura e reserva um bloco livre.
- `freeBlock`: libera um bloco ocupado.
- `isBlockFree`: verifica se um bloco esta livre.
- `readBlock`: le o conteudo de um bloco ocupado.
- `writeBlock`: escreve dados em um bloco ocupado.
- `printDiskInfo`: mostra as informacoes principais do disco.

## Exemplo de demonstracao

```bash
make
./tp2_so init disco_simulado.bin 65536 512
./tp2_so info disco_simulado.bin
./tp2_so alloc disco_simulado.bin
./tp2_so alloc disco_simulado.bin
./tp2_so free disco_simulado.bin 0
./tp2_so info disco_simulado.bin
```

Nesse exemplo, o disco e criado com 64 KB e blocos de 512 bytes. Depois, dois blocos sao alocados e um deles e liberado. No final, o comando `info` mostra a quantidade atual de blocos livres e usados.

## Fala sugerida para apresentacao

A minha parte foi a estrutura do disco simulado. Eu implementei a criacao da particao virtual em um arquivo binario, permitindo que o usuario escolha o tamanho do disco e o tamanho dos blocos. O disco possui um cabecalho com os metadados principais, um bitmap para controlar quais blocos estao livres ou ocupados e uma area reservada para os blocos de dados.

Escolhemos o bitmap porque ele e simples de implementar e representa bem o conceito de gerenciamento de espaco livre em sistemas de arquivos. Cada bit indica o estado de um bloco. Quando um bloco e alocado, o bit muda para ocupado; quando o bloco e liberado, ele volta para livre. Essas informacoes sao salvas no proprio arquivo do disco, entao o simulador consegue manter o estado entre execucoes.
