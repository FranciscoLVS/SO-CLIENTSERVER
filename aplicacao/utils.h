/**
 * Módulo Utilitários
 * ------------------
 * Declara funções auxiliares para leitura de ficheiros, manipulação segura
 * de strings e pesquisa de palavras em texto (case-sensitive, deteção de
 * limites de palavra).
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <ctype.h>

/**
 * is_word_boundary:
 *   Determina se o carácter 'c' é um separador de palavra
 *   (espaço, pontuação ou fim de string).
 *
 * Parâmetro:
 *   c - char a testar
 * Retorno:
 *   1 se 'c' for separador; 0 caso contrário.
 */
int is_word_boundary(char c);

/**
 * read_file_line:
 *   Lê do descritor 'fd' a próxima linha até '\n' ou EOF.
 *   Utiliza buffers internos para otimizar leituras de bloco.
 *
 * Parâmetros:
 *   fd         – descritor de ficheiro aberto para leitura
 *   buf        – buffer de trabalho (tamanho buf_size)
 *   buf_size   – tamanho de 'buf'
 *   line_size  – saída: número de caracteres lidos (sem '\n')
 *
 * Retorno:
 *   Ponteiro para buffer estático contendo a linha (sem '\n'),
 *   ou NULL em caso de EOF sem dados.
 */
char* read_file_line(int fd, char *buf, size_t buf_size, size_t *line_size);

/**
 * safe_strcat:
 *   Concatena 'src' a 'dest' sem ultrapassar 'dest_size - 1' bytes,
 *   garantindo terminação nula.
 *
 * Parâmetros:
 *   dest      – string de destino, deve estar inicializada
 *   src       – string a concatenar
 *   dest_size – capacidade total de 'dest'
 *
 * Retorno:
 *   Comprimento final de 'dest'.
 */
size_t safe_strcat(char *dest, const char *src, size_t dest_size);

/**
 * remaining_buffer_size:
 *   Calcula quantos bytes ainda cabem em 'buffer' (excluindo o '\0').
 *
 * Parâmetros:
 *   buffer     – string cujo comprimento é considerado
 *   total_size – capacidade total do buffer
 *
 * Retorno:
 *   Número de bytes livres antes de atingir total_size-1.
 */

size_t remaining_buffer_size(char *buffer, size_t total_size);

/**
 * word_match:
 *   Verifica se 'keyword' ocorre em 'text' como palavra inteira,
 *   respeitando limites de palavra.
 *
 * Parâmetros:
 *   text           – linha de texto para busca
 *   keyword        – palavra a procurar
 *   case_sensitive – 1 = distingue maiúsculas/minúsculas;
 *                    0 = ignora diferenças de caso
 *
 * Retorno:
 *   1 se encontrar a palavra completa; 0 caso contrário.
 */
int word_match(const char *text, const char *keyword, int case_sensitive);

/**
 * file_contains:
 *   Verifica se o ficheiro em 'filepath' contém 'keyword' como palavra
 *   inteira (case-sensitive).
 *
 * Parâmetros:
 *   filepath – caminho para o ficheiro a ler
 *   keyword  – palavra a procurar
 *
 * Retorno:
 *   1 se encontrar pelo menos uma ocorrência; 0 caso contrário.
 */
int file_contains(const char *filepath, const char *keyword);

/**
 * count_occurrences:
 *   Conta quantas vezes 'keyword' ocorre no ficheiro em 'filepath'
 *   como palavra inteira (case-sensitive).
 *
 * Parâmetros:
 *   filepath – caminho para o ficheiro a ler
 *   keyword  – palavra a contar
 *
 * Retorno:
 *   Número de ocorrências encontradas, ou -1 em caso de erro de leitura.
 */
int count_occurrences(const char *filepath, const char *keyword);

#endif // UTILS_H