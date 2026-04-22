/**
 * Módulo de Documentos
 * --------------------
 * Gere a lista de documentos e respectiva metadata:
 * - Grava e carrega metadata em ficheiro (metadata.bin)
 * - Adiciona novos documentos
 * - Recupera documentos por chave, usando cache para acelerar os acessos
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "document.h"
#include "cache.h"

#define METADATA_FILE "./metadata.bin"  // Ficheiro onde persistir metadata

Document documents[MAX_DOCS];  // Array global de documentos
int doc_count = 0;             // Contador de documentos ativos
int next_key = 1;              // Próxima chave disponível

/**
 * save_metadata:
 *   Persiste no disco o número de documentos, a próxima chave e
 *   o array de documentos (apenas os primeiros doc_count).
 */

int save_metadata(void) {
    int fd = open(METADATA_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) return -1;

    // Tenta escrever todos os dados
    ssize_t written = 0;
    written += write(fd, &doc_count, sizeof(int));
    written += write(fd, &next_key, sizeof(int));
    written += write(fd, documents, sizeof(Document) * doc_count);

    close(fd);

    // Verifica se todas as escritas foram bem sucedidas
    return (written == sizeof(int) * 2 + sizeof(Document) * doc_count) ? 0 : -1;
}

/**
 * get_document_by_key:
 *   Procura o documento com a chave 'key'.
 *   1) Tenta obter da cache (hit rápido)
 *   2) Se não existir na cache, procura no array documents,
 *      marca-o na cache e devolve ponteiro.
 *   Retorna NULL se não encontrar ou se inativo.
 */
Document* get_document_by_key(int key) {
    // 1) Verificar na cache
    CacheEntry *cached = cache_get(key);
    if (cached) {
        cached->doc.active = 1;  // garante flag ativa
        return &cached->doc;
    }
    
    // 2) Procurar no array global
    for (int i = 0; i < doc_count; i++) {
        if (documents[i].key == key && documents[i].active) {
            cache_put(documents[i]);  // inserir na cache para próxima vez
            return &documents[i];
        }
    }
    
    return NULL;  // Documento não encontrado ou inativo
}

/**
 * load_metadata:
 *   Lê de disco o ficheiro metadata.bin e popula doc_count,
 *   next_key e o array documents.
 *   Se o ficheiro não existir, deixa tudo em vazio.
 */
void load_metadata(void) {
    int fd = open(METADATA_FILE, O_RDONLY);
    if (fd == -1) return;  // Ficheiro não existe -> nada a carregar

    read(fd, &doc_count, sizeof(int));                       // Lê contador
    read(fd, &next_key, sizeof(int));                        // Lê próxima chave
    read(fd, documents, sizeof(Document) * doc_count);       // Lê documentos

    close(fd);
}

/**
 * add_document:
 *   Cria um novo Document com os dados fornecidos, atribui chave única,
 *   insere no array global, atualiza cache, grava metadata e retorna a chave.
 *   Retorna -1 se ultrapassar MAX_DOCS.
 */
int add_document(char *title, char *authors, char *year, char *path) {
    if (doc_count >= MAX_DOCS) {
        return -1;  // Não há espaço para mais documentos
    }
    
    Document doc = {0};  // Inicializa todos os campos com zero
    doc.key = next_key++;  // Incrementa next_key após atribuir
    doc.active = 1;
    
    // Copiar campos com segurança
    strncpy(doc.title, title, sizeof(doc.title) - 1);
    strncpy(doc.authors, authors, sizeof(doc.authors) - 1);
    strncpy(doc.year, year, sizeof(doc.year) - 1);
    strncpy(doc.path, path, sizeof(doc.path) - 1);
    
    // Adiciona ao array e atualiza persistência
    documents[doc_count++] = doc;
    cache_put(doc);
    
    // Garante que a metadata é salva
    if (save_metadata() != 0) {
        // Se falhar ao salvar, reverte as alterações
        doc_count--;
        next_key--;
        return -1;
    }
    
    return doc.key;
}