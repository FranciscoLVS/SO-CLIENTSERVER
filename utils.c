/**
 * Módulo Utilitários
 * ------------------
 * Funções auxiliares para leitura de ficheiros, manipulação segura de strings
 * e pesquisa de palavras em texto (case-sensitive, deteção de limites de palavra).
 */

#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include "utils.h"

/**
 * is_word_boundary:
 *   Determina se o char 'c' é considerado separador de palavra
 *   (espaço, pontuação ou fim de string).
 */
int is_word_boundary(char c) {
    return isspace(c) || ispunct(c) || c == '\0';
}


static char line_buffer[4096];
static char file_buffer[4096];
static ssize_t bytes_in_buffer = 0;
static ssize_t buffer_pos = 0;

// Adicionar esta nova função:
void reset_read_file_line(void) {
    bytes_in_buffer = 0;
    buffer_pos = 0;
    memset(line_buffer, 0, sizeof(line_buffer));
    memset(file_buffer, 0, sizeof(file_buffer));
}



/**
 * read_file_line:
 *   Lê de 'fd' a próxima linha até '\n' ou EOF.
 *   - buf: buffer de trabalho para leitura (não diretamente usado)
 *   - buf_size: tamanho de 'buf'
 *   - line_size: tamanho real da linha lida (output)
 *   Devolve ponteiro para uma string estática com o conteúdo da linha
 *   (sem '\n') ou NULL no fim de ficheiro.
 */

char* read_file_line(int fd, char *buf, size_t buf_size, size_t *line_size) {
    *line_size = 0;
    line_buffer[0] = '\0';
    
    while (1) {
        // Se esgotou o bloco, lê novo bloco de dados
        if (buffer_pos >= bytes_in_buffer) {
            bytes_in_buffer = read(fd, file_buffer, sizeof(file_buffer));
            buffer_pos = 0;
            if (bytes_in_buffer <= 0) {  // EOF ou erro
                if (*line_size > 0) {    // ainda há dados para devolver
                    line_buffer[*line_size] = '\0';
                    return line_buffer;
                }
                return NULL;            // nada mais para ler
            }
        }
        // Processa cada carácter do bloco
        while (buffer_pos < bytes_in_buffer) {
            char c = file_buffer[buffer_pos++];
            if (c == '\n') {            // fim de linha
                line_buffer[*line_size] = '\0';
                return line_buffer;
            }
            if (*line_size < (ssize_t)(sizeof(line_buffer) - 1)) {
                line_buffer[(*line_size)++] = c;  // adiciona carácter
            }
        }
    }
}

/**
 * safe_strcat:
 *   Concatena 'src' a 'dest' sem exceder 'dest_size - 1' caracteres,
 *   garantindo terminação nula.
 *   Devolve o novo comprimento de 'dest'.
 */
size_t safe_strcat(char *dest, const char *src, size_t dest_size) {
    size_t dest_len = strlen(dest);
    size_t i;
    for (i = 0; src[i] && dest_len + i < dest_size - 1; i++) {
        dest[dest_len + i] = src[i];
    }
    dest[dest_len + i] = '\0';
    return dest_len + i;
}

/**
 * remaining_buffer_size:
 *   Calcula quantos caracteres ainda cabem em 'buffer' de tamanho total_size.
 *   Devolve espaço livre para escrita (exclui terminador '\0').
 */
size_t remaining_buffer_size(char *buffer, size_t total_size) {
    size_t used = strlen(buffer);
    return (used < total_size) ? total_size - used - 1 : 0;
}

/**
 * word_match:
 *   Verifica se 'keyword' ocorre em 'text' como palavra inteira,
 *   respeitando limites de palavra.
 *   - case_sensitive: 1 = distingue maiúsculas/minúsculas; 0 = ignora diferenças de caso.
 *
 *   A função percorre todas as ocorrências possíveis. Retorna 1 se encontrar a
 *   palavra completa; 0 caso contrário.
 */


int word_match(const char *text, const char *keyword, int case_sensitive) {
    if (!text || !keyword) return 0;
    
    char *text_copy = strdup(text);
    char *keyword_copy = strdup(keyword);
    int found = 0;

    if (!text_copy || !keyword_copy) {
        free(text_copy);
        free(keyword_copy);
        return 0;
    }

    // Converter para minúsculas se não for case sensitive
    if (!case_sensitive) {
        for (size_t i = 0; text_copy[i]; i++)
            text_copy[i] = tolower(text_copy[i]);
        for (size_t i = 0; keyword_copy[i]; i++)
            keyword_copy[i] = tolower(keyword_copy[i]);
    }

    size_t key_len = strlen(keyword_copy);
    char *pos = text_copy;

    // Procurar todas as ocorrências da palavra
    while ((pos = strstr(pos, keyword_copy)) != NULL) {
        int idx = pos - text_copy;
        
        // Verificar limites da palavra
        int valid_start = (idx == 0 || is_word_boundary(text_copy[idx - 1]));
        int valid_end = is_word_boundary(pos[key_len]);

        if (valid_start && valid_end) {
            found = 1;
            break;
        }

        // Move para depois da palavra encontrada
        pos++;
    }

    free(text_copy);
    free(keyword_copy);
    return found;
}

/**
 * file_contains:
 *   Verifica se o ficheiro em 'filepath' contém 'keyword' como palavra
 *   completa (case-sensitive). Retorna 1 em caso afirmativo, 0 caso não.
 */
int file_contains(const char *filepath, const char *keyword) {
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) return 0;

    reset_read_file_line();  
    char buf[4096];
    size_t line_size;
    int found = 0;
    char *line;

    while ((line = read_file_line(fd, buf, sizeof(buf), &line_size)) != NULL) {
        if (word_match(line, keyword, 1)) {
            found = 1;
            break;
        }
    }
    close(fd);
    return found;
}

/**
 * count_occurrences:
 *   Conta quantas vezes 'keyword' ocorre no ficheiro 'filepath'
 *   como palavra inteira (case-sensitive). Retorna o count ou -1 em erro.
 */

 
 int count_occurrences(const char *filepath, const char *keyword) {
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) return -1;
    
    reset_read_file_line();  
    char buf[4096];
    size_t line_size;
    int count = 0;
    char *line;

    while ((line = read_file_line(fd, buf, sizeof(buf), &line_size)) != NULL) {
        // Se a linha contém a palavra, incrementa o contador
        // apenas verifica se a linha contém a palavra
        if (word_match(line, keyword, 1)) {
            count++; // Incrementa uma vez por linha
        }
    }

    close(fd);
    return count;
}