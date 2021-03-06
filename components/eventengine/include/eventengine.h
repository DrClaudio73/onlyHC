#ifndef _EVENT_ENGINE_
#define _EVENT_ENGINE_

#include "tipostazione.h"

#define NUM_MAX_CMDS 10                                                             //max number of commands to handle
#define NUM_MAX_RETRIES 5                                                           //numero massimo di volte che provo a riemettere il comando prima di arrendermi
#define NUM_MAX_CHECKS_FOR_ACK 5                                                    //numero massimo di volte che provo a vedere se ho ottenuto l'ACK prima di considerare il comando perso e quindi devo riemetterlo
#define NUM_CHECKS_FOR_ALREADY_RECEIVED NUM_MAX_RETRIES *NUM_MAX_CHECKS_FOR_ACK + 5 //numero di volte che provo a vedere se ho ho già ricevuto questo comando in precedenza (magari con un repcounts inferiore) per scartarlo. Superato questo check considero nuovamente possibile ricevere un comando siffatto

#include <stdio.h>
#include "string.h"
#ifndef UART_NUM_1
// UART driver
#include "driver/uart.h"
#endif
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "cripter.h"

#define ADDR_MASTER_STATION CONFIG_ADDR_MASTER_STATION
#define ADDR_SLAVE_STATION CONFIG_ADDR_SLAVE_STATION
#define ADDR_MOBILE_STATION CONFIG_ADDR_MOBILE_STATION
#define DEBUG CONFIG_MIODEBUG

enum RoleStation
{
    STATIONMASTER = 0,
    STATIONSLAVE = 1,
    STATIONMOBILE = 2
};

//#define DEVOPS_THIS_IS_STATION_MASTER
//#define DEVOPS_THIS_IS_STATION_SLAVE
//#define DEVOPS_THIS_IS_STATION_MOBILE

#ifdef DEVOPS_THIS_IS_STATION_MASTER
#define STATION_ROLE STATIONMASTER
#define LOCAL_STATION_ADDRESS ADDR_MASTER_STATION

#define GPIO_INPUT_COMMAND_PIN_UNO MASTER_GPIO_INPUT_COMMAND_PIN_UNO
#define GPIO_INPUT_COMMAND_PIN_DUE MASTER_GPIO_INPUT_COMMAND_PIN_DUE
#define GPIO_INPUT_COMMAND_PINS MASTER_GPIO_INPUT_COMMAND_PINS
#define BLINK_GPIO MASTER_BLINK_GPIO
#define OK_GPIO MASTER_OK_GPIO
#define NOK_GPIO MASTER_NOK_GPIO

#define GPIO_OUTPUT_PINS MASTER_GPIO_OUTPUT_PINS

#endif

#ifdef DEVOPS_THIS_IS_STATION_SLAVE
#define STATION_ROLE STATIONSLAVE
#define LOCAL_STATION_ADDRESS ADDR_SLAVE_STATION

#define GPIO_INPUT_COMMAND_PIN_UNO SLAVE_GPIO_INPUT_COMMAND_PIN_UNO
#define GPIO_INPUT_COMMAND_PIN_DUE SLAVE_GPIO_INPUT_COMMAND_PIN_DUE
#define GPIO_INPUT_COMMAND_PINS SLAVE_GPIO_INPUT_COMMAND_PINS
#define BLINK_GPIO SLAVE_BLINK_GPIO
#define OK_GPIO SLAVE_OK_GPIO
#define NOK_GPIO SLAVE_NOK_GPIO
#define GPIO_OUTPUT_PINS SLAVE_GPIO_OUTPUT_PINS
#endif

#ifdef DEVOPS_THIS_IS_STATION_MOBILE
#define STATION_ROLE STATIONMOBILE
#define LOCAL_STATION_ADDRESS ADDR_MOBILE_STATION

#define GPIO_INPUT_COMMAND_PIN_UNO MOBILE_GPIO_INPUT_COMMAND_PIN_UNO
#define GPIO_INPUT_COMMAND_PIN_DUE MOBILE_GPIO_INPUT_COMMAND_PIN_DUE
#define GPIO_INPUT_COMMAND_PINS MOBILE_GPIO_INPUT_COMMAND_PINS
#define BLINK_GPIO MOBILE_BLINK_GPIO
#define OK_GPIO MOBILE_OK_GPIO
#define NOK_GPIO MOBILE_NOK_GPIO
#define GPIO_OUTPUT_PINS MOBILE_GPIO_OUTPUT_PINS
#endif

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

enum typeOfevent
{
    NOTHING = 0,
    IO_INPUT_ACTIVE = 1,
    RECEIVED_MSG = 2,
    RECEIVED_ACK = 3,
    FAIL_TO_SEND_CMD = 4
};

typedef struct Valore_Evento_t
{
    unsigned char value_of_input;
    unsigned char input_number;
    unsigned char *cmd_received;
    unsigned char *param_received;
    unsigned char ack_rep_counts;
    unsigned char pair_addr;
    uart_port_t uart_controller;
} valore_evento_t;

typedef struct Evento
{
    enum typeOfevent type_of_event;
    valore_evento_t valore_evento;
} evento_t;

typedef struct Command_Status
{
    unsigned char cmd[FIELD_MAX];
    unsigned char param[FIELD_MAX];
    unsigned char rep_counts; //indice del numero di reinvio del medesimo comando (devono coincidere rep_counts e ack_rep_counts(?!?!)
    unsigned char num_checks; //numero di volte che verifico se il comando+rep_counts è stato ACKnowledgato, altrimenti dichiaro fallito e provo a reinviarlo con rep_counts incrememntato
    unsigned char addr_pair;
    unsigned char uart_controller;
} command_status_t;

typedef struct Commands
{
    unsigned char num_cmd_under_processing;
    command_status_t commands_status[NUM_MAX_CMDS];
} commands_t;

evento_t *detect_event(uart_port_t uart_controller, const gpio_num_t *gpio_input_command_pin, commands_t *my_commands, commands_t *rcv_commands);
unsigned char check_rcved_acks(evento_t *detected_event, commands_t *my_commands);
void clean_cmds_list(commands_t *my_commands, char *name);
unsigned char invia_comando(uart_port_t uart_controller, commands_t *my_commands, unsigned char addr_from, unsigned char addr_to, const unsigned char *cmd, const unsigned char *param, unsigned char rep_counts);
unsigned char invia_ack(uart_port_t uart_controller, commands_t *my_commands, unsigned char addr_from, evento_t *evento);
void list_commands_status(commands_t *my_commands, char *name);
unsigned char manage_issuedcmd_retries(uart_port_t uart_controller, evento_t *detected_event, commands_t *my_commands);
unsigned char manage_rcvcmds_retries(commands_t *rcv_commands);
unsigned char is_rcv_a_new_cmd(commands_t *rcv_commands, evento_t *last_event); //return 0 if already present; 1 if the event is a new command

#endif // _EVENT_ENGINE_