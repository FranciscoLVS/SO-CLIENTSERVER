/**
 * Módulo de Handlers
 * ------------------
 * Declara as funções que executam cada operação solicitada pelo cliente:
 *  - send_response:           envia resposta pela FIFO de resposta
 *  - handle_add:              adiciona e indexa novo documento
 *  - handle_get:              obtém metadados de um documento
 *  - handle_del:              remove um documento indexado
 *  - handle_count:            conta ocorrências de uma palavra num documento
 *  - handle_search:           pesquisa sequencial de palavra-chave
 *  - handle_search_command:   pesquisa paralela usando forks e pipes
 */

#ifndef HANDLERS_H
#define HANDLERS_H

/**
 * send_response:
 *   Envia a string 'response' (com terminador '\0') pela FIFO de resposta.
 *
 * Parâmetros:
 *   response - string com a mensagem a enviar ao cliente.
 */
void send_response(const char *response, const char *response_fifo);

/**
 * handle_add:
 *   Processa o comando ADD, partindo de 'args' no formato:
 *     "title|authors|year|path"
 *   Invoca add_document() e formata em 'response' a mensagem de sucesso
 *   ou erro.
 *
 * Parâmetros:
 *   args     - texto com campos separados por '|'
 *   response - buffer para colocar a resposta ao cliente
 */
void handle_add(char *args, char *response);

/**
 * handle_get:
 *   Processa o comando GET, onde 'args' é a chave do documento.
 *   Procura metadados e devolve em 'response' o formato:
 *     "title|authors|year|path"
 *   ou mensagem de erro se não existir.
 *
 * Parâmetros:
 *   args     - string com a chave do documento
 *   response - buffer para colocar a resposta ao cliente
 */
void handle_get(char *args, char *response);

/**
 * handle_del:
 *   Processa o comando DEL, marcando o documento como inativo.
 *   Em caso de sucesso, responde:
 *     "Index entry N deleted"
 *   ou erro se não encontrar.
 *
 * Parâmetros:
 *   args     - string com a chave do documento
 *   response - buffer para colocar a resposta ao cliente
 */
void handle_del(char *args, char *response);

/**
 * handle_count:
 *   Processa o comando COUNT, onde 'args' é:
 *     "key|keyword"
 *   Conta quantas vezes 'keyword' ocorre no ficheiro do documento 'key'
 *   e devolve o número em 'response'.
 *
 * Parâmetros:
 *   args     - string "key|keyword"
 *   response - buffer para colocar a resposta ao cliente
 */
void handle_count(char *args, char *response);

/**
 * handle_search:
 *   Processa o comando SEARCH de forma sequencial.
 *   'args' tem o formato "keyword|proc_count" (proc_count ignorado se >1).
 *   Devolve em 'response' lista com chaves encontradas:
 *     "[k1, k2, ...]" ou "[]".
 *
 * Parâmetros:
 *   args     - string "keyword|proc_count"
 *   response - buffer para colocar a resposta ao cliente
 */
void handle_search(char *args, char *response);

/**
 * handle_search_command:
 *   Processa o comando SEARCH em paralelo:
 *   - divide o trabalho entre 'proc_count' processos,
 *   - cada filho escreve resultados no pipe,
 *   - o pai usa select() para ler todos em tempo real,
 *   - agrega e envia lista final "[k1, k2, ...]".
 *
 * Parâmetros:
 *   keyword    - palavra-chave a pesquisar
 *   proc_count - número de processos a criar
 */
void handle_search_command(char *keyword, int proc_count, const char *response_fifo);
#endif // HANDLERS_H