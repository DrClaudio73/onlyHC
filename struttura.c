MAIN{
    #define DEBUG CONFIG_MIODEBUG
    #define DR_REG_RNG_BASE 0x3ff75144

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
}

EVENTENGINE{
    #include "messagehandle.h"
    enum RoleStation {STATIONMASTER=0, STATIONSLAVE = 1, STATIONMOBILE =2};
    #define DEVOPS_THIS_IS_STATION_MASTER
    //#define DEVOPS_THIS_IS_STATION_SLAVE
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
    #endif    #define ADDR_MASTER_STATION CONFIG_ADDR_MASTER_STATION
    #define ADDR_SLAVE_STATION CONFIG_ADDR_SLAVE_STATION
    #define ADDR_MOBILE_STATION CONFIG_ADDR_MOBILE_STATION

    enum typeOfevent {NOTHING=0, IO_INPUT_ACTIVE=1, RECEIVED_MSG = 2, RECEIVED_ACK= 3, FAIL_TO_SEND_CMD = 4};

    typedef struct Valore_Evento_t valore_evento_t; 
    typedef struct Evento;
    typedef struct Command_Status;
    typedef struct Commands commands;

    unsigned char check_rcved_acks(uart_port_t uart_controller, evento* detected_event, commands* my_commands);
    void clean_processed_cmds(commands* my_commands);
    evento* detect_event(uart_port_t uart_controller, commands* my_commands);
    unsigned char invia_comando(uart_port_t uart_controller, commands* my_commands, unsigned char addr_from, unsigned char addr_to, const unsigned char* cmd, const unsigned char* param, unsigned char rep_counts);
    void list_commands_status(commands my_commands);
    unsigned char manage_cmd_retries(uart_port_t uart_controller, evento* detected_event, commands *my_commands);
}

MESSAHANDLE{
    // max buffer length
    #ifndef LINE_MAX
    #define LINE_MAX 2400
    #define FIELD_MAX 255
    #define NUMCELL_MAX	20
    #endif //LINE_MAX
   
    ////////////////////////////////////////////// MESSAGE HANDLING FUNCTIONS //////////////////////////////////////////////
    int strcmpACK(const unsigned char* rcv, const unsigned char* cmd);
    unsigned char calcola_CRC(int totale);
    void pack_str(unsigned char* msg_to_send, const unsigned char* valore, unsigned char* k, int* totale);
    void pack_num(unsigned char* msg_to_send, unsigned char valore, unsigned char* k, int* totale);
    unsigned char* pack_msg(unsigned char addr_from, unsigned char addr_to, const unsigned char* cmd, const unsigned char* param, unsigned char rep_counts);
    unsigned char unpack_msg(const unsigned char* msg, unsigned char allowed_addr_from, unsigned char allowed_addr_to, unsigned char** cmd, unsigned char** param, unsigned char* acknowldged_rep_counts);
}

AUXILIARY FUNCS{
    void foreverRed(unsigned char blink_gpio)
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

    ////////////////////////////////////////////// AUXILIARY FUNCTIONS //////////////////////////////////////////////
    void foreverRed(unsigned char blink_gpio);
    void presentBlink(unsigned char LedPinNo, unsigned char num_pres_blinks);
}

MANAGEUART{
    #ifndef UART_NUM_1
    // UART driver
    #include "driver/uart.h"
    #endif
    #define LINE_MAX 2400
    int scriviUART(uart_port_t uart_controller, unsigned char* text);
    unsigned char* read_line(uart_port_t uart_controller);
    void stampaStringa(char* line);//print on default UART a text
}

typeconversion{
    #include "string.h"
    ////////////////////////////////////// FUNCTIONS FOR TYPE CONVERSION ////////////////////////////////////
    size_t strlen2(const unsigned char* stringa);
    unsigned char* strcpy2(unsigned char* dst , const unsigned char* src);
    int strcmp2(const unsigned char* first, const unsigned char* second);
    unsigned char* strcat2(unsigned char * destination, const unsigned char * source);
    int strncmp2(const unsigned char* first, const unsigned char* second, size_t n);
}