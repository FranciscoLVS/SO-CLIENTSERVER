/**
 * Módulo Servidor (dserver)
 * -------------------------
 * Gere o ciclo de vida do serviço de indexação e pesquisa:
 * - inicializa cache e metadados
 * - cria e monitoriza FIFOs para pedidos/respostas
 * - processa comandos ADD, GET, DEL, COUNT, SEARCH e SHUTDOWN
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>      // write, read, close, unlink
#include <fcntl.h>       // open, O_* 
#include <sys/stat.h>    // mkfifo
#include <sys/types.h>   // tipos básicos
#include <errno.h>
#include <sys/wait.h>    // waitpid
#include <ctype.h>
#include <dirent.h>
#include "cache.h"
#include "search.h"
#include "common.h"
#include "document.h"
#include "handlers.h"
#include <sys/time.h>


/**
 * process_command:
 *   Interpreta a string de comando recebida via FIFO de pedidos,
 *   invoca o handler apropriado e envia a resposta via FIFO de respostas.
 */
void process_command(ClientMessage *msg) {
    char response[MAX_BUF];
    char *cmd = msg->command;
    char *token = strtok(cmd, " ");
    char *args = strtok(NULL, "");

    if (token == NULL) return;

    if (strcmp(token, "SHUTDOWN") == 0) {
        strcpy(response, "Server is shutting down");
        send_response(response, msg->response_fifo);
        
        // Cleanup antes de sair
        cache_cleanup();
        if (save_metadata() != 0) {
            // Log erro ao salvar metadata
            const char *err = "Error saving metadata on shutdown\n";
            write(STDERR_FILENO, err, strlen(err));
        }
        unlink(FIFO_NAME);
        
        // Força saída do processo servidor
        exit(0);
    }
    else if (strcmp(token, "ADD") == 0) {
        handle_add(args, response);    // indexar documento novo
    }
    else if (strcmp(token, "GET") == 0) {
        handle_get(args, response);    // obter metadados
    }
    else if (strcmp(token, "DEL") == 0) {
        handle_del(args, response);    // eliminar índice
    }
    else if (strcmp(token, "COUNT") == 0) {
        handle_count(args, response);  // contar ocorrências
    }
    else if (strcmp(token, "SEARCH") == 0) {
        // pesquisa sequencial ou paralela consoante o nº de processos
        char *keyword        = strtok(args, "|");
        char *proc_count_str = strtok(NULL, "|");
        if (!keyword) {
            strcpy(response, "ERROR Missing keyword");
            send_response(response, msg->response_fifo);
            return;
        }
        int proc_count = proc_count_str ? atoi(proc_count_str) : 1;
        if (proc_count <= 1) {
            // busca sequencial
            handle_search(args, response);
            send_response(response, msg->response_fifo);
        } else {
            // busca paralela (cada fork envia resposta internamente)
            handle_search_command(keyword, proc_count, msg->response_fifo);
            return;
        }
    }
    else {
        // comando desconhecido
        strcpy(response, "ERROR Unknown command");
    }
    // envio da resposta padrão
    send_response(response, msg->response_fifo);
}

/**
 * index_documents_from_folder:
 *   Percorre recursivamente a pasta indicada, indexa todos os ficheiros
 *   regulares usando add_document() com metadados por defeito.
 */
void index_documents_from_folder(const char *folder_path) {
    DIR *dir = opendir(folder_path);
    if (!dir) return;

    struct dirent *entry;
    struct stat st;
    while ((entry = readdir(dir)) != NULL) {
        // ignora "." e ".."
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s",
                 folder_path, entry->d_name);
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            // retira extensão para usar como título
            char *title = strdup(entry->d_name);
            char *dot = strrchr(title, '.');
            if (dot) *dot = '\0';
            // adiciona documento com metadados genéricos
            add_document(title, "Unknown", "0000", full_path);
            free(title);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        write(STDOUT_FILENO,
            "Uso: ./dserver <pasta_documentos> <tamanho_cache>\n",
            48);
        return EXIT_FAILURE;
    }

    // processa tamanho de cache (com valor por defeito se inválido)
    int cache_size = atoi(argv[2]);
    if (cache_size <= 0 || cache_size > MAX_DOCS) {
        write(STDOUT_FILENO,
              "Tamanho de cache inválido. Usando valor por defeito.\n",
              52);
        cache_size = DEFAULT_CACHE_SIZE;
    }

    // inicializa cache LRU
    init_cache(cache_size);
    // cria FIFOs de comunicação (sobrescreve se já existirem)
    mkfifo(FIFO_NAME, 0666);

    // notifica arranque do servidor
    {
        char msg[MAX_BUF];
        int len = snprintf(msg, sizeof(msg),
            "Servidor iniciado com cache size: %d\n",
            cache_size);
        write(STDOUT_FILENO, msg, len);
    }

    // carrega metadados persistidos (podem repovoar 'documents' e 'cache')
    load_metadata();
    // pré-carrega cache com primeiras 'cache_size' entradas ativas
    for (int i = 0; i < doc_count && i < cache_size; i++) {
        if (documents[i].active) {
            cache_put(documents[i]);
        }
    }

    while (1) {
        ClientMessage msg;
        int fd = open(FIFO_NAME, O_RDONLY);
        if (fd == -1) {
            // Adicionar log de erro e pequena espera
            sleep(1);
            continue;
        }

        // Adicionar select para evitar bloqueio permanente
        fd_set readfds;
        struct timeval tv;
        tv.tv_sec = 5;  // timeout de 5 segundos
        tv.tv_usec = 0;

        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        int ready = select(fd + 1, &readfds, NULL, NULL, &tv);
        if (ready < 0) {
            close(fd);
            continue;
        }

        if (ready > 0 && FD_ISSET(fd, &readfds)) {
            int n = read(fd, &msg, sizeof(msg));
            close(fd);
            if (n <= 0) continue;
            
            // Processa comando...
            process_command(&msg);
        } else {
            close(fd);
        }
    }

    return EXIT_SUCCESS;
}