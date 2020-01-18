#ifndef UART_NUM_1
// UART driver
#include "driver/uart.h"
#endif

#define ADDR_MASTER_STATION CONFIG_ADDR_MASTER_STATION
#define ADDR_SLAVE_STATION CONFIG_ADDR_SLAVE_STATION
#define ADDR_MOBILE_STATION CONFIG_ADDR_MOBILE_STATION
#define DEBUG CONFIG_MIODEBUG

enum RoleStation {STATIONMASTER=0, STATIONSLAVE = 1, STATIONMOBILE =2};
#ifdef CONFIG_ROLEMASTER
    #define STATION_ROLE 0
#endif

#ifdef CONFIG_ROLESLAVE
    #define STATION_ROLE 1
#endif

#ifdef CONFIG_ROLEMOBILE
    #define STATION_ROLE 2
#endif

#define DR_REG_RNG_BASE 0x3ff75144

#define NUM_PRES_BLINKS CONFIG_NUM_PRES_BLINKS
#define GPIO_INPUT_COMMAND_PIN CONFIG_COMMAND_PIN1
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_COMMAND_PIN)

#include <stdio.h>
#include "string.h"
#include "driver/gpio.h"
#include "esp_log.h"

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
#define LINE_MAX	2400
#define FIELD_MAX 255
#define NUMCELL_MAX	20
#define MAX_ATTEMPTS 22
#define NUM_LINES_TO_FLUSH 4
#endif

unsigned char calcola_CRC(int totale);
void pack_str(unsigned char* msg_to_send, const unsigned char* valore, unsigned char* k, int* totale);
void pack_num(unsigned char* msg_to_send, unsigned char valore, unsigned char* k, int* totale);

//unsigned char* pack_msg(unsigned char uart_controller, unsigned char addr_from, unsigned char addr_to, const unsigned char* cmd, const unsigned char* param);
unsigned char* pack_msg(unsigned char uart_controller, unsigned char addr_from, unsigned char addr_to, const unsigned char* cmd, const unsigned char* param, unsigned char rep_counts);

unsigned char unpack_msg(const unsigned char* msg, unsigned char allowed_addr_from, unsigned char allowed_addr_to, unsigned char** cmd, unsigned char** param, unsigned char* acknowldged_rep_counts);
