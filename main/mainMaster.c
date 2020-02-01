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
#include "typeconv.h"
#include "messagehandle.h"
#include "auxiliaryfuncs.h"

////////////////////////////////////////////// GLOBAL VARIABLES //////////////////////////////////////////////
command_status commands_status[NUM_MAX_CMDS];
unsigned char num_cmd_under_processing=0;

////////////////////////////////////////////// SETUP FUNCTIONS //////////////////////////////////////////////
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

void setup(void){
    ESP_LOGW(TAG,"==============================");
    ESP_LOGW(TAG,"Welcome to HC12 control App");
    ESP_LOGW(TAG,"Setting up...................");

    /* CONFIGURING PRESENTATION BLINK LED */
    gpio_pad_select_gpio((gpio_num_t)BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction((gpio_num_t)BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)BLINK_GPIO, 1); //switch the led OFF
    ESP_LOGW(TAG,"Set up presentation led ended");

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
    ESP_LOGW(TAG,"Set up command input ended");

    //Initializing Radio HC12
    setupmyRadioHC12();
    ESP_LOGW(TAG,"Set up HC12 Radio ended");

    //presentation blinking
    presentBlink(BLINK_GPIO,NUM_PRES_BLINKS);
    ESP_LOGW(TAG,"Set up things ended");
    ESP_LOGW(TAG,"==============================");

    //Initializing DB
    for (unsigned char i=0; i<NUM_MAX_CMDS ; i++){
        memset(commands_status[i].cmd,0,sizeof(commands_status[i].cmd));
        memset(commands_status[i].param,0,sizeof(commands_status[i].param));
        commands_status[i].rep_counts=0;
        commands_status[i].num_checks=0;
        commands_status[i].addr_to=0xFF;
        commands_status[i].uart_controller=0x00;
    }
    num_cmd_under_processing=0;
    
    return;
}


////////////////////////////////////////////// CORE FUNCTIONS //////////////////////////////////////////////
void list_commands_status(){
    unsigned char i=0;
    /////REPORT commands_status[] BEFORE processing
    if (num_cmd_under_processing == 0) {
        printf("clean_acknowledged_cmds():commands_status is empty\r\n");
    }
    while (i<num_cmd_under_processing){
        printf("list_commands_status(): commands_status[%u].cmd: %s\r\n",i,commands_status[i].cmd);
        printf("list_commands_status(): commands_status[%u].param: %s\r\n",i,commands_status[i].param);
        printf("list_commands_status(): commands_status[%u].rep_counts: %u\r\n",i,commands_status[i].rep_counts);
        printf("list_commands_status(): commands_status[%u].num_checks: %u\r\n",i,commands_status[i].num_checks);
        printf("list_commands_status(): commands_status[%u].addr_to: %u\r\n",i,commands_status[i].addr_to);
        printf("list_commands_status(): commands_status[%u].uart_controller: %u\r\n",i,commands_status[i].uart_controller);
        i++;
    }
    return;
}

void clean_acknowledged_cmds(){
    printf("clean_acknowledged_cmds(): BEFORE PROCESSING\r\n");
    list_commands_status();

    //clean commands_status array from empty cells
    /////processing commands_status[]
    unsigned char i=0;
    while (i<num_cmd_under_processing){
        printf("clean_acknowledged_cmds(): processing cmds no. %u\r\n",i);
        if (commands_status[i].addr_to == 0xFF){ //this command has been previsously acknowledeged: so it has to be removed
            printf("clean_acknowledged_cmds(): cmds no. %u has been previously acknowledeged.....cleaning up\r\n",i);
            unsigned char j=i;
            while (j<num_cmd_under_processing-1){
                strcpy2(commands_status[j].cmd,commands_status[j+1].cmd);
                strcpy2(commands_status[j].param,commands_status[j+1].param);
                commands_status[j].rep_counts=commands_status[j+1].rep_counts;
                commands_status[j].num_checks=commands_status[j+1].num_checks;
                commands_status[j].addr_to=commands_status[j+1].addr_to;
                commands_status[j].uart_controller=commands_status[j+1].uart_controller;
                j++;
            }
            num_cmd_under_processing--;
            printf("clean_acknowledged_cmds(): num_cmd_under_processing: %u\r\n",num_cmd_under_processing);
            memset(commands_status[num_cmd_under_processing].cmd,0,sizeof(commands_status[i].cmd));
            memset(commands_status[num_cmd_under_processing].param,0,sizeof(commands_status[i].param));
            commands_status[num_cmd_under_processing].rep_counts=0;
            commands_status[num_cmd_under_processing].num_checks=0;
            commands_status[num_cmd_under_processing].addr_to=0xFF;
            commands_status[num_cmd_under_processing].uart_controller=0;

            i=-1; //after having shifted one ACKed command then restart scanning from the beginning
        }
        i++;
    }

    /////REPORT commands_status[] AFTER processing
    printf("clean_acknowledged_cmds(): AFTER PROCESSING\r\n");
    list_commands_status();
    return;
}

unsigned char invia_comando(uart_port_t uart_controller, unsigned char addr_from, unsigned char addr_to, const unsigned char* cmd, const unsigned char* param, unsigned char rep_counts){
    unsigned char* msg;

    printf("invia_messaggio():uart_controller: %u\r\n",uart_controller);
    printf("invia_messaggio():addr_from: %u\r\n",addr_from);
    printf("invia_messaggio():addr_to: %u\r\n",addr_to);
    printf("invia_messaggio():cmd_tosend: %s\r\n",cmd);
    printf("invia_messaggio():param_tosend: %s\r\n",param);
    printf("invia_messaggio():rep_counts: %u\r\n",rep_counts);
    
    msg=pack_msg(addr_from, addr_to, cmd, param, rep_counts);
    printf("invia_messaggio(): msg: %s , msg_len: %u\r\n", msg, strlen2(msg));
    //printf("loop(): ***************end:%d\r\n",msg[strlen2(msg)-1]);
    msg[strlen2(msg)]='E';
    msg[strlen2(msg)]='N';
    msg[strlen2(msg)]='D';
    
    //sending msg on UART2
    printf("invia_messaggio(): msg: %s , msg_len: %u\r\n", msg, strlen2(msg));
    int numbytes=scriviUART(uart_controller, msg);
    
    if (numbytes<0){
        printf("invia_messaggio(): Fatal error in sending data on UART%u\r\n",uart_controller);
        vTaskDelay(5000 / portTICK_RATE_MS);
        return -1;
    } else {
        printf("invia_messaggio(): sent %d bytes on UART_NUM_%u\r\n",numbytes,uart_controller);
        vTaskDelay(500 / portTICK_RATE_MS);
        presentBlink(BLINK_GPIO,NUM_PRES_BLINKS);
    }
    
    //if this command is the first of the repetition and is not an ACK (so it is an actually new command) I am inserting this new command in commands_status to be tracked
    if ((!strncmp2(cmd,(const unsigned char*) "ACK",3)==0)&&(rep_counts==1)){
        printf("This is an actual command inserting in commands_status: ^%s^\n", cmd);
        strcpy2(commands_status[num_cmd_under_processing].cmd, cmd);
        strcpy2(commands_status[num_cmd_under_processing].param,param);
        commands_status[num_cmd_under_processing].rep_counts=rep_counts; 
        commands_status[num_cmd_under_processing].num_checks=0; 
        commands_status[num_cmd_under_processing].addr_to=addr_to;
        commands_status[num_cmd_under_processing].uart_controller=uart_controller;
        num_cmd_under_processing++;
    }

    return 0;
}

void check_rcv_acks(uart_port_t uart_controller, evento detected_event){
    printf("check_rcv_acks():BEGIN\r\n");
    if (detected_event.id_evento == RECEIVED_MSG){
        unsigned char i=0;
        printf("check_rcv_acks():detected_event->id_evento == RECEIVED_MSG\r\n");
        //check if current event is an ACK
        i=0;
        while (i<num_cmd_under_processing){
            printf("check_rcv_acks():detected_event->valore_evento.cmd_received=%s\r\n",detected_event.valore_evento.cmd_received);
            printf("check_rcv_acks():commands_status[%u].cmd=%s\r\n",i,commands_status[i].cmd);
            printf("check_rcv_acks():detected_event->valore_evento.param_received=%s\r\n",detected_event.valore_evento.param_received);
            printf("check_rcv_acks():commands_status[%u].param=%s\r\n",i,commands_status[i].param);
            printf("check_rcv_acks():detected_event->valore_evento.ack_rep_counts=%u\r\n",detected_event.valore_evento.ack_rep_counts);
            printf("check_rcv_acks():commands_status[%u].rep_counts=%u\r\n",i,commands_status[i].rep_counts);
            printf("check_rcv_acks():detected_event->valore_evento.sender=%u\r\n",detected_event.valore_evento.sender);
            printf("check_rcv_acks():commands_status[%u].addr_to=%u\r\n",i,commands_status[i].addr_to);
            printf("check_rcv_acks():uart_controller=%u\r\n",uart_controller);
            printf("check_rcv_acks():commands_status[%u].uart_controller=%u\r\n",i,commands_status[i].uart_controller);
                     
            if((strcmpACK(detected_event.valore_evento.cmd_received,commands_status[i].cmd)==0)&&
                (strcmp2(detected_event.valore_evento.param_received,commands_status[i].param)==0)&&
                (detected_event.valore_evento.ack_rep_counts==commands_status[i].rep_counts)&&
                (detected_event.valore_evento.sender==commands_status[i].addr_to)&&
                (uart_controller==commands_status[i].uart_controller)
                ){
                    printf("check_rcv_acks(): CMD NUMBER %u HAS BEEN ACKNOWLEDGED!!!!!!!!\n",i);       
                    commands_status[i].addr_to=0xFF;
                    return; //with this return only one command is acknowleded with a received ACK, without this return all the same cmd+param+rep_count+addr_to will be ackowledeged with one only ACK  
            } else {
                if (commands_status[i].num_checks < NUM_MAX_CHECKS_FOR_ACK) {
                    commands_status[i].num_checks++;
                } else {
                    commands_status[i].num_checks=0;
                    invia_comando(uart_controller, LOCAL_STATION_ADDRESS,commands_status[i].addr_to,commands_status[i].cmd,commands_status[i].param,commands_status[i].rep_counts+1);
                }
            } 
            i++;
        } //while (i<num_cmd_under_processing)
    } 
    return;
}

evento* detect_event(uart_port_t uart_controller){
    printf("detectEvent(): Calling function clean_acknowledged_cmds()\r\n");
    clean_acknowledged_cmds();

    static evento detected_event;
    static unsigned char IN_PREC;
    unsigned char IN=(unsigned char) gpio_get_level((gpio_num_t )GPIO_INPUT_COMMAND_PIN);
    //printf("detectEvent(): value read: %d!\r\n",IN);
    ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)BLINK_GPIO, (uint32_t) IN));
    //Reset event variable
    detected_event.id_evento = NOTHING;
    detected_event.valore_evento.value_of_input = 0;
    detected_event.valore_evento.cmd_received=0;
    detected_event.valore_evento.param_received=0;
    detected_event.valore_evento.ack_rep_counts=0;
    detected_event.valore_evento.sender=0;
    
    //Detect if IO_INPUT went low 
    if (!(IN==IN_PREC)){
        detected_event.id_evento = IO_INPUT_ACTIVE;
        printf("detectEvent(): IN: %u ; IN_PREC: %u\r\n",IN,IN_PREC);
        printf("detectEvent(): !!!!!!! IO Input Event Occurred !!!!!!!\r\n");
        detected_event.valore_evento.value_of_input = IN;
        detected_event.valore_evento.cmd_received=0;
        detected_event.valore_evento.param_received=0;
        detected_event.valore_evento.ack_rep_counts=0;
        detected_event.valore_evento.sender=0; //myself
        IN_PREC=IN;
        return &detected_event;
    }
    IN_PREC=IN;

    //Detect if a message has been received from any station reading data from the uart controller
    unsigned char *line1;
    line1 = read_line(uart_controller);
    printf("detect_event(): line read from UART%u: ",uart_controller);
    stampaStringa((char *) line1);

    unsigned char station_iter=0;
    unsigned char allowed_addr_to=LOCAL_STATION_ADDRESS;

    while (station_iter<2){
        printf("\r\ndetect_event(): Parsing line read -  iter: %u!\r\n ", station_iter);
        unsigned char allowed_addr_from;
        if (station_iter==0){ //check if someone sent me a message
            //First I try with one of the other 2 actors 
            if (STATION_ROLE==STATIONMASTER){
                allowed_addr_from=ADDR_SLAVE_STATION;
            }
            if (STATION_ROLE==STATIONSLAVE){
                allowed_addr_from=ADDR_MASTER_STATION;
            }
            if (STATION_ROLE==STATIONMOBILE){
                allowed_addr_from=ADDR_MASTER_STATION;
            }
        } else {
            //Then I try with the other of the other 2 actors 
            if (STATION_ROLE==STATIONMASTER){
                allowed_addr_from=ADDR_MOBILE_STATION;
            }
            if (STATION_ROLE==STATIONSLAVE){
                allowed_addr_from=ADDR_MOBILE_STATION;
            }
            if (STATION_ROLE==STATIONMOBILE){
                allowed_addr_from=ADDR_SLAVE_STATION;
            }
        }

        unsigned char* cmd_received;
        unsigned char* param_received;
        unsigned char ack_rep_counts=0;
        
        unsigned char ret=unpack_msg(line1, allowed_addr_from, allowed_addr_to, &cmd_received, &param_received, &ack_rep_counts);
        printf("\ndetect_event(): unpack returned with code: %d!\r\n",ret);
        printf("detect_event(): cmd_received: %s\r\n",cmd_received);
        printf("detect_event(): param_received: %s\r\n",param_received);
        printf("detect_event(): ack_rep_counts: %d\r\n",ack_rep_counts);

        if (ret==0){
            detected_event.id_evento = RECEIVED_MSG;
            printf("detectEvent(): !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!A message of interest has been detected!!!!!!!\r\n");
            detected_event.valore_evento.value_of_input = 0;
            detected_event.valore_evento.cmd_received=cmd_received;
            detected_event.valore_evento.param_received=param_received;
            detected_event.valore_evento.ack_rep_counts=ack_rep_counts;
            detected_event.valore_evento.sender=allowed_addr_from; //the actual sender
            station_iter=1;//return &detected_event;
            check_rcv_acks(uart_controller, detected_event);
        } else {
            printf("detectEvent(): message was not of interest. RetCode=%u\r\n",ret);
        }
        station_iter++;
    }
    return &detected_event;
}

void loop(void){
    evento* detected_event;

    printf("loop(): *******************************************Entering loop() function!!!!!\r\n");
    if(num_cmd_under_processing==0) printf("loop(): commands_status is empty!!!!\r\n");
    for(unsigned char l=0;l<num_cmd_under_processing;l++){
        printf("loop(): commands_status[%u].cmd: %s\r\n",l,commands_status[l].cmd);
        printf("loop(): commands_status[%u].prm: %s\r\n",l,commands_status[l].param);
        printf("loop(): commands_status[%u].rep_counts: %u\r\n",l,commands_status[l].rep_counts);
        printf("loop(): commands_status[%u].num_checks: %u\r\n",l,commands_status[l].num_checks);
        printf("loop(): commands_status[%u].addr_to: %u\r\n",l,commands_status[l].addr_to);
        printf("loop(): commands_status[%u].uart_controller: %u\r\n",l,commands_status[l].uart_controller);
    }

    //Listening for any event to occour
    detected_event=detect_event(UART_NUM_2);
    
    if (STATION_ROLE==STATIONMASTER){ //BEHAVE AS MASTER STATION
        //addr_from=ADDR_MASTER_STATION; //ADDR_MOBILE_STATION+5;
        printf("\nloop(): ######################This is the Master station######################\n");

        if (detected_event->id_evento == IO_INPUT_ACTIVE) { // CHECKING IF MY INPUT PIN TOLD ME TO SEND A CMD TO SLAVE STATION
            if (detected_event->valore_evento.value_of_input==1){ // NOTHING TO DO: 
                printf("\r\nloop(): EVENTO SU I_O PIN GESTITO CORRETTAMENTE!!!!!\r\n");
                return;
            } else { //SENDING COMMAND TO OPEN
                //num_retries_cmds_frominput=0;
                /*addr_to=ADDR_SLAVE_STATION; //ADDR_MOBILE_STATION;
                strcpy((char *)cmd_tosend,"APRI");
                strcpy((char *)param_tosend,"0");
                invia_comando(UART_NUM_2, addr_from, addr_to, cmd_tosend, param_tosend, 1);*/
                invia_comando(UART_NUM_2, ADDR_MASTER_STATION, ADDR_SLAVE_STATION, (const unsigned char *) "APRI", (const unsigned char *) "0", 1);
                
            }
        } else if (detected_event->id_evento == RECEIVED_MSG) { 
            ESP_LOGW(TAG,"loop(): The detected event is a msg for me! With this content:\r\n");
            printf("loop():detected_event->valore_evento.cmd_received: %s\r\n",detected_event->valore_evento.cmd_received);
            printf("loop():detected_event->valore_evento.param_received: %s\r\n",detected_event->valore_evento.param_received);
            printf("loop():detected_event->valore_evento.ack_rep_counts: %u\r\n",detected_event->valore_evento.ack_rep_counts);
            printf("loop():detected_event->valore_evento.sender: %u\r\n",detected_event->valore_evento.sender);

            //check_rcv_acks(detected_event);

            /*
            if (waiting_for_cmdinputack==1){
                printf("loop(): I was waiting for an ACK so I should do something\r\n");
                // code 
            } else
            {
                printf("loop(): I received a valid message from mobile station that I should forward to slave station\r\n");
                // code 
            } */
        } else {
            vTaskDelay(500/portTICK_RATE_MS);
            return;
        }
    } //if (STATION_ROLE==STATIONMASTER)

    if (STATION_ROLE==STATIONSLAVE){ //BEHAVE AS SLAVE STATION
        //addr_from=ADDR_SLAVE_STATION;
        printf("\nn######################This is the Slave station######################\n");
        if (detected_event->id_evento == RECEIVED_MSG) { 
            ESP_LOGW(TAG,"loop(): This was the msg for me:\r\n");
            printf("loop():cmd_received: %s\r\n",detected_event->valore_evento.cmd_received);
            printf("loop():param_received: %s\r\n",detected_event->valore_evento.param_received);
            printf("loop():ack_rep_counts: %u\r\n",detected_event->valore_evento.ack_rep_counts);
            printf("loop():sender: %u\r\n",detected_event->valore_evento.sender);
            //strcpy((char *)cmd_tosend,"ACK_APRI");
            //strcpy((char *)param_tosend,"0");
            invia_comando(UART_NUM_2, ADDR_SLAVE_STATION, detected_event->valore_evento.sender, (const unsigned char *) "ACK_APRI", detected_event->valore_evento.param_received,
             detected_event->valore_evento.ack_rep_counts);

            /*if (waiting_for_ack==1){
                printf("loop(): I was waiting for an ACK (?!?!?!?) so I should do something\r\n");
                // code 
            } else
            {
                printf("loop(): I received a valid COMMAND (open, close, sendreport) from MASTER OR MOBILE station that I should satisfy\r\n");
                // code 
            } */
        } else {
            vTaskDelay(500/portTICK_RATE_MS);
            return;
        }
    } //if (STATION_ROLE==STATIONSLAVE)

    if (STATION_ROLE==STATIONMOBILE){ //BEHAVE AS MOBILE STATION
        //addr_from=ADDR_MOBILE_STATION;
        printf("n######################This is the Mobile station######################\n");
        if (detected_event->id_evento == IO_INPUT_ACTIVE) { // CHECKING IF MY INPUT PIN TOLD ME TO SEND A CMD TO SLAVE STATION
            if (detected_event->valore_evento.value_of_input==1){ // NOTHING TO DO: 
                printf("\r\n\r\n\r\n\r\n\r\n\r\nEVENTO SU I_O PIN GESTITO CORRETTAMENTE!!!!!\r\n");
                return;
            } else { //SENDING COMMAND TO OPEN
                /*addr_from=ADDR_MOBILE_STATION;
                addr_to=ADDR_SLAVE_STATION; //ADDR_MOBILE_STATION;
                strcpy((char *)cmd_tosend,"APRI");
                strcpy((char *)param_tosend,"0");
                printf("loop():cmd_tosend: %s\r\n",cmd_tosend);
                printf("loop():param_tosend: %s\r\n",param_tosend);*/
                //def nuova func: send_msg(UART_NUM_2, addr_from, addr_to,cmd_tosend, param_tosend, rep_counts);
                //uso nuova func: send_msg(UART_NUM_2, ADDR_MASTER_STATION, ADDR_SLAVE_STATION,"APRI", "0", rep_counts);

                /*unsigned char rep_counts=0;//++pp;
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
                waiting_for_cmdinputack=1;*/
                ;
            }
        } else if (detected_event->id_evento == RECEIVED_MSG) { 
            printf("loop(): This was the msg for me:\r\n");
            printf("loop():cmd_received: %s\r\n",detected_event->valore_evento.cmd_received);
            printf("loop():param_received: %s\r\n",detected_event->valore_evento.param_received);
            printf("loop():ack_rep_counts: %d\r\n",detected_event->valore_evento.ack_rep_counts);
            printf("loop():sender: %d\r\n",detected_event->valore_evento.sender);

            /*if (waiting_for_ack==1){
                printf("loop(): I was waiting for an ACK so I should do something\r\n");
                // code 
            } else
            {
                printf("loop(): I received a valid message from ?!?!?! station that I should forward to ?!?!?!\r\n");
                // code 
            } */
        } else {
            vTaskDelay(500/portTICK_RATE_MS);
            return;
        }
    }  //if (STATION_ROLE==STATIONMOBILE)

    return; 
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