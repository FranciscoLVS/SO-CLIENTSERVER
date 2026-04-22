SO-CLIENTSERVER

Projeto desenvolvido no ambito da unidade curricular de Sistemas Operativos.

Autores:
- Carlos Miguel Lopes Silva - a106792
- Francisco Luis Veloso Soares - a106882
- Nuno Francisco Rocha Soares - a107366

Descricao
Este trabalho implementa um servico de indexacao e pesquisa de documentos em arquitetura cliente-servidor, usando IPC com FIFOs em Linux.

O servidor (dserver):
- recebe comandos de clientes por FIFO publica
- gere metadados de documentos e persistencia em metadata.bin
- suporta operacoes de ADD, GET, DEL, COUNT, SEARCH e SHUTDOWN
- executa pesquisa sequencial ou paralela (multiprocessos)

O cliente (dclient):
- envia pedidos ao servidor
- usa FIFO de resposta por cliente
- apresenta resultados formatados no terminal

Funcionalidades principais
- Indexacao de documentos com titulo, autores, ano e caminho
- Consulta e remocao de entradas indexadas
- Contagem de ocorrencias de palavra-chave num documento
- Pesquisa global por palavra-chave (modo sequencial ou paralelo)
- Cache em memoria com politica LRU
- Persistencia de metadados entre execucoes

Estrutura resumida
- dserver.c: ciclo principal do servidor e processamento de comandos
- dclient.c: interface de linha de comandos do cliente
- handlers.c: logica de cada operacao
- cache.c: gestao de cache LRU
- search.c: pesquisa por palavra-chave
- addGdatasetMetadata.sh: carga automatica de metadados a partir do dataset
- test_performance.sh: benchmark de pesquisa paralela e impacto da cache

Requisitos
- Linux
- gcc
- make
- bash
- bc (necessario para script de performance)

Compilacao
Na pasta aplicacao:

make clean
make

Isto gera os executaveis:
- ./dserver
- ./dclient

Execucao
1) Iniciar o servidor

./dserver server 1000

Notas:
- server e a pasta com os ficheiros de texto
- 1000 define o tamanho maximo da cache (ajustavel)

2) Comandos do cliente

Adicionar documento:
./dclient -a "Titulo" "Autor" "Ano" "server/test.txt"

Consultar documento (por chave):
./dclient -c "1"

Apagar documento:
./dclient -d "1"

Contar ocorrencias de palavra num documento:
./dclient -l "1" "keyword"

Pesquisar palavra-chave (1 processo):
./dclient -s "keyword"

Pesquisar palavra-chave com N processos:
./dclient -s "keyword" 8

Desligar servidor:
./dclient -f

Carga automatica com dataset
Para indexar documentos da tabela Gcatalog.tsv:

./addGdatasetMetadata.sh Gcatalog.tsv

Se surgir "Doc list full", aumente MAX_DOCS em common.h ou remova metadata.bin para reiniciar os dados.

Teste de performance
O script mede:
- impacto do numero de processos na pesquisa paralela
- impacto do tamanho da cache no tempo medio de pesquisa

Execucao:

./test_performance.sh

Resultados gerados:
- parallel_search_results.csv
- cache_results.csv

Limpeza e reset
- Remover binarios e objetos: make clean
- Reiniciar metadados persistidos: rm -f metadata.bin

Objetivo academico
Este projeto foi desenvolvido para praticar conceitos de Sistemas Operativos, com foco em:
- comunicacao entre processos (IPC)
- sincronizacao e organizacao de servicos cliente-servidor
- desempenho de pesquisa com paralelizacao
- politicas de cache e persistencia de dados

