#ifndef UART_NUM_1
// UART driver
#include "driver/uart.h"
#endif

#define ADDR_MASTER_STATION CONFIG_ADDR_MASTER_STATION
#define ADDR_SLAVE_STATION CONFIG_ADDR_SLAVE_STATION
#define ADDR_MOBILE_STATION CONFIG_ADDR_MOBILE_STATION
#define DEBUG CONFIG_MIODEBUG

/*When in production
#ifdef CONFIG_ROLEMASTER
    #define STATION_ROLE STATIONMASTER
#endif

#ifdef CONFIG_ROLESLAVE
    #define STATION_ROLE STATIONSLAVE
#endif

#ifdef CONFIG_ROLEMOBILE
    #define STATION_ROLE STATIONMOBILE
#endif
*/
// ********************************************
#define STATION_ROLE STATIONMASTER
//#define STATION_ROLE STATIONSLAVE
//#define STATION_ROLE STATIONMOBILE
// ********************************************

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
#define NUM_MAX_RETRIES 5 //numero massimo di volte che provo a riemettere il comando prima di arrendermi
#define NUM_LINES_TO_FLUSH 4
#endif //LINE_MAX

enum RoleStation {STATIONMASTER=0, STATIONSLAVE = 1, STATIONMOBILE =2};

enum typeOfevent {NOTHING=0, IO_INPUT_ACTIVE=1, RECEIVED_MSG = 2};

typedef struct Valore_Evento_t
{
    unsigned char value_of_input;
    unsigned char* cmd_received;
    unsigned char* param_received;
    unsigned char ack_rep_counts;
    unsigned char sender;
} valore_evento_t; 

typedef struct Evento
{
    enum typeOfevent id_evento;
    valore_evento_t valore_evento;
} evento;

typedef struct Command_Status
{
    unsigned char cmd[FIELD_MAX];
    unsigned char param[FIELD_MAX];
    unsigned char rep_counts; //indice del numero di reinvio del medesimo comando (devono coincidere rep_counts e ack_rep_counts(?!?!)
    unsigned char num_checks; //numero di volte che verifico se il comando+rep_counts è stato ACKnowledgato, altrimenti dichiaro fallito e provo a reinviarlo con rep_counts incrememntato
    unsigned char addr_to;
} command_status;

unsigned char calcola_CRC(int totale);
void pack_str(unsigned char* msg_to_send, const unsigned char* valore, unsigned char* k, int* totale);
void pack_num(unsigned char* msg_to_send, unsigned char valore, unsigned char* k, int* totale);
unsigned char* strcpy2(unsigned char* dst , const unsigned char* src);
size_t strlen2(const unsigned char* stringa);

//unsigned char* pack_msg(unsigned char uart_controller, unsigned char addr_from, unsigned char addr_to, const unsigned char* cmd, const unsigned char* param);
unsigned char* pack_msg(unsigned char addr_from, unsigned char addr_to, const unsigned char* cmd, const unsigned char* param, unsigned char rep_counts);
unsigned char unpack_msg(const unsigned char* msg, unsigned char allowed_addr_from, unsigned char allowed_addr_to, unsigned char** cmd, unsigned char** param, unsigned char* acknowldged_rep_counts);