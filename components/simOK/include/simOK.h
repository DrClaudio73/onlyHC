#ifndef UART_NUM_1
// UART driver
#include "driver/uart.h"
#endif

#include <stdio.h>
#include "string.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "SIM800-HC12App";

#define BLINK_GPIO CONFIG_BLINK_GPIO
#define RESET_SIM800GPIO CONFIG_RESET_SIM800GPIO
#define UART_SIM800TXGPIO CONFIG_UART_SIM800TXGPIO
#define UART_SIM800RXGPIO CONFIG_UART_SIM800RXGPIO
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
#define NO_PENDING_DTMF_TONE -1
#endif

/*#define NOT_ALLOWED_CALLER -1
#define ALLOWED_CALLER 0
#define NO_CALLER +1*/
enum typeOfCaller {NOT_ALLOWED_CALLER=-1, ALLOWED_CALLER = 0, NO_CALLER =1};
// phone activity status: 0= ready, 2= unknown, 3= ringing, 4= in call
enum phoneActivityStatus {PHONE_READY=0, PHONE_STATUS_UNKNOWN = 2, RINGING =3, CALL_IN_PROGRESS =4};

typedef struct Parsed_Params
{
    int CSQ_qualityLevel;
    int phoneActivityStatus;
    char generic_parametro_feedback[LINE_MAX];
    int pending_DTMF;
    char calling_number[NUMCELL_MAX];
    enum typeOfCaller calling_number_valid; //-1=Number Calling not allowed ; 0=Number Calling allowed; ; +1=No calling number
} parsed_params; //struct gun arnies;

typedef struct Allowed_IDs
{
    char allowed1[LINE_MAX];
    char allowed2[LINE_MAX];
} allowed_IDs; //struct gun arnies;


int checkDTMF(char* line);

char* parse_line(char* line, parsed_params* parametri_globali, allowed_IDs chiamantiAbilitati);

int verificaComando(uart_port_t uart_controller, char* command, char* condition, parsed_params* parametri_globali, allowed_IDs chiamantiAbilitati);

void reset_module(uart_port_t uart_controller, parsed_params* parametri_globali, allowed_IDs chiamantiAbilitati);

//enum typeOfCaller checkCallingNumber(parsed_params* parametri_globali);

int checkPhoneActivityStatus(parsed_params* parametri_globali, allowed_IDs chiamantiAbilitati);

int simOK(uart_port_t uart_controller, parsed_params* parametri_globali, allowed_IDs chiamantiAbilitati);