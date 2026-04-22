/**
 * Módulo Cliente (dclient)
 * ------------------------
 * Declara as funções usadas pelo cliente de linha de comandos
 * para formar pedidos, enviá-los ao servidor via FIFO e apresentar
 * as respostas ao utilizador.
 */

#ifndef DCLIENT_H
#define DCLIENT_H

/**
 * print_usage:
 *   Escreve no stdout as instruções de uso do programa dclient,
 *   detalhando as opções suportadas e a sintaxe de cada comando.
 */
void print_usage(void);

#endif // DCLIENT_H