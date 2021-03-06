#ifndef _MANAGE_UART_
#define _MANAGE_UART_

#include <stdio.h>
#include "string.h"
#ifndef UART_NUM_1
// UART driver
#include "driver/uart.h"
#endif

#define LINE_MAX 2400
//print on specified UART a text
//void scriviUART(uart_port_t uart_controller, char* text);
int scriviUART(uart_port_t uart_controller, unsigned char *text);

// read a line from the UART controller
unsigned char *read_line(uart_port_t uart_controller);

//print on default UART a text
void stampaStringa(char *line);

#endif // _MANAGE_UART_