/**
 * Módulo de Pesquisa (search)
 * --------------------------
 * Implementa a função de pesquisa de documentos por palavra-chave,
 * tanto para uso sequencial como para paralelização via SearchTask.
 * A actual função percorre um intervalo de documentos e preenche
 * o task->results com as chaves encontradas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

#include "search.h"    // Declara SearchTask e protótipo de search_documents
#include "document.h"  // Document, documents[], doc_count
#include "cache.h"     // cache_put, cache_get (caso queiramos cachear resultados)
#include "common.h"    // MAX_DOCS, MAX_BUF, etc.
#include "utils.h"     // file_contains()

/**
 * search_documents:
 *   Percorre o intervalo [start_idx, end_idx) de documentos ativos,
 *   verifica se cada ficheiro contém a palavra-chave e, em caso afirmativo,
 *   armazena a chave no task->results.
 *
 *
 * Parâmetro:
 *  task - apontador para SearchTask configurada previamente
 *
 *  - start_idx: índice inicial (inclusivo) no array documents[]
 *  - end_idx:   índice final (exclusivo) no array documents[]
 *  - keyword:   palavra-chave a procurar em cada ficheiro
 *  - results:   array alocado para guardar as chaves encontradas
 *  - results_size: número máximo de elementos em results; após execução,
 *                   contém o número real de resultados
 *  - docs:     ponteiro para o array de documentos (global)
 */

void search_documents(SearchTask *task) {
    if (!task || !task->results || task->results_size <= 0 || !task->docs) {
        //printf("DEBUG: Task inválida ou não inicializada\n");
        return;
    }

    //printf("DEBUG: Processo %d procurando '%s' nos índices %d-%d\n", 
          // getpid(), task->keyword, task->start_idx, task->end_idx);

    int found_count = 0;
    int found_docs[MAX_DOCS];

    for (int i = task->start_idx; i < task->end_idx && i < doc_count; i++) {
        //printf("DEBUG: Doc %d (key=%d) active=%d\n", 
            //   i, task->docs[i].key, task->docs[i].active);
        
        if (!task->docs[i].active) continue;

        if (file_contains(task->docs[i].path, task->keyword)) {
            //printf("DEBUG: Encontrado em doc %d (key=%d)\n", i, task->docs[i].key);
            if (found_count < MAX_DOCS) {
                found_docs[found_count++] = task->docs[i].key;
            }
        }
    }

    //printf("DEBUG: Processo %d encontrou %d resultados\n", getpid(), found_count);
    task->results_size = found_count;
    for (int i = 0; i < found_count; i++) {
        task->results[i] = found_docs[i];
    }
}