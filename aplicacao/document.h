/**
 * Módulo Documentos
 * -----------------
 * Declara a estrutura Document e as funções de gestão de metadata:
 * - persistência em ficheiro (metadata.bin)
 * - adição de novos documentos
 * - carregamento e consulta com suporte a cache
 */

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include "common.h"   // Definições gerais (MAX_*)

/// Estrutura que guarda toda a meta-informação de um documento
typedef struct {
    int   key;                           // Chave única do documento
    char  title[MAX_TITLE];              // Título do documento
    char  authors[MAX_AUTHORS];          // Lista de autores
    char  year[MAX_YEAR];                // Ano de publicação
    char  path[MAX_PATH];                // Caminho para o ficheiro
    int   active;                        // Flag: 1 = ativo, 0 = eliminado
} Document;

// Array global de documentos indexados em memória
extern Document documents[MAX_DOCS];
// Número de documentos atualmente ativos em 'documents'
extern int      doc_count;
// Próxima chave única a atribuir a um novo documento
extern int      next_key;

/**
 * save_metadata:
 *   Persiste em disco (metadata.bin) o estado atual.
 *   Retorna 0 em caso de sucesso, -1 em caso de erro.
 */
int save_metadata(void);

/**
 * load_metadata:
 *   Carrega de disco (metadata.bin) o estado guardado em:
 *   - doc_count
 *   - next_key
 *   - array documents
 *   Se o ficheiro não existir, deixa tudo em vazio.
 */
void load_metadata(void);

/**
 * add_document:
 *   Adiciona um novo documento à lista, copia os campos fornecidos,
 *   atribui chave única, marca ativo e persiste em cache e disco.
 *   Retorna a chave atribuída, ou -1 se exceder MAX_DOCS.
 */
int add_document(char *title, char *authors, char *year, char *path);

/**
 * get_document_by_key:
 *   Procura documento por chave:
 *   1) tenta cache (cache_get)
 *   2) se não em cache, pesquisa em documents[] e injeta na cache
 *   Retorna NULL se não encontrar ou se documento estiver inativo.
 */
Document* get_document_by_key(int key);

#endif // DOCUMENT_H