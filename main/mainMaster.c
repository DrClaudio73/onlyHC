//automatically pick up any incoming call after 1 ring: ATS0=1
//cfun=1,1 (full,rst)
//AT+CREG? if answer is 0,1 then is registered on home network
/*
int checkStatus() { // CHECK STATUS FOR RINGING or IN CALL

    sim800.println(“AT+CPAS”); // phone activity status: 0= ready, 2= unknown, 3= ringing, 4= in call
    delay(100);
    if (sim800.find(“+CPAS: “)) { // decode reply
        char c = sim800.read(); // gives ascii code for status number
        // simReply(); // should give ‘OK’ in Serial Monitor
        return c – 48; // return integer value of ascii code
    }
        else return -1;
}
*/

#include <stdio.h>
// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// UART driver
#include "driver/uart.h"

// Error library
#include "esp_err.h"

#include "string.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

//Personal modules includes
#include "funzioni.h"
#include "manageUART.h"
static char tag[] = "masterOnlyHC12";
char command[LINE_MAX];

int kkk=0;
int ledus=0;

unsigned char pp=0; //temporaty to be deleted
unsigned char ok=0;
unsigned char ko=0;

size_t strlen2(const unsigned char* stringa){
    return(strlen((const char*) stringa));
}
unsigned char calcola_CRC(int totale) {
    return((unsigned char) totale%256);
}

void pack_str(unsigned char* msg_to_send, const unsigned char* valore, unsigned char* k, int* totale){
    //printf("----START pack str----\r\n");
    for (int i=0; i<strlen2(valore); i++){
        msg_to_send[*k]=valore[i];
        *totale+=msg_to_send[*k];
        //printf("val: %c, val: %d , tot=%d, k=%d;\r\n",msg_to_send[*k], msg_to_send[*k], *totale, *k);
        *k=*k+1;
    }
    //printf("----END pack str----\r\n");
    return;
}

void pack_num(unsigned char* msg_to_send, unsigned char valore, unsigned char* k, int* totale){
        //printf("----START pack num----\r\n");
        msg_to_send[*k]=valore;
        *totale+=msg_to_send[*k];
        //printf("val: %c, val: %d , tot=%d, k=%d;\r\n",msg_to_send[*k], msg_to_send[*k], *totale, *k);
        *k=*k+1;
        //printf("----END pack num----\r\n");
    return;
}

unsigned char* pack_msg(unsigned char addr_from, unsigned char addr_to, const unsigned char* cmd, const unsigned char* param, unsigned char rep_counts) {
    static unsigned char msg_to_send[LINE_MAX];

    //unsigned char expectedACK[255];
    //unsigned char num_max_attempts=5;
    //unsigned char *line_read;
    //short success;

    memset(msg_to_send,0,255);

    //Frame structure
    //CMD: 'B'+'G'+F7+ADDR_FROM(1 byte)+ADDR_TO(1 byte)+REP_COUNTS+CMD_LEN(1)+PARAM_LEN(1)+CMD(var byte)+PARAM(var byte)+CRC(1 byte)+7F
    //ANSWER: 'B'+'G'+F7+ADDR_FROM(1 byte)+ADDR_TO(1 byte)+ACKNOWLEDGED_REP_COUNTS+CMD_LEN(1)+PARAM_LEN(1)+CMD(var byte)+PARAM(var byte)+CRC(1 byte)+7F
    //CRC is evaluated starting at ADDDR_FROM up to PARAM

    //build message
    int totale = 0;
    unsigned char k=0;
    msg_to_send[k++]='B'; //for synch purpose
    msg_to_send[k++]='G'; //for synch purpose
    msg_to_send[k++]=(unsigned char) 0xf7; //start bye
    pack_num(msg_to_send,addr_from,&k,&totale); 
    pack_num(msg_to_send,addr_to,&k,&totale);
    pack_num(msg_to_send,rep_counts,&k,&totale);
    pack_num(msg_to_send,strlen2(cmd),&k,&totale);
    pack_num(msg_to_send,strlen2(param),&k,&totale);
    pack_str(msg_to_send,cmd,&k,&totale);
    pack_str(msg_to_send,param,&k,&totale);
    msg_to_send[k++]=calcola_CRC(totale);
    msg_to_send[k]=(unsigned char) 0x7f;

    if (DEBUG) {
        for(int i=0 ; i<strlen2(msg_to_send); i++){
        printf("*%d",msg_to_send[i]);
        }
        printf("*");
        printf("\r\n");
        for(int i=0 ; i<strlen2(msg_to_send); i++){
        printf("*%x",msg_to_send[i]);
        }
        printf("*");
        printf("\r\n");
        for(int i=0 ; i<strlen2(msg_to_send); i++){
        printf("*%c",msg_to_send[i]);
        }
        printf("*");
        printf("\r\n");
    }

    return msg_to_send;
/*
    sprintf(pino,"Ciao: %d!\n",level);
    sprintf(expectedACKpino,"ACKCiao: %d!!",level);

    for (int k=25; k>=0; k--){
        line_read =read_line(uart_controller);
        printf("******flushed line: %s",line_read);
    }

    success=false;
    for (int k=num_max_attempts;k>=0; k--){
        scriviUART(uart_controller, pino);
        printf("writeHC12 printed %s on %d UART\n", pino, uart_controller);
        vTaskDelay(500 / portTICK_RATE_MS);
        line_read =read_line(uart_controller);
        printf("lineread: %s\n",line_read);
        printf("epectedACK: %s\n",expectedACKpino);
        
        if(strncmp(expectedACKpino,line_read,strlen(expectedACKpino))==0){
            printf("Strings are equal. What a success!!!!!\n");
            success=true;
            k=-1;
            ESP_LOGD(tag, "Transmitted data");
        } else {
            ESP_LOGD(tag, "Transmitted NOT data");
            printf("String are NOT equal. What a nightmare!!!!!\n");
        }
    }
    //success=true; //STUB always TRUE until ACK check is perfrmed see line ABOVE

    //myRadio.printDetails();
    //ESP_LOGD(tag, "Transmitted data");
    return(success);*/
}


unsigned char unpack_msg(const unsigned char* msg, unsigned char allowed_addr_from, unsigned char allowed_addr_to, unsigned char** cmd, unsigned char** param, unsigned char* acknowldged_rep_counts){
    //CMD: 'B'+'G'+F7+ADDR_FROM(1 byte)+ADDR_TO(1 byte)+REP_COUNTS+CMD_LEN(1)+PARAM_LEN(1)+CMD(var byte)+PARAM(var byte)+CRC(1 byte)+7F
    //ANSWER: 'B'+'G'+F7+ADDR_FROM(1 byte)+ADDR_TO(1 byte)+ACKNOWLEDGED_REP_COUNTS+CMD_LEN(1)+PARAM_LEN(1)+CMD(var byte)+PARAM(var byte)+CRC(1 byte)+7F
    //CRC is evaluated starting at ADDDR_FROM up to PARAM
    // RETCODES
    // 0 = OK
    // 1 = INVALID CRC
    // 2 = MESSAGE RECEIVED FROM NOT allowed_addr_from
    // 3 = MESSAGE ADDRESSED to OTHER STATION
    // 4 = MESSAGE NOT PROPERLY TERMINATED
    // 5 = MESSAGE SHORTER THAN EXPECTED

    //printf("\r\nunpack_msg(): ----START UNpack str----\r\n");
    static unsigned char cmd_rcved[255];
    static unsigned char param_rcved[255];
    memset(cmd_rcved,0,255);
    memset(param_rcved,0,255);
    *cmd=cmd_rcved;
    *param=param_rcved;

    unsigned char received_CRC=0;
    
    unsigned char found_startCHR=0;
    unsigned char result=5;
    unsigned char validated_addr_from=0;
    unsigned char validated_addr_to=0;
    unsigned char extracted_rep_counts=0;
    unsigned char extracted_cmd_len=0;
    unsigned char extracted_param_len=0;
    unsigned char cmd_len=0;
    unsigned char param_len=0;
    unsigned char k=0;
    unsigned char j=0;
    unsigned char CRC_extracted=0;
    int totale=0;

    for (unsigned char i=0; i<strlen2(msg);i++) {
        //printf("unpack_msg(): msgrcv[%d]= %d - %c\r\n", i, msg[i], msg[i]);

        if (found_startCHR){
            if (validated_addr_from && validated_addr_to && extracted_rep_counts && extracted_cmd_len && extracted_param_len && (k==cmd_len) && (j==param_len) && CRC_extracted){
                if (msg[i]==0x7f){
                    result=0;
                    return result;
                } else {
                    result=4;
                    return result;
                }
            }
            
            if (validated_addr_from && validated_addr_to && extracted_rep_counts && extracted_cmd_len && extracted_param_len && (k==cmd_len) && (j==param_len)){
                CRC_extracted=1;
                received_CRC=msg[i];
                if (!(received_CRC==calcola_CRC(totale))){
                    result=1;
                    return result;
                } else {
                    printf("unpack_msg(): CRC has been CORRECTLY evaluated (%d)! \r\n",received_CRC);
                }
            }

            if (validated_addr_from && validated_addr_to && extracted_rep_counts && extracted_cmd_len && extracted_param_len){
                if ((k==cmd_len)&&(j<param_len)){
                    param_rcved[j++]=msg[i];
                    totale+=msg[i];
                }
                if (k<cmd_len){
                    cmd_rcved[k++]=msg[i];
                    totale+=msg[i];
                }
            }

            if (validated_addr_from && validated_addr_to && extracted_rep_counts && extracted_cmd_len && !extracted_param_len){
                param_len=msg[i];
                totale+=msg[i];
                extracted_param_len=1;
                printf("unpack_msg(): param_len=%d\r\n",param_len);
            }

            if (validated_addr_from && validated_addr_to && extracted_rep_counts && !extracted_cmd_len){
                cmd_len=msg[i];
                totale+=msg[i];
                extracted_cmd_len=1;
                printf("unpack_msg(): cmd_len=%d\r\n",cmd_len);
            }

            if (validated_addr_from && validated_addr_to && !extracted_rep_counts){
                *acknowldged_rep_counts=msg[i];
                totale+=msg[i];
                extracted_rep_counts=1;
                printf("unpack_msg(): rep_counts=%d\r\n",*acknowldged_rep_counts);
            }

            if((validated_addr_from) && (!validated_addr_to)){
                if (msg[i]==allowed_addr_to){
                    validated_addr_to=1;
                    totale+=msg[i];
                    printf("unpack_msg(): Valid addr_to found: %d\r\n", msg[i]);
                } else {
                    printf("unpack_msg(): message was addressed to other station: %d\r\n", msg[i]);
                    result = 3;
                    return result;            
                }
            }

            if(!validated_addr_from){
                if (msg[i]==allowed_addr_from){
                    validated_addr_from=1;
                    totale+=msg[i];
                    printf("unpack_msg(): Valid addr_from found: %d\r\n", msg[i]);
                } else {
                    printf("unpack_msg(): message was sent from not allowed station: %d\r\n", msg[i]);
                    result = 2;
                    return result;
                }  
            }
        } //if found_startCHR
        if ((!found_startCHR) && (msg[i]==0xf7)){
            found_startCHR=1;
        }
    } //for (int i)
    //printf("unpack_msg(): ----END UNpack str----\r\n");
    return result;
}


void foreverRed() {
    int k=0;
    for (;;) { // blink red LED forever
        gpio_set_level((gpio_num_t)BLINK_GPIO, 1);
        vTaskDelay(100 / portTICK_RATE_MS);
        gpio_set_level((gpio_num_t)BLINK_GPIO, 0);
        vTaskDelay(100 / portTICK_RATE_MS);
        k++;
        k=k % 1000;
        printf("%d: forever Red!!!!\r\n",k);
    }
}

void presentBlink(unsigned char LedPinNo){
    for (unsigned char k=0; k<NUM_PRES_BLINKS; k++){
        gpio_set_level((gpio_num_t)LedPinNo, 1);
        vTaskDelay(150 / portTICK_RATE_MS);
        gpio_set_level((gpio_num_t) LedPinNo, 0);
        vTaskDelay(150 / portTICK_RATE_MS);
    }   
}

void setupmyRadioHC12(void) {
    gpio_pad_select_gpio((gpio_num_t)HC12SETGPIO);
    /* Set the GPIO as a push/pull output  */
    gpio_set_direction((gpio_num_t)HC12SETGPIO, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)HC12SETGPIO, 1); //enter transparent mode

    uart_config_t uart_configHC12 = {
        .baud_rate = BAUDETRANSPARENTMODE,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .rx_flow_ctrl_thresh = 122,
        .use_ref_tick= false
    };
    // configure the UART2 controller 
    uart_param_config(UART_NUM_2, &uart_configHC12);
    uart_set_pin(UART_NUM_2, HC12RXGPIO, HC12TXGPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE); //TX , RX
    uart_driver_install(UART_NUM_2, 1024, 0, 0, NULL, 0);

    //VERY LIKELY USELESS SINCE HC12 HOLDS CONFIGURATION IN EEPROM or FLASH
    //CONFIGURATION LIKE:
    // - BAUD 2400
    // - CHANNEL 0X01
    // - FU FU3
    // - POWER +20dBm
    // ARE ALREADY SET UP INTO HC12 MODULE 
    return;
}

bool writeHC12(uart_port_t uart_controller, int level) {
    unsigned char *line_read;
    bool success;
    char pino[250],expectedACKpino[255];
    int num_max_attempts=5;

    for (int k=0; k<50; k++){
        pino[k]=0;
    }

    sprintf(pino,"Ciao: %d!\n",level);
    sprintf(expectedACKpino,"ACKCiao: %d!!",level);

    for (int k=25; k>=0; k--){
        line_read =read_line(uart_controller);
        printf("******flushed line: %s",line_read);
    }

    success=false;
    for (int k=num_max_attempts;k>=0; k--){
        scriviUART(uart_controller, (unsigned char*)pino);
        printf("writeHC12 printed %s on %d UART\n", pino, uart_controller);
        vTaskDelay(500 / portTICK_RATE_MS);
        line_read =read_line(uart_controller);
        printf("lineread: %s\n",line_read);
        printf("epectedACK: %s\n",expectedACKpino);
        
        if(strncmp(expectedACKpino,(const char *)line_read,strlen(expectedACKpino))==0){
            printf("Strings are equal. What a success!!!!!\n");
            success=true;
            k=-1;
            ESP_LOGD(tag, "Transmitted data");
        } else {
            ESP_LOGD(tag, "Transmitted NOT data");
            printf("String are NOT equal. What a nightmare!!!!!\n");
        }
    }
    return(success);
}

void setup(void){
    ESP_LOGW(TAG,"==============================");
    ESP_LOGW(TAG,"Welcome to HC12 control App");
    ESP_LOGW(TAG,"Setting up...................");

    /* CONFIGURING PRESENTATION BLINK LED */
    gpio_pad_select_gpio((gpio_num_t)BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction((gpio_num_t)BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)BLINK_GPIO, 1); //switch the led OFF
    ESP_LOGW(TAG,"Set up presentation led ended\r\n");

    /* CONFIGURING COMMAND INPUT */
    gpio_config_t io_conf;
    //bit mask of the pins, use GPIO4 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //diisable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    ESP_LOGW(TAG,"Set up command input ended\r\n");

    //Initializing Radio HC12
    setupmyRadioHC12();
    ESP_LOGW(TAG,"Set up HC12 Radio ended\r\n");

    //presentation blinking
    presentBlink(BLINK_GPIO);
    ESP_LOGW(TAG,"Set up things ended\r\n");
    ESP_LOGW(TAG,"==============================\r\n");

    return;
}

enum typeOfevent {NOTHING=0, IO_INPUT_ACTIVE=1, RECEIVED_MSG = 2};

typedef struct Valore_Evento_t
{
    unsigned char status;
    unsigned char* cmd_received;
    unsigned char* param_received;
    unsigned char ack_rep_counts;
} valore_evento_t; 

typedef struct Evento
{
    enum typeOfevent id_evento;
    valore_evento_t valore_evento;
} evento;

evento* detectEvent(unsigned char uart_controller){
    static evento detected_event;
    static unsigned char IN_PREC;
    unsigned char IN=(unsigned char) gpio_get_level((gpio_num_t )GPIO_INPUT_COMMAND_PIN);
    printf("detectEvent(): value read: %d!\r\n",IN);
    ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)BLINK_GPIO, (uint32_t) IN));
    //Reset event variable
    detected_event.id_evento = NOTHING;
    detected_event.valore_evento.status = 0;
    detected_event.valore_evento.cmd_received=0;
    detected_event.valore_evento.param_received=0;
    detected_event.valore_evento.ack_rep_counts=0;

    //Detect if IO_INPUT went low 
    printf("detectEvent(): IN: %u ; IN_PREC: %u\r\n",IN,IN_PREC);
    if (!(IN ==IN_PREC)){
        detected_event.id_evento = IO_INPUT_ACTIVE;
        printf("detectEvent(): !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!Event Occurred!!!!!!!\r\n");
        detected_event.valore_evento.status = IN;
        detected_event.valore_evento.cmd_received=0;
        detected_event.valore_evento.param_received=0;
        detected_event.valore_evento.ack_rep_counts=0;
        IN_PREC=IN;
        return &detected_event;
    }
    IN_PREC=IN;

    //Detect if a message has been received from any station 
    // Read data from the UART2 (the only used UART)
    unsigned char *line1;
    line1 = read_line(UART_NUM_2);
    //char *subline1=parse_line(line1,&parametri_globali,allowedCallers);
    printf("detect_event(): line read from UART: ");
    stampaStringa((char *) line1);
    printf("\r\ndetect_event(): Parsing line read!\r\n ");
    unsigned char allowed_addr_from;
    unsigned char allowed_addr_to;

    printf("\r\ndetect_event(): Parsing line read!\r\n ");
    if (STATION_ROLE==STATIONMASTER){
        allowed_addr_from=ADDR_MASTER_STATION;
        allowed_addr_to=ADDR_SLAVE_STATION; //in realta nell'utilizzo reale saranno da swappare ma per ora in fase di sviluppo codice mi viene comodo così (infatti saranno generati da una pack_msg che gira sulla slave station)
    }
    if (STATION_ROLE==STATIONSLAVE){
        allowed_addr_from=ADDR_MASTER_STATION;
        allowed_addr_to=ADDR_SLAVE_STATION; 
    }
    if (STATION_ROLE==STATIONMOBILE){
        allowed_addr_from=ADDR_MASTER_STATION;
        allowed_addr_to=ADDR_MOBILE_STATION; 
    }

    unsigned char* cmd_received;
    unsigned char* param_received;
    unsigned char ack_rep_counts=0;
    
    unsigned char ret=unpack_msg(line1, allowed_addr_from, allowed_addr_to, &cmd_received, &param_received, &ack_rep_counts);
    printf("detect_event(): unpack returned with code: %d!\r\n",ret);
    printf("detect_event(): cmd_received: %s\r\n",cmd_received);
    printf("detect_event(): param_received: %s\r\n",param_received);
    printf("detect_event(): ack_rep_counts: %d\r\n",ack_rep_counts);

    if (ret==0) {
        ok++;
        detected_event.id_evento = RECEIVED_MSG;
        printf("detectEvent(): !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!A message of interest has been detected!!!!!!!\r\n");
        detected_event.valore_evento.status = 0;
        detected_event.valore_evento.cmd_received=cmd_received;
        detected_event.valore_evento.param_received=param_received;
        detected_event.valore_evento.ack_rep_counts=ack_rep_counts;
        return &detected_event;
    } else
    {
        printf("message was not of interest. RetCode=%u\r\n",ret);
        ko++;
    }
    printf("detect_event(): ok: % d -- ko: %u\r\n",ok,ko);
    return &detected_event;
}

unsigned char  waiting_for_ack=0;

void loop(void){
    evento* detected_event;

    unsigned char cmd_tosend[FIELD_MAX]; //="abdcefgh1234567891";
    unsigned char param_tosend[FIELD_MAX]; //="ijklmnop0";
    unsigned char* msg;
    memset(cmd_tosend,0,FIELD_MAX);
    memset(param_tosend,0,FIELD_MAX);
    unsigned char  addr_from;
    unsigned char  addr_to;

    printf("loop(): Ciao!!!!! ");

    //Listening for any event to occour
    detected_event=detectEvent(UART_NUM_2);

    if (STATION_ROLE==STATIONMASTER){ //BEHAVE AS MASTER STATION
        printf("This is the Master station\n");

        if (detected_event->id_evento == IO_INPUT_ACTIVE) { // CHECKING IF MY INPUT PIN TOLD ME TO SEND A CMD TO SLAVE STATION
            if (detected_event->valore_evento.status==1){ // NOTHING TO DO: 
                printf("\r\n\r\n\r\n\r\n\r\n\r\nEVENTO SU I_O PIN GESTITO CORRETTAMENTE!!!!!\r\n");
                return;
            } else { //SENDING COMMAND TO OPEN
                addr_from=ADDR_MASTER_STATION; //ADDR_MOBILE_STATION+5;
                addr_to=ADDR_SLAVE_STATION; //ADDR_MOBILE_STATION;
                strcpy((char *)cmd_tosend,"APRI");
                strcpy((char *)param_tosend,"0");
                printf("loop():cmd_tosend: %s\r\n",cmd_tosend);
                printf("loop():param_tosend: %s\r\n",param_tosend);
                unsigned char rep_counts=++pp;
                msg=pack_msg(addr_from, addr_to, cmd_tosend, param_tosend, rep_counts);
                //printf("msg: %s , msg_len: %u\r\n", msg, strlen2(msg));
                //printf("msg: %s , msg_len: %u\r\n", msg, strlen2(msg));
                //printf("loop(): ***************end:%d\r\n",msg[strlen2(msg)-1]);
                msg[strlen2(msg)]='E';
                msg[strlen2(msg)]='N';
                msg[strlen2(msg)]='D';
                //sending msg on UART2
                printf("msg: %s , msg_len: %u\r\n", msg, strlen2(msg));
                int numbytes=scriviUART(UART_NUM_2, msg);
                if (numbytes<0){
                    printf("loop(): Fatal error in sending data on UART\r\n");
                    vTaskDelay(5000 / portTICK_RATE_MS);
                    return;
                } else {
                    printf("loop(): sent %d bytes on UART_NUM_2\r\n",numbytes);
                    vTaskDelay(500 / portTICK_RATE_MS);
                    presentBlink(BLINK_GPIO);
                }
                waiting_for_ack=1;
            }
        } else if (detected_event->id_evento == RECEIVED_MSG) { 
            printf("loop(): This was the msg for me:\r\n");
            printf("loop():cmd_received: %s\r\n",detected_event->valore_evento.cmd_received);
            printf("loop():param_received: %s\r\n",detected_event->valore_evento.param_received);
            printf("loop():ack_rep_counts: %d\r\n",detected_event->valore_evento.ack_rep_counts);

            if (waiting_for_ack==1){
                printf("loop(): I was waiting for an ACK so I should do something\r\n");
                /* code */
            } else
            {
                printf("loop(): I received a valid message from mobile station that I should forward to slave station\r\n");
                /* code */
            } 
        } else {
            vTaskDelay(500/portTICK_RATE_MS);
            return;
        }
    } //if (STATION_ROLE==STATIONMASTER)

    if (STATION_ROLE==STATIONSLAVE){ //BEHAVE AS SLAVE STATION
        printf("This is the Slave station\n");
        if (detected_event->id_evento == RECEIVED_MSG) { 
            printf("loop(): This was the msg for me:\r\n");
            printf("loop():cmd_received: %s\r\n",detected_event->valore_evento.cmd_received);
            printf("loop():param_received: %s\r\n",detected_event->valore_evento.param_received);
            printf("loop():ack_rep_counts: %d\r\n",detected_event->valore_evento.ack_rep_counts);

            if (waiting_for_ack==1){
                printf("loop(): I was waiting for an ACK (?!?!?!?) so I should do something\r\n");
                /* code */
            } else
            {
                printf("loop(): I received a valid COMMAND (open, close, sendreport) from MASTER  OR MOBILE station that I should satisfy\r\n");
                /* code */
            } 
        } else {
            vTaskDelay(500/portTICK_RATE_MS);
            return;
        }
    } //if (STATION_ROLE==STATIONSLAVE)

    if (STATION_ROLE==STATIONMOBILE){ //BEHAVE AS MOBILE STATION
        printf("This is the Mobile station\n");
        if (detected_event->id_evento == IO_INPUT_ACTIVE) { // CHECKING IF MY INPUT PIN TOLD ME TO SEND A CMD TO SLAVE STATION
            if (detected_event->valore_evento.status==1){ // NOTHING TO DO: 
                printf("\r\n\r\n\r\n\r\n\r\n\r\nEVENTO SU I_O PIN GESTITO CORRETTAMENTE!!!!!\r\n");
                return;
            } else { //SENDING COMMAND TO OPEN
                addr_from=ADDR_MASTER_STATION; //ADDR_MOBILE_STATION+5;
                addr_to=ADDR_SLAVE_STATION; //ADDR_MOBILE_STATION;
                strcpy((char *)cmd_tosend,"APRI");
                strcpy((char *)param_tosend,"0");
                printf("loop():cmd_tosend: %s\r\n",cmd_tosend);
                printf("loop():param_tosend: %s\r\n",param_tosend);
                //def nuova func: send_msg(UART_NUM_2, addr_from, addr_to,cmd_tosend, param_tosend, rep_counts);
                //uso nuova func: send_msg(UART_NUM_2, ADDR_MASTER_STATION, ADDR_SLAVE_STATION,"APRI", "0", rep_counts);

                unsigned char rep_counts=++pp;
                msg=pack_msg(addr_from, addr_to, cmd_tosend, param_tosend, rep_counts);
                //printf("msg: %s , msg_len: %u\r\n", msg, strlen2(msg));
                //printf("msg: %s , msg_len: %u\r\n", msg, strlen2(msg));
                //printf("loop(): ***************end:%d\r\n",msg[strlen2(msg)-1]);
                msg[strlen2(msg)]='E';
                msg[strlen2(msg)]='N';
                msg[strlen2(msg)]='D';
                //sending msg on UART2
                printf("msg: %s , msg_len: %u\r\n", msg, strlen2(msg));
                int numbytes=scriviUART(UART_NUM_2, msg);
                if (numbytes<0){
                    printf("loop(): Fatal error in sending data on UART\r\n");
                    vTaskDelay(5000 / portTICK_RATE_MS);
                    return;
                } else {
                    printf("loop(): sent %d bytes on UART_NUM_2\r\n",numbytes);
                    vTaskDelay(500 / portTICK_RATE_MS);
                    presentBlink(BLINK_GPIO);
                }
                waiting_for_ack=1;
            }
        } else if (detected_event->id_evento == RECEIVED_MSG) { 
            printf("loop(): This was the msg for me:\r\n");
            printf("loop():cmd_received: %s\r\n",detected_event->valore_evento.cmd_received);
            printf("loop():param_received: %s\r\n",detected_event->valore_evento.param_received);
            printf("loop():ack_rep_counts: %d\r\n",detected_event->valore_evento.ack_rep_counts);

            if (waiting_for_ack==1){
                printf("loop(): I was waiting for an ACK so I should do something\r\n");
                /* code */
            } else
            {
                printf("loop(): I received a valid message from ?!?!?! station that I should forward to ?!?!?!\r\n");
                /* code */
            } 
        } else {
            vTaskDelay(500/portTICK_RATE_MS);
            return;
        }
    }  //if (STATION_ROLE==STATIONMOBILE)

    return;
    /*unsigned char* tmp=cmd_tosend;
    for (int r=0; r<FIELD_MAX-10 ; r++){
        uint32_t randomNumber = READ_PERI_REG(DR_REG_RNG_BASE);
        unsigned char rndShort=49+((randomNumber>>26));
        *tmp=rndShort;
        tmp++;
    }
    tmp=param_tosend;
    for (int r=0; r<FIELD_MAX-10 ; r++){
        uint32_t randomNumber = READ_PERI_REG(DR_REG_RNG_BASE);
        unsigned char rndShort=49+((randomNumber>>26));
        *tmp=rndShort;
        tmp++;
    }*/
    //strcpy((char *)cmd_tosend,"abdcefgh1234567891");
    //pp=(pp+1)%9;
    //cmd_tosend[strlen2(cmd_tosend)]=pp+48;
    //strcpy((char *)param_tosend,"ijklmnop0");
    printf("loop():cmd_tosend: %s\r\n",cmd_tosend);
    printf("loop():param_tosend: %s\r\n",param_tosend);
    unsigned char rep_counts=++pp;
    msg=pack_msg(addr_from, addr_to, cmd_tosend, param_tosend, rep_counts);
    //printf("msg: %s , msg_len: %u\r\n", msg, strlen2(msg));
    //printf("msg: %s , msg_len: %u\r\n", msg, strlen2(msg));
    //printf("loop(): ***************end:%d\r\n",msg[strlen2(msg)-1]);
    msg[strlen2(msg)]='E';
    msg[strlen2(msg)]='N';
    msg[strlen2(msg)]='D';
    //msg[strlen2(msg)-7]=0;
    //for (int i=0; i<strlen2(msg);i++){
    //    printf("loop(): packed_msg[%d]= %d - %x - %c\r\n", i, msg[i], msg[i], msg[i]);
    //}

    //sending msg on UART2
    printf("msg: %s , msg_len: %u\r\n", msg, strlen2(msg));
    int numbytes=scriviUART(UART_NUM_2, msg);
    if (numbytes<0){
        printf("loop(): Fatal error in sending data on UART\r\n");
        vTaskDelay(5000 / portTICK_RATE_MS);
        return;
    } else {
        printf("loop(): sent %d bytes on UART_NUM_2\r\n",numbytes);
        vTaskDelay(500 / portTICK_RATE_MS);
        presentBlink(BLINK_GPIO);
    }

    // Read data from the UART2 (the only used UART) -- TO DETECT ACK
    unsigned char *line1;
    line1 = read_line(UART_NUM_2);
    //char *subline1=parse_line(line1,&parametri_globali,allowedCallers);
    printf("line read from UART: ");
    stampaStringa((char *) line1);
    
    unsigned char allowed_addr_from=ADDR_MASTER_STATION;
    unsigned char allowed_addr_to=ADDR_SLAVE_STATION; //in realta nell'utilizzo reale saranno da swappare ma per ora in fase di sviluppo codice mi viene comodo così (infatti saranno generati da una pack_msg che gira sulla slave station)
    unsigned char* cmd_received;
    unsigned char* param_received;
    unsigned char ack_rep_counts=0;
    
    unsigned char ret=unpack_msg(line1, allowed_addr_from, allowed_addr_to, &cmd_received, &param_received, &ack_rep_counts);
    printf("loop():unpack returned with code: %d!\r\n",ret);
    printf("loop():cmd_received: %s\r\n",cmd_received);
    printf("loop():param_received: %s\r\n",param_received);
    printf("loop():ack_rep_counts: %d\r\n",ack_rep_counts);

    if (ret==0) {
        ok++;
    } else
    {
        ko++;
    }
    printf("loop(): ok: % d -- ko: %u\r\n",ok,ko);

    vTaskDelay(1 / portTICK_RATE_MS);
    return;

    /*if (command_valid){
        ESP_LOGW(TAG,"\n********Command valid, sending %s",command);
        //uart_write_bytes(UART_NUM_1, command, strlen(command));
        scriviUART(UART_NUM_1, command);
        //verificaComando(UART_NUM_1,command,"OK\r\n");
        command_valid=0;
    }
    vTaskDelay(100 / portTICK_RATE_MS);

    if (parametri_globali.calling_number_valid==ALLOWED_CALLER){
        ESP_LOGI(TAG,"\n********Valid calling number: %s. Answering call.....",parametri_globali.generic_parametro_feedback); //da eliminare
        ESP_LOGI(TAG,"\n********Valid calling number: %s. Answering call.....",parametri_globali.calling_number);
    
        char msg1[4]="ATA";
        char msg_tmp[19]="AT+DDET=1,1000,0,0";
        char condition1[5]="OK\r\n";

        //RESETTING GLOBAL PARAMETERS RELEVANT TO CALLERS (ALLOWED)
        parametri_globali.calling_number_valid=NO_CALLER;
        //parametri_globali.calling_number[0]=0;
        memset(parametri_globali.calling_number, 0, NUMCELL_MAX*sizeof(parametri_globali.calling_number[0])); 

        ESP_LOGW(TAG,"\n********Enabling DTMF recognition....");
        if (verificaComando(UART_NUM_1,msg_tmp,condition1,&parametri_globali,allowedCallers)==0){
            ESP_LOGI(TAG,"\n********DTMF recognition is on !!!");
        } else {
            ESP_LOGE(TAG,"\n********Error in setting mode DTMF recognition on!!!");
        }
        
        if (verificaComando(UART_NUM_1,msg1,condition1,&parametri_globali,allowedCallers)==0){
            ESP_LOGI(TAG,"\n********Answered call!!!");
            char msg2[15]="AT+VTS=\"1,0,6\"";
            char condition2[5]="OK\r\n";
            //if (verificaComando(UART_NUM_1,"AT+VTS=\"{1,1},{0,2},{6,1}\"","OK\r\n",&parametri_globali)==0){
            if (verificaComando(UART_NUM_1,msg2,condition2,&parametri_globali,allowedCallers)==0){
                ESP_LOGI(TAG,"\n********Answer tones sent!!!");
                vTaskDelay(500 / portTICK_RATE_MS);
            } else {
                ESP_LOGE(TAG,"\n********ERROR in sending answering tones!!!");
            }
        } else {
            ESP_LOGE(TAG,"\n********Error in answering allowed call!!!");
        }
    } else if (parametri_globali.calling_number_valid==NOT_ALLOWED_CALLER) {
        ESP_LOGE(TAG,"\n********Calling number %s NOT valid!!! Answering and hanging right after.",parametri_globali.generic_parametro_feedback); //DA ELIMINARE
        ESP_LOGE(TAG,"\n********Calling number %s NOT valid!!! Answering and hanging right after.",parametri_globali.calling_number);
        
        char msg3[4]="ATA";
        char condition3[5]="OK\r\n";
        
        //RESETTING GLOBAL PARAMETERS RELEVANT TO CALLERS (NOT_ALLOWED)
        parametri_globali.calling_number_valid=NO_CALLER;
        //parametri_globali.calling_number[0]=0;
        memset(parametri_globali.calling_number, 0, NUMCELL_MAX*sizeof(parametri_globali.calling_number[0])); 

        if (verificaComando(UART_NUM_1,msg3,condition3,&parametri_globali,allowedCallers)==0){
                ESP_LOGI(TAG,"\n********Answered NOT allowed call!!!");
                vTaskDelay(2000 / portTICK_RATE_MS);
                char msg4[4]="ATH";
                char condition4[5]="OK\r\n";                       
                if (verificaComando(UART_NUM_1,msg4,condition4,&parametri_globali,allowedCallers)==0){
                    ESP_LOGI(TAG,"\n********Hanged call!!!");
                    vTaskDelay(1000 / portTICK_RATE_MS);
                } else {
                    ESP_LOGE(TAG,"\n********ERROR in hanging NOT allowed call!!!");
                }
            } else {
                ESP_LOGE(TAG,"\n********Error in answering NOT allowed call!!!");
            }
    }

    //int parametri_globali.pending_DTMF = checkDTMF();
    if (parametri_globali.pending_DTMF>=0){
        int DTMF=parametri_globali.pending_DTMF;
        ESP_LOGW(TAG,"Tone detected equals %d. Replying with the same tone.",DTMF);;

        char replyTone[30];
        sprintf(replyTone,"AT+VTS=\"{%d,2}\"",DTMF);
        vTaskDelay(2000 / portTICK_RATE_MS);
        char condition5[5]="OK\r\n";

        //RESETTING GLOBAL PARAMETERS RELEVANT TO PENDING DTMF
        parametri_globali.pending_DTMF=NO_PENDING_DTMF_TONE;

        if (verificaComando(UART_NUM_1,replyTone,condition5,&parametri_globali,allowedCallers)==0){
                    ESP_LOGI(TAG,"\nReply tone sent!!!");

                    if(writeHC12(UART_NUM_2,DTMF)==true){ //sending command to remote station
                        ESP_LOGI(TAG,"\nMessage sent to SlaveStation was succesffully delivered!!!");   
                        sprintf(replyTone,"AT+VTS=\"{%d,1}\"",DTMF);
                        verificaComando(UART_NUM_1,replyTone,condition5,&parametri_globali,allowedCallers);
                        verificaComando(UART_NUM_1,replyTone,condition5,&parametri_globali,allowedCallers);
                        verificaComando(UART_NUM_1,replyTone,condition5,&parametri_globali,allowedCallers); 
                    } else {
                        ESP_LOGE(TAG,"\nMessage sent to SlaveStation was NOT delivered!!!");    
                        //verificaComando(UART_NUM_1,replyTone,condition5,&parametri_globali);
                    }
                } else {
                    ESP_LOGE(TAG,"\nERROR in sending reply tone!!!");
                }
    } else 
    {
        char output[50];
        sprintf(output,"nop : %d",kkk);
        ESP_LOGI(TAG,"nop");
        printf(output);
    }

    kkk++;
    if (kkk>=999) {
        ledus=1-ledus;
        gpio_set_level((gpio_num_t)BLINK_GPIO, ledus);
        kkk=0;
        int retCode = checkPhoneActivityStatus(&parametri_globali,allowedCallers);
        if (parametri_globali.phoneActivityStatus == PHONE_READY) {
            ESP_LOGI(TAG,"No call in progress");
            //digitalWrite(red, LOW); // red LED off
        } else if ((retCode == -1) || (parametri_globali.phoneActivityStatus == PHONE_STATUS_UNKNOWN)){
            ESP_LOGE(TAG,"Not possible to determine Phone Activity Status!!!");
            // QUI PUOI METTERE UN TENTATUVI DI RESET AGENDO SU D9 e CON UN reset_module (che forse la farai già che agisce su D9)
            if (simOK(UART_NUM_1,&parametri_globali,allowedCallers)==-1){
                ESP_LOGE(TAG,"\nSim808 module not found, stop here!");
                foreverRed();
            } 
        } else if (parametri_globali.phoneActivityStatus == RINGING){ 
            printf("RINGING\n");
            ESP_LOGW(TAG,"RINGING!!!!");
            //RINGING
            // / *if (checkCallingNumber(parametri_globali)==0){
                ESP_LOGI(TAG,"\n********Valid calling number: %s. Answering call.....",parametri_globali.parametro_feedback);;
                char msg1[4]="ATA";
                char condition1[5]="OK\r\n";
                parametri_globali.calling_number_valid=-1;
                parametri_globali.calling_number[0]=0;
                if (verificaComando(UART_NUM_1,msg1,condition1,&parametri_globali)==0){
                        ESP_LOGI(TAG,"\n********Answered call!!!");
                        char msg2[15]="AT+VTS=\"1,0,6\"";
                        char condition2[5]="OK\r\n";
                        //if (verificaComando(UART_NUM_1,"AT+VTS=\"{1,1},{0,2},{6,1}\"","OK\r\n",&parametri_globali)==0){
                        if (verificaComando(UART_NUM_1,msg2,condition2,&parametri_globali)==0){
                            ESP_LOGI(TAG,"\n********Answer tones sent!!!");
                            vTaskDelay(500 / portTICK_RATE_MS);
                        } else {
                            ESP_LOGE(TAG,"\n********ERROR in sending answering tones!!!");
                        }
                    } else {
                        ESP_LOGE(TAG,"\n********Error in answering allowed call!!!");
                    }
            } else {
                parametri_globali.calling_number_valid=-1;
                parametri_globali.calling_number[0]=0;
                ESP_LOGE(TAG,"\n********Calling number %s NOT valid!!! Answering and hanging right after.",parametri_globali.parametro_feedback);
                char msg3[4]="ATA";
                char condition3[5]="OK\r\n";
                if (verificaComando(UART_NUM_1,msg3,condition3,&parametri_globali)==0){
                        ESP_LOGI(TAG,"\n********Answered NOT allowed call!!!");
                        vTaskDelay(2000 / portTICK_RATE_MS);
                        char msg4[4]="ATH";
                        char condition4[5]="OK\r\n";                       
                        if (verificaComando(UART_NUM_1,msg4,condition4,&parametri_globali)==0){
                            ESP_LOGI(TAG,"\n********Hanged call!!!");
                            vTaskDelay(1000 / portTICK_RATE_MS);
                        } else {
                            ESP_LOGE(TAG,"\n********ERROR in hanging NOT allowed call!!!");
                        }
                    } else {
                        ESP_LOGE(TAG,"\n********Error in answering NOT allowed call!!!");
                    }
            }
        } else if (current_phone_status == 4) { // in call 
                ESP_LOGW(TAG,"Handling Call....");
                //digitalWrite(red, HIGH); // red LED on
                if (parametri_globali.pending_DTMF>=0){
                    int DTMF=parametri_globali.pending_DTMF;
                    parametri_globali.pending_DTMF=-1;
                    ESP_LOGW(TAG,"Tone detected equals %d. Replying with the same tone.",DTMF);;
                    char replyTone[30];
                    sprintf(replyTone,"AT+VTS=\"{%d,2}\"",DTMF);
                    vTaskDelay(2000 / portTICK_RATE_MS);
                    char condition5[5]="OK\r\n";
                    if (verificaComando(UART_NUM_1,replyTone,condition5,&parametri_globali)==0){
                                ESP_LOGI(TAG,"\nReply tone sent!!!");
                                if(write_nrf24(myRadio,DTMF)==true){
                                    ESP_LOGI(TAG,"\nMessage sent to SlaveStation was succesffully delivered!!!");   
                                    sprintf(replyTone,"AT+VTS=\"{%d,1}\"",DTMF);
                                    verificaComando(UART_NUM_1,replyTone,condition5,&parametri_globali);
                                    verificaComando(UART_NUM_1,replyTone,condition5,&parametri_globali);
                                    verificaComando(UART_NUM_1,replyTone,condition5,&parametri_globali); 
                                } else {
                                    ESP_LOGE(TAG,"\nMessage sent to SlaveStation was NOT delivered!!!");    
                                    //verificaComando(UART_NUM_1,replyTone,condition5,&parametri_globali);
                                }
                            } else {
                                ESP_LOGE(TAG,"\nERROR in sending reply tone!!!");
                            }
                } else 
                {
                    printf("\nnop\n");
                }
        } else if (parametri_globali.phoneActivityStatus == CALL_IN_PROGRESS) {
            printf("CALL IN PROGRESS\n");
            ESP_LOGW(TAG,"CALL IN PROGRESS!!!!");
        }
        parametri_globali.phoneActivityStatus=2;
    }
    return;*/
}

// Main application
void app_main() {
    setup();
    ESP_LOGE(TAG,"Entering while loop!!!!!\r\n");
    while(1)
        loop();
    fflush(stdout);
    return;
}