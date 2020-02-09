#ifndef UART_NUM_1
// UART driver
#include "driver/uart.h"
#endif
#include "messagehandle.h"

#define ADDR_MASTER_STATION CONFIG_ADDR_MASTER_STATION
#define ADDR_SLAVE_STATION CONFIG_ADDR_SLAVE_STATION
#define ADDR_MOBILE_STATION CONFIG_ADDR_MOBILE_STATION
#define DEBUG CONFIG_MIODEBUG

/*When in production
#ifdef CONFIG_ROLEMASTER
    #define STATION_ROLE STATIONMASTER
    #define LOCAL_STATION_ADDRESS ADDR_MASTER_STATION
#endif

#ifdef CONFIG_ROLESLAVE
    #define STATION_ROLE STATIONSLAVE
    #define LOCAL_STATION_ADDRESS ADDR_SLAVE_STATION
#endif

#ifdef CONFIG_ROLEMOBILE
    #define STATION_ROLE STATIONMOBILE
    #define LOCAL_STATION_ADDRESS ADDR_MOBILE_STATION
#endif
*/ 

// ********************************************
//#define DEVOPS_THIS_IS_STATION_MASTER
#define DEVOPS_THIS_IS_STATION_SLAVE
//#define DEVOPS_THIS_IS_STATION_MOBILE

#ifdef DEVOPS_THIS_IS_STATION_MASTER
#define STATION_ROLE STATIONMASTER
#define LOCAL_STATION_ADDRESS ADDR_MASTER_STATION
#endif

#ifdef DEVOPS_THIS_IS_STATION_SLAVE
    #define STATION_ROLE STATIONSLAVE
    #define LOCAL_STATION_ADDRESS ADDR_SLAVE_STATION
#endif

#ifdef DEVOPS_THIS_IS_STATION_MOBILE
    #define STATION_ROLE STATIONMOBILE
    #define LOCAL_STATION_ADDRESS ADDR_MOBILE_STATION
#endif

// ********************************************
#define DR_REG_RNG_BASE 0x3ff75144

#define NUM_PRES_BLINKS CONFIG_NUM_PRES_BLINKS
#define NUM_HANDLED_INPUTS 2
#define GPIO_INPUT_COMMAND_PIN_UNO CONFIG_COMMAND_PIN_UNO
#define GPIO_INPUT_COMMAND_PIN_DUE CONFIG_COMMAND_PIN_DUE
#define GPIO_INPUT_COMMAND_PINS  ((1ULL<<GPIO_INPUT_COMMAND_PIN_UNO) | (1ULL<<GPIO_INPUT_COMMAND_PIN_DUE))
static const char *TAG = "OnlyHC12App";

#define BLINK_GPIO CONFIG_BLINK_GPIO
#define OK_GPIO CONFIG_GPIO_OK
#define NOK_GPIO CONFIG_GPIO_NOK
#define GPIO_OUTPUT_LEDS ((1ULL<<BLINK_GPIO) | (1ULL<<OK_GPIO) | (1ULL<<NOK_GPIO))

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

#define NUM_MAX_CMDS 50 //max number of commands to handle
#define NUM_MAX_RETRIES 5 //numero massimo di volte che provo a riemettere il comando prima di arrendermi
#define NUM_MAX_CHECKS_FOR_ACK 5 //numero massimo di volte che provo a vedere se ho ottenuto l'ACK prima di considerare il comando perso e quindi devo riemetterlo

enum RoleStation {STATIONMASTER=0, STATIONSLAVE = 1, STATIONMOBILE =2};

enum typeOfevent {NOTHING=0, IO_INPUT_ACTIVE=1, RECEIVED_MSG = 2, RECEIVED_ACK= 3, FAIL_TO_SEND_CMD = 4};

typedef struct Valore_Evento_t
{
    unsigned char value_of_input;
    unsigned char input_number;
    unsigned char* cmd_received;
    unsigned char* param_received;
    unsigned char ack_rep_counts;
    unsigned char pair_addr;
} valore_evento_t; 

typedef struct Evento
{
    enum typeOfevent type_of_event;
    valore_evento_t valore_evento;
} evento;

typedef struct Command_Status
{
    unsigned char cmd[FIELD_MAX];
    unsigned char param[FIELD_MAX];
    unsigned char rep_counts; //indice del numero di reinvio del medesimo comando (devono coincidere rep_counts e ack_rep_counts(?!?!)
    unsigned char num_checks; //numero di volte che verifico se il comando+rep_counts è stato ACKnowledgato, altrimenti dichiaro fallito e provo a reinviarlo con rep_counts incrememntato
    unsigned char addr_to;
    unsigned char uart_controller;
} command_status;

////////////////////////////////////////////// CORE FUNCTIONS //////////////////////////////////////////////
void list_commands_status();
void clean_processed_cmds();
unsigned char invia_comando(uart_port_t uart_controller, unsigned char addr_from, unsigned char addr_to, const unsigned char* cmd, const unsigned char* param, unsigned char rep_counts);
unsigned char check_rcved_acks(uart_port_t uart_controller, evento* detected_event);
unsigned char manage_cmd_retries(uart_port_t uart_controller,evento* detected_event);
evento* detect_event(uart_port_t uart_controller);
