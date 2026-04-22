#include <unistd.h>   // read, write, STDIN_FILENO, STDERR_FILENO, STDOUT_FILENO
#include <stdlib.h>   // exit, atoi
#include <string.h>   // strcmp, strlen, snprintf, strtok_r
#include <fcntl.h>    // open, O_WRONLY, O_RDONLY
#include <stdio.h>    // perror
#include <sys/types.h>  // pid_t
#include <sys/stat.h>   // mkfifo
#include <sys/select.h> // select, FD_SET, FD_ISSET, FD_ZERO
#include "common.h"  // Definições gerais (MAX_BUF, FIFO_NAME, etc.)


// Imprime instruções de uso do cliente, só mesmo para ajudar o utilizador
void print_usage() {
    const char *usage =
        "Uso:\n"
        "  Adicionar doc:  dclient -a \"title\" \"authors\" \"year\" \"path\"\n"
        "  Consultar doc:  dclient -c \"key\"\n"
        "  Apagar doc:     dclient -d \"key\"\n"
        "  Contar linhas:  dclient -l \"key\" \"keyword\"\n"
        "  Pesquisar:      dclient -s \"keyword\" [nr_processes]\n"
        "  Desligar srv:   dclient -f\n";
    write(STDOUT_FILENO, usage, strlen(usage));  // escreve no stdout
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();        // sem argumentos suficientes -> mostra ajuda
        return EXIT_FAILURE;
    }

    // Tenta abrir FIFO de pedidos para escrever
    int fd = open(FIFO_NAME, O_WRONLY);
    if (fd == -1) {
        const char *err_msg = "Erro: servidor não está em execução\n";
        write(STDERR_FILENO, err_msg, strlen(err_msg));
        return EXIT_FAILURE;
    }

    char buf[MAX_BUF] = {0};  // buffer para mensagem ao servidor
    char response[MAX_BUF];  // Adicionar declaração do buffer de resposta

    // Processa opções do cliente
    if (strcmp(argv[1], "-a") == 0 && argc == 6) {
        // -a: ADD title|authors|year|path
        snprintf(buf, MAX_BUF, "ADD %s|%s|%s|%s", 
                 argv[2], argv[3], argv[4], argv[5]);
    }
    else if (strcmp(argv[1], "-c") == 0 && argc == 3) {
        // -c: GET key
        snprintf(buf, MAX_BUF, "GET %s", argv[2]);
    }
    else if (strcmp(argv[1], "-d") == 0 && argc == 3) {
        // -d: DEL key
        snprintf(buf, MAX_BUF, "DEL %s", argv[2]);
    }
    else if (strcmp(argv[1], "-l") == 0 && argc == 4) {
        // -l: COUNT key|keyword
        snprintf(buf, MAX_BUF, "COUNT %s|%s", argv[2], argv[3]);
    }
    else if (strcmp(argv[1], "-s") == 0) {
        // -s: SEARCH keyword|nr_processes
        if (argc == 3) {
            snprintf(buf, MAX_BUF, "SEARCH %s|1", argv[2]);     // default 1 processo
        } else if (argc == 4) {
            snprintf(buf, MAX_BUF, "SEARCH %s|%s", argv[2], argv[3]);
        } else {
            print_usage();
            close(fd);
            return EXIT_FAILURE;
        }
    }
    else if (strcmp(argv[1], "-f") == 0 && argc == 2) {
        // -f: SHUTDOWN servidor
        snprintf(buf, MAX_BUF, "SHUTDOWN");
    }
    else {
        // argumentos inválidos
        print_usage();
        close(fd);
        return EXIT_FAILURE;
    }

    char response_fifo[64];
    snprintf(response_fifo, sizeof(response_fifo), 
             "/tmp/doc_service_resp_%d", getpid());
    mkfifo(response_fifo, 0666);

    ClientMessage msg = {
        .client_id = getpid()
    };
    strncpy(msg.response_fifo, response_fifo, sizeof(msg.response_fifo));
    strncpy(msg.command, buf, sizeof(msg.command));

    // Enviar mensagem estruturada
    write(fd, &msg, sizeof(msg));
    close(fd);

    // Ler do FIFO próprio
    fd = open(response_fifo, O_RDONLY);
    if (fd == -1) {
        unlink(response_fifo);
        const char *err_msg = "Erro ao abrir FIFO de resposta\n";
        write(STDERR_FILENO, err_msg, strlen(err_msg));
        return EXIT_FAILURE;
    }

    // Adicionar timeout na leitura para evitar bloqueio indefinido
    struct timeval tv;
    tv.tv_sec = 30;  // timeout de 30 segundos
    tv.tv_usec = 0;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    int ready = select(fd + 1, &readfds, NULL, NULL, &tv);
    if (ready <= 0) {
        close(fd);
        unlink(response_fifo);
        const char *err_msg = ready == 0 ? "Timeout na resposta do servidor\n" 
                                        : "Erro ao esperar resposta\n";
        write(STDERR_FILENO, err_msg, strlen(err_msg));
        return EXIT_FAILURE;
    }

    // Só lê se select indicou dados disponíveis
    if (FD_ISSET(fd, &readfds)) {
        read(fd, response, MAX_BUF);
    }
    close(fd);

    // Limpar FIFO no final
    unlink(response_fifo);

    // Processar e mostrar resposta
    if (strcmp(argv[1], "-c") == 0) {
        // para GET: response é "title|authors|year|path"
        char *saveptr;
        char *title  = strtok_r(response, "|", &saveptr);
        char *authors= strtok_r(NULL, "|", &saveptr);
        char *year   = strtok_r(NULL, "|", &saveptr);
        char *path   = strtok_r(NULL, "|", &saveptr);
        if (title && authors && year && path) {
            // Formata saída no estilo pedido
            char out[MAX_BUF];
            int len = snprintf(out, MAX_BUF,
                "Title: %s\nAuthors: %s\nYear: %s\nPath: %s\n",
                title, authors, year, path);
            write(STDOUT_FILENO, out, len);
        }
    } else {
        // para outras operações: imprime resposta direta + nova linha
        write(STDOUT_FILENO, response, strlen(response));
        write(STDOUT_FILENO, "\n", 1);
    }

    return EXIT_SUCCESS;
}