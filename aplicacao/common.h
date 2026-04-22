/**
 * Módulo Comum
 * ------------
 * Definições e constantes partilhadas entre cliente e servidor:
 * tamanhos de buffers, limites de campos, nomes de FIFOs e comandos suportados.
 */

#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>  // pid_t

// Tamanhos máximos de buffers e campos
#define MAX_BUF            512   // Buffer geral para mensagens e respostas 
#define MAX_DOCS          2500   // Número máximo de documentos indexados
#define MAX_TITLE          200   // Comprimento máximo do título
#define MAX_AUTHORS        200   // Comprimento máximo da lista de autores
#define MAX_PATH            64   // Comprimento máximo do caminho do ficheiro
#define MAX_YEAR             5   // Comprimento máximo do ano (ex.: "2025")

// Tamanho padrão da cache em memória (número de entradas)
#define DEFAULT_CACHE_SIZE  10

// Nomes das FIFOs para comunicação entre cliente e servidor
#define FIFO_NAME          "/tmp/doc_service"       // FIFO de pedidos

typedef struct {
    pid_t client_id;          
    char response_fifo[64];   
    char command[MAX_BUF];    
} ClientMessage;

// Remover definição de FIFO_RESPONSE pois cada cliente terá seu próprio
#undef FIFO_RESPONSE

#endif // COMMON_H