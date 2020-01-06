#include <stdio.h>
#include "string.h"
#ifndef UART_NUM_1
// UART driver
#include "driver/uart.h"
#endif

//print on specified UART a text
void scriviUART(uart_port_t uart_controller, char* text);

// read a line from the UART controller
char* read_line(uart_port_t uart_controller);

//print on default UART a text
void stampaStringa(char* line);