# <center>Trabalho Prático 2 - Sistemas Operacionais</center>

# <center>Simulador de Sistema de Arquivos com i-nodes</center>

---

## Sumário

### 1. Introdução

### 2. Objetivo

### 3. Descrição

### 4. Integrantes

---

## Introdução

Este trabalho tem como objetivo a implementação de um simulador de sistema de arquivos inspirado nos conceitos estudados na disciplina de Sistemas Operacionais. O projeto foi desenvolvido na linguagem C e busca representar, de forma simplificada, o funcionamento interno de um sistema de arquivos baseado em i-nodes, abordando conceitos fundamentais como gerenciamento de blocos, diretórios, arquivos e organização do armazenamento, tudo no sistema operacional Linux.

---

## Objetivo

Desenvolver um simulador de sistema de arquivos capaz de gerenciar arquivos e diretórios por meio da utilização de i-nodes, permitindo operações como criação, remoção, renomeação, movimentação e listagem de arquivos e diretórios no sistema Linux.

---

## Descrição

O programa foi organizado em módulos independentes, cada um responsável por uma parte específica do sistema de arquivos simulado.

### disco

Responsável pelo gerenciamento da partição simulada, incluindo a definição do tamanho do disco, dos blocos de armazenamento, da alocação e liberação de espaço e do controle de blocos disponíveis.

### inode

Implementa a estrutura dos i-nodes, armazenando informações sobre arquivos e diretórios, como identificadores, tamanho, datas de criação, modificação e acesso, além das referências para os blocos de dados.

### diretorio

Gerencia a estrutura hierárquica de diretórios do sistema, permitindo criar, renomear, remover e listar diretórios e seus respectivos conteúdos.

### arquivo

Responsável pelas operações relacionadas aos arquivos simulados, incluindo criação, remoção, movimentação, renomeação e exibição de conteúdo.

### importador

Permite a importação de arquivos reais do sistema operacional hospedeiro para o sistema de arquivos simulado, realizando a leitura do conteúdo e seu armazenamento na estrutura interna do projeto.

### interface

Implementa a interação com o usuário, oferecendo um modo de operação interativo e a execução de comandos previamente definidos em arquivos de entrada. Também disponibiliza um modo verboso para acompanhamento detalhado das operações realizadas.

Além desses módulos, o projeto utiliza um arquivo de configuração compartilhado contendo constantes, definições globais e estruturas de dados utilizadas por todo o sistema.

---

## Integrantes

<p>Geovana Beatriz de Oliveira</p>

<p>Lucas Garcia Ferreira Franco Fonseca</p>

<p>Nome do Integrante 3</p>

<p>Nome do Integrante 4</p>

<p>Nome do Integrante 5</p>

<p>Nome do Integrante 6</p>
