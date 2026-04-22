
#include <stdlib.h>
#include "cache.h"

static Cache cache;    // Estrutura global que mantém o estado da cache

/**
 * init_cache:
 *   Inicializa a estrutura de cache com capacidade para max_size entradas.
 *   Define lista vazia e aloca o array de CacheEntry.
 */
void init_cache(int max_size) {
    cache.head = NULL;            // nenhuma entrada usada recentemente
    cache.tail = NULL;            // nenhuma entrada a remover
    cache.size = 0;               // cache vazia
    cache.max_size = max_size;    // capacidade máxima configurada
    cache.entries = malloc(sizeof(CacheEntry) * max_size);  // aloca array de entradas
}

 /**
 * move_to_front:
 *   Move a entrada indicada para o início da lista (mais recentemente usada).
 *   Ajusta ponteiros prev/next para remover a entrada da posição atual.
 */
static void move_to_front(CacheEntry *entry) {
    if (entry == cache.head) return;  // já está no início

    // desliga entry da sua posição atual
    if (entry->prev) entry->prev->next = entry->next;
    if (entry->next) entry->next->prev = entry->prev;
    if (entry == cache.tail) cache.tail = entry->prev;  // tail atualizado

    // insere entry no início
    entry->next = cache.head;
    entry->prev = NULL;
    if (cache.head) cache.head->prev = entry;
    cache.head = entry;
    if (!cache.tail) cache.tail = entry;  // primeira inserção
}

/**
 * cache_get:
 *   Procura uma entrada na cache pelo identificador key.
 *   Em caso de cache hit, move a entrada para o início e retorna-a.
 *   Se não encontrar, retorna NULL.
 */
CacheEntry* cache_get(int key) {
    CacheEntry *entry = cache.head;
    while (entry) {
        if (entry->doc.key == key) {
            move_to_front(entry);
            return entry;
        }
        entry = entry->next;
    }
    return NULL;  // falta na cache
}

/**
 * cache_put:
 *   Insere ou atualiza uma entrada na cache.
 *   Se já existe, atualiza dados e move para início.
 *   Caso a cache esteja cheia, remove a entry menos usada (tail).
 */
void cache_put(Document doc) {
    CacheEntry *existing = cache_get(doc.key);
    if (existing) {
        // já existe na cache: apenas atualiza e move para início
        existing->doc = doc;
        move_to_front(existing);
        return;
    }

    CacheEntry *new_entry;
    if (cache.size < cache.max_size) {
        // ainda há espaço livre: usa próxima posição no array
        new_entry = &cache.entries[cache.size++];
    } else {
        // cache cheia: reutiliza tail como nova entrada e avança tail
        new_entry = cache.tail;
        if (cache.tail->prev) cache.tail->prev->next = NULL;
        cache.tail = cache.tail->prev;
    }

    // preenche e insere new_entry no início da lista
    new_entry->doc = doc;
    new_entry->next = cache.head;
    new_entry->prev = NULL;
    if (cache.head) cache.head->prev = new_entry;
    cache.head = new_entry;
    if (!cache.tail) cache.tail = new_entry;  // primeira inserção
}


/**
 * cache_cleanup:
 *   Liberta memória alocada para a cache e reinicializa a estrutura.
 */
void cache_cleanup(void) {
    free(cache.entries);
    cache.entries = NULL;
    cache.head = NULL;
    cache.tail = NULL;
    cache.size = 0;
}