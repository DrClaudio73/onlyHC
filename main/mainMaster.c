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

size_t strlen2(const unsigned char* stringa){
    return(strlen((const char*) stringa));
}
unsigned char calcola_CRC(int totale) {
    return((unsigned char) totale%256);
}

void pack_str(unsigned char* msg_to_send, const unsigned char* valore, unsigned char* k, int* totale){
    printf("----START pack str----\r\n");
    for (int i=0; i<strlen2(valore); i++){
        msg_to_send[*k]=valore[i];
        *totale+=msg_to_send[*k];
        printf("val: %c, val: %d , tot=%d, k=%d;\r\n",msg_to_send[*k], msg_to_send[*k], *totale, *k);
        *k=*k+1;
    }
    printf("----END pack str----\r\n");
    return;
}

void pack_num(unsigned char* msg_to_send, unsigned char valore, unsigned char* k, int* totale){
        printf("----START pack num----\r\n");
        msg_to_send[*k]=valore;
        *totale+=msg_to_send[*k];
        printf("val: %c, val: %d , tot=%d, k=%d;\r\n",msg_to_send[*k], msg_to_send[*k], *totale, *k);
        *k=*k+1;
        printf("----END pack num----\r\n");
    return;
}

unsigned char* pack_msg(unsigned char uart_controller, unsigned char addr_from, unsigned char addr_to, const unsigned char* cmd, const unsigned char* param) {
    static unsigned char msg_to_send[255];

    unsigned char expectedACK[255];
    unsigned char num_max_attempts=5;
    unsigned char *line_read;
    short success;

    memset(msg_to_send,0,255);

    //Frame structure
    //CMD: 'B'+'G'+F7+ADDR_FROM(1 byte)+ADDR_TO(1 byte)+CMD_LEN(1)+PARAM_LEN(1)+CMD(var byte)+PARAM(var byte)+CRC(1 byte)+7F
    //ANSWER: 'B'+'G'+F7+ADDR_FROM(1 byte)+ADDR_TO(1 byte)+CMD_LEN(1)+PARAM_LEN(1)+CMD(var byte)+PARAM(var byte)+CRC(1 byte)+7F
    //CRC is evaluated starting at ADDDR_FROM up to PARAM

    //build message
    int totale = 0;
    unsigned char k=0;
    msg_to_send[k++]='B'; //for synch purpose
    msg_to_send[k++]='G'; //for synch purpose
    msg_to_send[k++]=(unsigned char) 0xf7; //start bye
    pack_num(msg_to_send,addr_from,&k,&totale); 
    pack_num(msg_to_send,addr_to,&k,&totale);
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


unsigned char unpack_msg(const unsigned char* msg, unsigned char allowed_addr_from, unsigned char allowed_addr_to, unsigned char** cmd, unsigned char** param){
    //CMD: 'B'+'G'+F7+ADDR_FROM(1 byte)+ADDR_TO(1 byte)+CMD_LEN(1)+PARAM_LEN(1)+CMD(var byte)+PARAM(var byte)+CRC(1 byte)+7F
    //ANSWER: 'B'+'G'+F7+ADDR_FROM(1 byte)+ADDR_TO(1 byte)+CMD_LEN(1)+PARAM_LEN(1)+CMD(var byte)+PARAM(var byte)+CRC(1 byte)+7F
    //CRC is evaluated starting at ADDDR_FROM up to PARAM
    // RETCODES
    // 0 = OK
    // 1 = INVALID CRC
    // 2 = MESSAGE RECEIVED FROM NOT allowed_addr_from
    // 3 = MESSAGE ADDRESSED to OTHER STATION
    // 4 = MESSAGE NOT PROPERLY TERMINATED
    // 5 = MESSAGE SHORTER THAN EXPECTED

    printf("----START UNpack str----\r\n");
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
    unsigned char extracted_cmd_len=0;
    unsigned char extracted_param_len=0;
    unsigned char cmd_len=0;
    unsigned char param_len=0;
    unsigned char k=0;
    unsigned char j=0;
    unsigned char CRC_extracted=0;
    int totale=0;

    for (unsigned char i=0; i<strlen2(msg);i++) {
        printf("msgrcv[%d]= %d - %c\r\n", i, msg[i], msg[i]);

        if (found_startCHR){
            if (validated_addr_from && validated_addr_to && extracted_cmd_len && extracted_param_len && (k==cmd_len) && (j==param_len) && CRC_extracted){
                if (msg[i]==0x7f){
                    result=0;
                    return result;
                } else {
                    result=4;
                    return result;
                }
            }
            
            if (validated_addr_from && validated_addr_to && extracted_cmd_len && extracted_param_len && (k==cmd_len) && (j==param_len)){
                CRC_extracted=1;
                received_CRC=msg[i];
                if (!(received_CRC==calcola_CRC(totale))){
                    result=1;
                    return result;
                } else {
                    printf("CRC has been CORRECTLY evaluated (%d)! \r\n",received_CRC);
                }
            }

            if (validated_addr_from && validated_addr_to && extracted_cmd_len && extracted_param_len){
                if ((k==cmd_len)&&(j<param_len)){
                    param_rcved[j++]=msg[i];
                    totale+=msg[i];
                }
                if (k<cmd_len){
                    cmd_rcved[k++]=msg[i];
                    totale+=msg[i];
                }
            }

            if (validated_addr_from && validated_addr_to && extracted_cmd_len && !extracted_param_len){
                param_len=msg[i];
                totale+=msg[i];
                extracted_param_len=1;
                printf("param_len=%d\r\n",param_len);
            }

            if (validated_addr_from && validated_addr_to && !extracted_cmd_len){
                cmd_len=msg[i];
                totale+=msg[i];
                extracted_cmd_len=1;
                printf("cmd_len=%d\r\n",cmd_len);
            }

            if((validated_addr_from) && (!validated_addr_to)){
                if (msg[i]==allowed_addr_to){
                    validated_addr_to=1;
                    totale+=msg[i];
                    printf("Valid addr_to found: %d\r\n", msg[i]);
                } else {
                    printf("message was addressed to other station: %d\r\n", msg[i]);
                    result = 3;
                    return result;            
                }
            }

            if(!validated_addr_from){
                if (msg[i]==allowed_addr_from){
                    validated_addr_from=1;
                    totale+=msg[i];
                    printf("Valid addr_from found: %d\r\n", msg[i]);
                } else {
                    printf("message was sent from not allowed station: %d\r\n", msg[i]);
                    result = 2;
                    return result;
                }  
            }
        } //if found_startCHR
        if ((!found_startCHR) && (msg[i]==0xf7)){
            found_startCHR=1;
        }
    } //for (int i)
    printf("----END UNpack str----\r\n");
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
        vTaskDelay(500 / portTICK_RATE_MS);
        gpio_set_level((gpio_num_t) LedPinNo, 0);
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
        scriviUART(uart_controller, pino);
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

void loop(void){
    unsigned char IN= gpio_get_level((gpio_num_t )GPIO_INPUT_COMMAND_PIN);
    printf("value read: %d!\r\n",IN);
    ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)BLINK_GPIO, (uint32_t) IN));
    vTaskDelay(50 / portTICK_RATE_MS);
    // Read data from the UART1
    unsigned char *line1;
    line1 = read_line(UART_NUM_2);
    //char *subline1=parse_line(line1,&parametri_globali,allowedCallers);
    //char debugstr[150];
    stampaStringa((char *) line1);
    unsigned char allowed_addr_from=ADDR_MASTER_STATION;
    unsigned char allowed_addr_to=ADDR_SLAVE_STATION; //in realta nell'utilizzo reale saranno da swappare ma per ora in fase di sviluppo codice mi viene comodo così (infatti saranno generati da una pack_msg che gira sulla slave station)
    unsigned char* msg;
    unsigned char* cmd_received;
    unsigned char* param_received;
    
    unsigned char ret=unpack_msg(line1, allowed_addr_from, allowed_addr_to, &cmd_received, &param_received);
    printf("unpack returned with code: %d!\r\n",ret);
    printf("cmd_received: %s\r\n",cmd_received);
    printf("param_received: %s\r\n",param_received);

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