/**
 * Módulo de Handlers
 * ------------------
 * Contém as funções que executam cada operação solicitada pelo cliente:
 *  - send_response: envia resposta pela FIFO
 *  - handle_add:    adiciona novo documento
 *  - handle_get:    obtém metadados de um documento
 *  - handle_del:    remove documento
 *  - handle_count:  conta ocorrências de palavra num documento
 *  - handle_search: pesquisa sequencial
 *  - handle_search_command: pesquisa paralela com fork e pipes
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>      // write, close, fork, pipe
#include <sys/wait.h>    // waitpid
#include <fcntl.h>       // open, O_*
#include <sys/select.h>  // select, fd_set

#include "document.h"    // Document, add_document, get_document_by_key, save_metadata
#include "cache.h"       // cache_put, cache_get (usado em GET)
#include "common.h"      // MAX_BUF, FIFO_RESPONSE, comandos
#include "search.h"      // search_documents, SearchTask
#include "utils.h"       // file_contains, count_occurrences

/**
 * send_response:
 *   Abre a FIFO de resposta e escreve a string 'response' (inclui '\0').
 */
void send_response(const char *response, const char *response_fifo) {
    int fd = open(response_fifo, O_WRONLY);
    if (fd != -1) {
        write(fd, response, strlen(response) + 1);  // envia também o terminador nulo
        close(fd);
    }
}

/**
 * handle_add:
 *   Parâmetro args: "title|authors|year|path"
 *   Atribui novos campos, chama add_document e formata resposta:
 *   - Document N indexed   ou mensagens de erro
 */
void handle_add(char *args, char *response) {
    char *title   = strtok(args, "|");
    char *authors = strtok(NULL, "|");
    char *year    = strtok(NULL, "|");
    char *path    = strtok(NULL, "|");

    if (title && authors && year && path) {
        int key = add_document(title, authors, year, path);
        if (key == -2) {
            strcpy(response, "ERROR Failed to copy file");
        } else if (key == -1) {
            strcpy(response, "ERROR Document list full");
        } else {
            snprintf(response, MAX_BUF, "Document %d indexed", key);
        }
    } else {
        strcpy(response, "ERROR Invalid arguments");
    }
}

/**
 * handle_get:
 *   Parâmetro args: chave (string)
 *   Procura documento por key e retorna "title|authors|year|path"
 *   ou "ERROR Document not found".
 */
void handle_get(char *args, char *response) {
    int key = atoi(args);
    Document *doc = get_document_by_key(key);
    
    if (doc) {
        snprintf(response, MAX_BUF, "%s|%s|%s|%s",
            doc->title,
            doc->authors,
            doc->year,
            doc->path);
        response[MAX_BUF - 1] = '\0';  // garante terminação
    } else {
        strcpy(response, "ERROR Document not found");
    }
}

/**
 * handle_del:
 *   Parâmetro args: chave (string)
 *   Marca documento como inativo, atualiza metadata e responde:
 *   - Index entry N deleted  ou "ERROR Document not found".
 */
void handle_del(char *args, char *response) {
    int key = atoi(args);
    int found = 0;
    for (int i = 0; i < doc_count; i++) {
        if (documents[i].key == key && documents[i].active) {
            documents[i].active = 0;
            save_metadata();  // grava alteração
            snprintf(response, MAX_BUF, "Index entry %d deleted", key);
            found = 1;
            break;
        }
    }
    if (!found) {
        strcpy(response, "ERROR Document not found");
    }
}

/**
 * handle_count:
 *   Parâmetro args: "key|keyword"
 *   Conta ocorrências de 'keyword' no ficheiro do documento 'key'.
 *   Resposta: número (string) ou erro se não existir.
 */
void handle_count(char *args, char *response) {
    char *key_str = strtok(args, "|");
    char *keyword = strtok(NULL, "|");
    int key = atoi(key_str);
    int found = 0;

    for (int i = 0; i < doc_count; i++) {
        if (documents[i].key == key && documents[i].active) {
            int count = count_occurrences(documents[i].path, keyword);
            snprintf(response, MAX_BUF, "%d", count);
            found = 1;
            break;
        }
    }
    if (!found) {
        strcpy(response, "ERROR Document not found");
    }
}

/**
 * handle_search:
 *   Pesquisa sequencial de 'keyword' em todos os documentos ativos.
 *   args: "keyword|proc_count" (proc_count ignorado aqui se >1)
 *   Resposta: lista "[k1, k2, ...]" ou "[]".
 */
void handle_search(char *args, char *response) {
    char *keyword        = strtok(args, "|");
    char *proc_count_str = strtok(NULL, "|");

    if (!keyword) {
        strcpy(response, "ERROR Missing keyword");
        return;
    }

    // força processamento sequencial quando proc_count <= 1
    int proc_count = proc_count_str ? atoi(proc_count_str) : 1;
    if (proc_count < 2) {
        int total = 0;
        strcpy(response, "[");
        size_t len = 1, maxlen = MAX_BUF - 2;

        for (int i = 0; i < doc_count; i++) {
            if (!documents[i].active) continue;
            if (file_contains(documents[i].path, keyword)) {
                char tmp[16];
                size_t tmplen = snprintf(tmp, sizeof(tmp),
                    total ? ", %d" : "%d", documents[i].key);
                if (len + tmplen < maxlen) {
                    strcat(response, tmp);
                    len += tmplen;
                    total++;
                } else break;
            }
        }
        strcat(response, "]");
        if (total == 0) strcpy(response, "[]");
    } else {
        // se pedirem paralela, devolve mensagem placeholder
        strcpy(response, "ERROR Parallel search not implemented");
    }
}

/**
 * handle_search_command:
 *   Pesquisa paralela usando fork/pipes:
 *   - divide documentos entre 'proc_count' filhos
 *   - cada filho escreve resultados no pipe
 *   - pai usa select() para ler cada pipe em tempo real
 *   - agrega listas e envia resposta "[k1, k2, ...]".
 */
void handle_search_command(char *keyword, int proc_count, const char *response_fifo) {
    pid_t *pids    = malloc(proc_count * sizeof(pid_t));
    SearchTask *tasks = malloc(proc_count * sizeof(SearchTask));
    int    *pipes  = malloc(2 * proc_count * sizeof(int));
    char    dummy[MAX_BUF];

    if (!pids || !tasks || !pipes) {
        free(pids); free(tasks); free(pipes);
        strcpy(dummy, "ERROR Memory allocation failed");
        send_response(dummy, response_fifo);
        return;
    }

    /**
    printf("DEBUG: Estado dos documentos antes do fork:\n");
    for (int i = 0; i < doc_count; i++) {
        if (documents[i].key == 1646 || documents[i].key == 1647) {
            printf("DEBUG: Doc %d (key=%d) active=%d\n", 
                   i, documents[i].key, documents[i].active);
        }
    }
    **/

    int docs_per = (doc_count + proc_count - 1) / proc_count;
    int active = 0;

    // 1) Cria processos filhos e configura pipes
    for (int i = 0; i < proc_count; i++) {
        int start = i * docs_per;
        int end   = start + docs_per;
        if (start >= doc_count) { pids[i] = 0; continue; }
        if (end > doc_count) end = doc_count;

        if (pipe(&pipes[2*i]) < 0) { pids[i] = 0; continue; }
        tasks[i].start_idx     = start;
        tasks[i].end_idx       = end;
        tasks[i].docs          = documents;  // Este ponteiro é correto, mas...
        strncpy(tasks[i].keyword, keyword, sizeof(tasks[i].keyword)-1);
        tasks[i].keyword[sizeof(tasks[i].keyword)-1] = '\0';
        tasks[i].results       = malloc((end - start) * sizeof(int));
        tasks[i].results_size  = end - start;

        pid_t pid = fork();
        if (pid == 0) {
            // No processo filho, o ponteiro tasks[i].docs aponta para uma cópia
            // dos documentos no momento do fork. Quaisquer alterações feitas no
            // processo pai (como marcar documentos como inativos) não serão
            // refletidas nesta cópia.
            close(pipes[2*i]);
            search_documents(&tasks[i]);
            write(pipes[2*i+1], &tasks[i].results_size, sizeof(int));
            if (tasks[i].results_size)
                write(pipes[2*i+1], tasks[i].results, tasks[i].results_size * sizeof(int));
            close(pipes[2*i+1]);
            free(tasks[i].results);
            exit(0);
        } else if (pid > 0) {
            // pai fecha escrita e guarda PID
            close(pipes[2*i+1]);
            pids[ i ] = pid;
            active++;
        } else {
            // erro no fork
            close(pipes[2*i]); close(pipes[2*i+1]);
            pids[i] = 0;
        }
    }

    // 2) Prepara leitura dos resultados 
    int *sizes       = calloc(proc_count, sizeof(int));
    int **all_results= calloc(proc_count, sizeof(int*));
    int remaining = active;
    fd_set rfds;

    // 3) Usa select() para ler resultados assim que disponíveis
    while (remaining > 0) {
        FD_ZERO(&rfds);
        int maxfd = 0;
        for (int i = 0; i < proc_count; i++) {
            if (pids[i]) {
                int r = pipes[2*i];
                FD_SET(r, &rfds);
                if (r > maxfd) maxfd = r;
            }
        }
        if (select(maxfd+1, &rfds, NULL, NULL, NULL) < 0) break;
        // verifica cada pipe e lê resultados
        // se o pipe estiver pronto, lê o tamanho e depois os inteiros
        // se o tamanho for 0, o processo terminou
        // se o tamanho for >0, aloca buffer e lê os inteiros
        // se o read falhar ou o tamanho for 0, fecha pipe e marca PID como 0
        // decrementa remaining
        for (int i = 0; i < proc_count; i++) {
            int r = pipes[2*i];
            if (pids[i] && FD_ISSET(r, &rfds)) {
                if (sizes[i] == 0) {
                    int n = read(r, &sizes[i], sizeof(int));
                    if (n <= 0 || sizes[i] == 0) {
                        close(r);
                        pids[i] = 0;
                        remaining--;
                    } else {
                        all_results[i] = malloc(sizes[i] * sizeof(int)); // aloca buffer 
                        read(r, all_results[i], sizes[i] * sizeof(int));
                        close(r);
                        pids[i] = 0;
                        remaining--;
                    }
                }
            }
        }
    }

    // 4) Espera todos os filhos terminarem
    for (int i = 0; i < proc_count; i++)
        if (pids[i]) waitpid(pids[i], NULL, 0);

    // 5) Agrega resultados em string JSON-like
    char result[MAX_BUF] = "[";
    int count = 0;
    for (int i = 0; i < proc_count; i++) {
        if (sizes[i] > 0 && all_results[i]) {
            for (int j = 0; j < sizes[i]; j++) {
                char tmp[16];
                snprintf(tmp, sizeof(tmp),
                         count ? ", %d" : "%d",
                         all_results[i][j]);
                if (strlen(result) + strlen(tmp) < sizeof(result)-1) {
                    strcat(result, tmp);
                    count++;
                }
            }
            free(all_results[i]);
        }
    }
    strcat(result, "]");
    if (count == 0) strcpy(result, "[]");

    // 6) Liberta buffers e envia resposta
    free(sizes); free(all_results);
    free(pids);  free(tasks); free(pipes);
    send_response(result, response_fifo);
}