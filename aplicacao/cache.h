/**
 * Módulo de Cache
 * ----------------
 * Implementa uma cache em memória para metadata de documentos,
 * utilizando uma lista duplamente ligada para manter a ordem de uso
 * (Least Recently Used - LRU), ouy seja, a entrada mais antiga
 * é removida quando a cache atinge sua capacidade máxima.
 * Suporta operações de inicialização, pesquisa, inserção e limpeza da cache.
 */

#ifndef CACHE_H
#define CACHE_H

#include "common.h"    // Definições gerais (MAX_BUF, MAX_DOCS, etc.)
#include "document.h"  // Estrutura Document e array global de documentos

/**
 * CacheEntry:
 *   Estrutura que representa uma entrada na cache em memória.
 *   Utiliza lista duplamente ligada para manter ordem de uso através de uma política LRU.
 */

typedef struct CacheEntry {
    Document doc;             // Meta-informação do documento
    struct CacheEntry *next;  // Próxima entrada na lista (mais antiga)
    struct CacheEntry *prev;  // Entrada anterior na lista (mais recente)
} CacheEntry;

/**
 * Cache:
 *   Estrutura principal da cache que mantém:
 *   - head: entrada mais recentemente usada
 *   - tail: entrada menos recentemente usada
 *   - size: número atual de entradas
 *   - max_size: capacidade máxima de entradas
 *   - entries: array de CacheEntry
 */
typedef struct {
    CacheEntry *head;    // Entrada mais recentemente usada
    CacheEntry *tail;    // Entrada menos recentemente usada
    int size;            // Contagem atual de entradas
    int max_size;        // Capacidade máxima de entradas
    CacheEntry *entries; // Array de CacheEntry
} Cache;

/**
 * init_cache:
 *   Inicializa a cache, reservando espaço para 'max_size' entradas.
 */
void init_cache(int max_size);

/**
 * cache_put:
 *   Insere ou atualiza a entrada de um documento na cache.
 *   Se atingir a capacidade máxima, remove a entrada LRU (menos usada).
 */
void cache_put(Document doc);


/**
 * cache_get:
 *   Procura uma entrada na cache pelo identificador (key) do documento.
 *   Retorna ponteiro para a entrada em caso de cache hit, ou NULL caso contrário.
 */
CacheEntry* cache_get(int key);

/**
 * cache_cleanup:
 *   Liberta toda a memória usada pela cache e reinicializa os recursos.
 */
void cache_cleanup(void);

#endif // CACHE_H