
#define ADDR_MASTER_STATION 1
#define ADDR_SLAVE_STATION 2
#define ADDR_MOBILE_STATION 3
#define DEBUG 1

#define NUM_PRES_BLINKS CONFIG_NUM_PRES_BLINKS
#define GPIO_INPUT_COMMAND_PIN CONFIG_COMMAND_PIN1
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_COMMAND_PIN)

#include <stdio.h>
#include "string.h"

static const char *TAG = "OnlyHC12App";

#define BLINK_GPIO CONFIG_BLINK_GPIO
#define HC12SETGPIO CONFIG_HC12SETGPIO
#define HC12TXGPIO CONFIG_HC12TXGPIO
#define HC12RXGPIO CONFIG_HC12RXGPIO
#define CH_NO_HC12  CONFIG_CH_NO_HC12

#ifdef CONFIG_BAUDTRANS1200
    #define BAUDETRANSPARENTMODE 1200
#endif
#ifdef CONFIG_BAUDTRANS2400
    #define BAUDETRANSPARENTMODE 2400
#endif
#ifdef CONFIG_BAUDTRANS4800
    #define BAUDETRANSPARENTMODE 4800
#endif
#ifdef CONFIG_BAUDTRANS9600
    #define BAUDETRANSPARENTMODE 9600
#endif
#ifdef CONFIG_BAUDTRANS19200
    #define BAUDETRANSPARENTMODE 19200
#endif
#ifdef CONFIG_BAUDTRANS38400
    #define BAUDETRANSPARENTMODE 38400
#endif
#ifdef CONFIG_BAUDTRANS57600
    #define BAUDETRANSPARENTMODE 57600
#endif
#ifdef CONFIG_BAUDTRANS115200
    #define BAUDETRANSPARENTMODE 115200
#endif

#ifdef CONFIG_SIM800MODULE
    #define RESET_IDLE 1
    #define RESET_ACTIVE 0
#endif

#ifdef CONFIG_SIM808MODULE
    #define RESET_IDLE 0
    #define RESET_ACTIVE 1
#endif

// max buffer length
#ifndef LINE_MAX
#define LINE_MAX	123
#define NUMCELL_MAX	20
#define MAX_ATTEMPTS 22
#define NUM_LINES_TO_FLUSH 4
#endif

unsigned char calcola_CRC(int totale);
void pack_str(unsigned char* msg_to_send, const unsigned char* valore, unsigned char* k, int* totale);
void pack_num(unsigned char* msg_to_send, unsigned char valore, unsigned char* k, int* totale);

unsigned char* pack_msg(unsigned char uart_controller, unsigned char addr_from, unsigned char addr_to, const unsigned char* cmd, const unsigned char* param);
unsigned char unpack_msg(const unsigned char* msg, unsigned char allowed_addr_from, unsigned char allowed_addr_to, unsigned char** cmd, unsigned char** param);
