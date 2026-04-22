/**
 * Módulo de Pesquisa (search)
 * ---------------------------
 * Declara estruturas e funções para pesquisa de documentos por palavra-chave.
 * Suporta processamento sequencial ou em paralelo via SearchTask.
 */

#ifndef SEARCH_H
#define SEARCH_H

#include "document.h"   // Document, documents[], doc_count

/**
 * SearchTask:
 *   Estrutura usada para dividir o trabalho de pesquisa em paralelo.
 *   - start_idx: índice inicial (inclusivo) no array documents[]
 *   - end_idx:   índice final (exclusivo) no array documents[]
 *   - keyword:   palavra-chave a procurar em cada ficheiro
 *   - results:   array alocado para guardar as chaves encontradas
 *   - results_size: número máximo de elementos em results; após execução,
 *                   contém o número real de resultados
 *  - docs:     ponteiro para o array de documentos (global)
 */
typedef struct {
    int   start_idx;
    int   end_idx;
    char  keyword[MAX_BUF];  // palavra-chave a pesquisar
    int  *results;           // buffer de resultados (chaves encontradas)
    int   results_size;      // capacidade / após execução, total de resultados
    Document *docs;          // ponteiro para o array de documentos     
} SearchTask;


/**
 * search_documents:
 *   Executa pesquisa de 'task->keyword' em todos os documentos ativos
 *   no intervalo [start_idx, end_idx). Preenche task->results com as
 *   chaves encontradas e atualiza task->results_size.
 *
 * Parâmetro:
 *   task - apontador para SearchTask configurada previamente
 */
void search_documents(SearchTask *task);

#endif // SEARCH_H