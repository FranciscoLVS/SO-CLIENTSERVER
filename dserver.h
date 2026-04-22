/**
 * Módulo Servidor (dserver)
 * -------------------------
 * Declarações das funções principais do serviço de indexação e pesquisa:
 * - process_command: interpreta e executa comandos recebidos via FIFO
 * - index_documents_from_folder: indexa recursivamente ficheiros de uma pasta
 */

#ifndef DSERVER_H
#define DSERVER_H

#include "common.h"    // Definições gerais e comandos suportados
#include "document.h"  // Estruturas e funções de gestão de documentos
#include "cache.h"     // Interface da cache LRU de meta-informação
#include "search.h"    // Protótipos de search_documents e SearchTask
#include "handlers.h"  // Handlers para cada comando (ADD, GET, DEL, ...)

/**
 * process_command:
 *   Recebe uma string de comando (ex.: "ADD ...", "GET ...") vinda da FIFO
 *   de pedidos, invoca o handler apropriado e envia a resposta pela FIFO de resposta.
 *
 * Parâmetros:
 *   cmd - linha de texto com o comando e argumentos, termina em '\0'
 */
void process_command(char *cmd);

/**
 * index_documents_from_folder:
 *   Percorre recursivamente a pasta indicada, identifica ficheiros regulares
 *   e adiciona cada um como documento (com metadados genéricos) via add_document().
 *
 * Parâmetros:
 *   folder_path - caminho para a pasta onde iniciar a indexação
 */
void index_documents_from_folder(const char *folder_path);

#endif // DSERVER_H