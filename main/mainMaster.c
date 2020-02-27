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
#include "freertos/event_groups.h"

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
//#include "manageUART.h"
#include "typeconv.h"
#include "messagehandle.h"
#include "auxiliaryfuncs.h"
#include "eventengine.h"

////////////////////////////////////////////// GLOBAL VARIABLES //////////////////////////////////////////////
gpio_num_t miogpio_input_command_pin[NUM_HANDLED_INPUTS];
static EventGroupHandle_t my_event_group;
const int TRIGGERED = BIT0;
static const char *TAG1 = "OnlyHC12App";
////////////////////////////////////////////// SETUP FUNCTIONS //////////////////////////////////////////////
void setup(commands* my_commands, commands* rcv_commands){
    ESP_LOGW(TAG1,"==============================");
    ESP_LOGW(TAG1,"Welcome to HC12 control App");
    ESP_LOGW(TAG1,"Setting up...................");

    /* CONFIGURING PRESENTATION BLINK LED */
    //gpio_pad_select_gpio((gpio_num_t)BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    //gpio_set_direction((gpio_num_t)BLINK_GPIO, GPIO_MODE_OUTPUT);
    //gpio_set_level((gpio_num_t)BLINK_GPIO, 1); //switch the led OFF
    //ESP_LOGW(TAG1,"Set up presentation led ended");

    /* CONFIGURING OUTPUT LEDS */
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PINS;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    ESP_LOGW(TAG1,"Set up of output leds ended");

    /* CONFIGURING COMMAND INPUT */
    //bit mask of the pins, use GPIO4 here
    io_conf.pin_bit_mask = GPIO_INPUT_COMMAND_PINS;
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
    ESP_LOGW(TAG1,"Set up of command inputs ended");

    miogpio_input_command_pin[0]=(gpio_num_t) GPIO_INPUT_COMMAND_PIN_UNO;
    miogpio_input_command_pin[1]=(gpio_num_t) GPIO_INPUT_COMMAND_PIN_DUE;

    for (int i=0; i<NUM_HANDLED_INPUTS; i++ )  {
        printf("IN[%u] =%u; gpio_input_command_pin[%u]=%u\r\n",i,(unsigned char) gpio_get_level(miogpio_input_command_pin[i]),i,miogpio_input_command_pin[i]);
    }

    //Initializing Radio HC12
    setupmyRadioHC12();
    ESP_LOGW(TAG1,"Set up of HC12 Radio ended");

   	my_event_group = xEventGroupCreate();
    ESP_LOGW(TAG1,"Event Group Created");

    //Initializing DB
    for (unsigned char i=0; i<NUM_MAX_CMDS ; i++){
        memset(my_commands->commands_status[i].cmd,0,sizeof(my_commands->commands_status[i].cmd));
        memset(my_commands->commands_status[i].param,0,sizeof(my_commands->commands_status[i].param));
        my_commands->commands_status[i].rep_counts=0;
        my_commands->commands_status[i].num_checks=0;
        my_commands->commands_status[i].addr_pair=0xFF; //command is not of interest (is not yet valid)
        my_commands->commands_status[i].uart_controller=0x00; //uart is unknown

        memset(rcv_commands->commands_status[i].cmd,0,sizeof(rcv_commands->commands_status[i].cmd));
        memset(rcv_commands->commands_status[i].param,0,sizeof(rcv_commands->commands_status[i].param));
        rcv_commands->commands_status[i].rep_counts=0;
        rcv_commands->commands_status[i].num_checks=0;
        rcv_commands->commands_status[i].addr_pair=0xFF; //command is not of interest (is not yet valid)
        rcv_commands->commands_status[i].uart_controller=0x00; //uart is unknown
    }
    my_commands->num_cmd_under_processing=0;
    rcv_commands->num_cmd_under_processing=0;

    //presentation blinking
    presentBlink(BLINK_GPIO,NUM_PRES_BLINKS);
    ESP_LOGW(TAG1,"Set up of everything ended");
    ESP_LOGW(TAG1,"==============================");
    
    return;
}

void give_gpio_feedback(char unsigned OK_val, char unsigned NOK_val){
    #ifdef OK_GPIO        
    ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)NOK_GPIO, NOK_val)); 
    ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)OK_GPIO, OK_val)); 
    #endif
    if (OK_val || NOK_val){ //if I lit any LED then shall trigger TIMER (timeout task)
        xEventGroupSetBits(my_event_group, TRIGGERED);
    }
    return;
}

unsigned char is_rcv_a_new_cmd(commands* rcv_commands, evento* last_event){ //return 0 if already present; 1 if the event is a new command
    //if this command is the first of the repetition and is not an ACK (so it is an actually new command)
    //I am inserting this new command in commands_status array to be tracked
    //I also accept a command if the received repetition count is less then of what I have previously received
    unsigned char already_rcv=0;
    if (!(strncmp2(last_event->valore_evento.cmd_received,(const unsigned char*) "ACK",3)==0)){ //this is only a safeguard I am not considering ACKs
        for (int i=0;i<rcv_commands->num_cmd_under_processing;i++){
            if ( (strncmp2(rcv_commands->commands_status[i].cmd,last_event->valore_evento.cmd_received,strlen2(last_event->valore_evento.cmd_received))==0)&&
                 (strncmp2(rcv_commands->commands_status[i].param,last_event->valore_evento.param_received,strlen2(last_event->valore_evento.param_received))==0)&&
                 (rcv_commands->commands_status[i].addr_pair==last_event->valore_evento.pair_addr)//&& se togli questo ti levi il problema che potresti aver ricevuto un comando doppio perchè forwardato
                 //(rcv_commands->commands_status[i].rep_counts < last_event->valore_evento.ack_rep_counts ) //this last '&&' condition is for accepting commands with lower repetition counts: this means that the command has been volontarily re-issued 
                 ) {
                already_rcv=1;
                printf("is_rcv_a_new_cmd(): This is an already received command discarding it: cmd: \"%s\"\n prm: %s\n rep counts:%u\n pair_addr: %u\n uart: %u\r\n", last_event->valore_evento.cmd_received, 
                last_event->valore_evento.param_received, last_event->valore_evento.ack_rep_counts, last_event->valore_evento.pair_addr, last_event->valore_evento.uart_controller);
                vTaskDelay(500/portTICK_RATE_MS);
            }
        }
        if (!already_rcv){
            printf("is_rcv_a_new_cmd(): This is an actual NEW command inserting in commands_received list: ^%s^\n", last_event->valore_evento.cmd_received);
            vTaskDelay(500/portTICK_RATE_MS);
            strcpy2(rcv_commands->commands_status[rcv_commands->num_cmd_under_processing].cmd, last_event->valore_evento.cmd_received);
            strcpy2(rcv_commands->commands_status[rcv_commands->num_cmd_under_processing].param,last_event->valore_evento.param_received);
            rcv_commands->commands_status[rcv_commands->num_cmd_under_processing].rep_counts=last_event->valore_evento.ack_rep_counts;
            rcv_commands->commands_status[rcv_commands->num_cmd_under_processing].num_checks=0; 
            rcv_commands->commands_status[rcv_commands->num_cmd_under_processing].addr_pair=last_event->valore_evento.pair_addr;
            rcv_commands->commands_status[rcv_commands->num_cmd_under_processing].uart_controller=last_event->valore_evento.uart_controller;
            rcv_commands->num_cmd_under_processing++;
        }
    }
    return (1-already_rcv);
}
////////////////////////////////////////////// TIMEOUT FUNCTIONS //////////////////////////////////////////////
void timeout_task(void *pvParameter) {
	
	while(1) {
		// wait the timeout period
		EventBits_t uxBits;
		uxBits = xEventGroupWaitBits(my_event_group, TRIGGERED, true, true, CONFIG_TIMEOUT / portTICK_PERIOD_MS);
		
		// if found bit was not set, the function returned after the timeout period
		if((uxBits & TRIGGERED) == 0) {
			// turn both leds off
            give_gpio_feedback(0,0);
		}
	}
}
////////////////////////////////////////////// CORE FUNCTIONS //////////////////////////////////////////////
void loop(commands* my_commands, commands* rcv_commands){
    evento* detected_event;
    unsigned char cmd_param[FIELD_MAX];
    //unsigned char cmd_cmdtosend[FIELD_MAX];

    printf("loop(): *******************************************Entering loop() function!!!!!\r\n");
    if(my_commands->num_cmd_under_processing==0) printf("loop(): commands_status is empty!!!!\r\n");
    for(unsigned char l=0;l<my_commands->num_cmd_under_processing;l++){
        printf("loop(): commands_status[%u].cmd: %s\r\n",l,my_commands->commands_status[l].cmd);
        printf("loop(): commands_status[%u].prm: %s\r\n",l,my_commands->commands_status[l].param);
        printf("loop(): commands_status[%u].rep_counts: %u\r\n",l,my_commands->commands_status[l].rep_counts);
        printf("loop(): commands_status[%u].num_checks: %u\r\n",l,my_commands->commands_status[l].num_checks);
        printf("loop(): commands_status[%u].addr_pair: %u\r\n",l,my_commands->commands_status[l].addr_pair);
        printf("loop(): commands_status[%u].uart_controller: %u\r\n",l,my_commands->commands_status[l].uart_controller);
    }
    
    for(unsigned char l=0;l<NUM_HANDLED_INPUTS;l++){
        printf("loop(): miogpio_input_command_pin[%u]: %u\r\n",l,(unsigned char) miogpio_input_command_pin[l]);
    }

    //Listening for any event to occour
    detected_event=detect_event(UART_NUM_2, miogpio_input_command_pin, my_commands, rcv_commands);
    
    if (STATION_ROLE==STATIONMASTER){ //BEHAVE AS MASTER STATION
        printf("\nloop(): ######################This is the Master station######################\n");

        if (detected_event->type_of_event == IO_INPUT_ACTIVE) { // CHECKING IF MY INPUT PIN TOLD ME TO SEND A CMD TO SLAVE STATION
            if (detected_event->valore_evento.value_of_input==1){ // NOTHING TO DO: 
                printf("\r\nloop(): EVENTO SU I_O PIN %u GESTITO CORRETTAMENTE!!!!!\r\n", detected_event->valore_evento.input_number);
                //ESP_LOGW(TAG1,"loop(): The detected event is a BUTTON press:\r\n");
                fflush(stdout);
                printf("loop():detected_event->type_of_event: %u\r\n",detected_event->type_of_event);
                printf("loop():detected_event->valore_evento.input_number: %u\r\n",detected_event->valore_evento.input_number);
                printf("loop():detected_event->valore_evento.value_of_input: %u\r\n",detected_event->valore_evento.value_of_input);
                
            } else { //SENDING COMMAND TO OPEN
                //num_retries_cmds_frominput=0;
                /*addr_pair=ADDR_SLAVE_STATION; //ADDR_MOBILE_STATION;
                strcpy((char *)cmd_tosend,"APRI");
                strcpy((char *)param_tosend,"0");
                invia_comando(UART_NUM_2, addr_from, addr_pair, cmd_tosend, param_tosend, 1);*/
                give_gpio_feedback(0,0); //swithces off NOK and OK led since a new command is going to be issued
                sprintf((char *)cmd_param,"%d",detected_event->valore_evento.input_number);
                invia_comando(UART_NUM_2, my_commands, ADDR_MASTER_STATION, ADDR_SLAVE_STATION, (const unsigned char *) "APRI", (const unsigned char *) cmd_param, 1);
            }
        } else if (detected_event->type_of_event == RECEIVED_MSG) { 
            ESP_LOGW(TAG1,"loop(): The detected event is a msg for me! With this content:\r\n");
            printf("loop():detected_event->valore_evento.cmd_received: %s\r\n",detected_event->valore_evento.cmd_received);
            printf("loop():detected_event->valore_evento.param_received: %s\r\n",detected_event->valore_evento.param_received);
            printf("loop():detected_event->valore_evento.ack_rep_counts: %u\r\n",detected_event->valore_evento.ack_rep_counts);
            printf("loop():detected_event->valore_evento.pair_addr: %u\r\n",detected_event->valore_evento.pair_addr);
            
            //I am sending ACK to the sending station in any case (i.e. even if a previous repetition of the same command has already been received)
            /*if (!(strncmp2((const unsigned char*)"ACK_",detected_event->valore_evento.cmd_received,4)==0)){
                strcpy2(cmd_cmdtosend,(const unsigned char *) "ACK_");
                invia_comando(UART_NUM_2, my_commands, ADDR_MASTER_STATION, detected_event->valore_evento.pair_addr, (const unsigned char *) strcat2(cmd_cmdtosend,detected_event->valore_evento.cmd_received), detected_event->valore_evento.param_received,
                detected_event->valore_evento.ack_rep_counts);
            }*/
            invia_ack(UART_NUM_2,my_commands,ADDR_MASTER_STATION,detected_event);
            //check if this one is an actually new command and in case I track in the received commands
            if (is_rcv_a_new_cmd(rcv_commands,detected_event)){
                //if I have received a command I am forwarding it to the slave station (for now MASTER station just forwards received commands)
                give_gpio_feedback(0,0); //swithces off NOK and OK led since a new command is going to be issued
                invia_comando(UART_NUM_2, my_commands, ADDR_MASTER_STATION, ADDR_SLAVE_STATION, detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received, 1); //FORWARD RECEIVED COMMAND TO SLAVE STATION
                //In SLAVE STATION I insetad would expect to actuate something
            } 
        } else if (detected_event->type_of_event == RECEIVED_ACK) {
            ESP_LOGW(TAG1,"loop(): received and ACK for command:\r\n");
            give_gpio_feedback(1,0); //NOK led is switched off and OK led is lit since an ACK has been received
            printf("loop():detected_event->valore_evento.cmd_received: %s\r\n",detected_event->valore_evento.cmd_received);
            printf("loop():detected_event->valore_evento.param_received: %s\r\n",detected_event->valore_evento.param_received);
            printf("loop():detected_event->valore_evento.ack_rep_counts: %u\r\n",detected_event->valore_evento.ack_rep_counts);
            printf("loop():detected_event->valore_evento.pair_addr: %u\r\n",detected_event->valore_evento.pair_addr);
            vTaskDelay(1000/portTICK_RATE_MS);
        } else if (detected_event->type_of_event == FAIL_TO_SEND_CMD) {
            ESP_LOGW(TAG1,"loop(): failure to send command:\r\n");
            give_gpio_feedback(0,1); //NOK led is lit and OK led is switched off since a FAILURE happened
            
            printf("loop():detected_event->valore_evento.cmd_received: %s\r\n",detected_event->valore_evento.cmd_received);
            printf("loop():detected_event->valore_evento.param_received: %s\r\n",detected_event->valore_evento.param_received);
            printf("loop():detected_event->valore_evento.ack_rep_counts: %u\r\n",detected_event->valore_evento.ack_rep_counts);
            printf("loop():detected_event->valore_evento.pair_addr: %u\r\n",detected_event->valore_evento.pair_addr);
            vTaskDelay(2500/portTICK_RATE_MS);
        } else {
            vTaskDelay(500/portTICK_RATE_MS);
            return;
        }
    } //if (STATION_ROLE==STATIONMASTER)

    if (STATION_ROLE==STATIONSLAVE){ //BEHAVE AS SLAVE STATION
        //addr_from=ADDR_SLAVE_STATION;
        printf("\nn######################This is the Slave station######################\n");
        if (detected_event->type_of_event == RECEIVED_MSG) { 
            ESP_LOGW(TAG1,"loop(): This was the msg for me:\r\n");
            printf("loop():cmd_received: %s\r\n",detected_event->valore_evento.cmd_received);
            printf("loop():param_received: %s\r\n",detected_event->valore_evento.param_received);
            printf("loop():ack_rep_counts: %u\r\n",detected_event->valore_evento.ack_rep_counts);
            printf("loop():pair_addr: %u\r\n",detected_event->valore_evento.pair_addr);
            
            //I am sending ACK to the sending station in any case (i.e. even if a previous repetition of the same command has already been received)
            //I am sending ACK to the sending station in any case (i.e. even if a previous repetition of the same command has already been received)
            /*if (!(strncmp2((const unsigned char*)"ACK_",detected_event->valore_evento.cmd_received,4)==0)){
                strcpy2(cmd_cmdtosend,(const unsigned char *) "ACK_");
                invia_comando(UART_NUM_2, my_commands, ADDR_SLAVE_STATION, detected_event->valore_evento.pair_addr, (const unsigned char *) strcat2(cmd_cmdtosend,detected_event->valore_evento.cmd_received), detected_event->valore_evento.param_received,
                detected_event->valore_evento.ack_rep_counts);
            }*/
            invia_ack(UART_NUM_2,my_commands, ADDR_SLAVE_STATION, detected_event);
            //I am sending ACK to the sending station
            //invia_comando(UART_NUM_2, my_commands, ADDR_SLAVE_STATION, detected_event->valore_evento.pair_addr, (const unsigned char *) "ACK_APRI", detected_event->valore_evento.param_received,
            // detected_event->valore_evento.ack_rep_counts);

            //check if this one is an actually new command and in case I track in the received commands
            if (is_rcv_a_new_cmd(rcv_commands,detected_event)){
                //Actuating APRI commands
                if (strncmp2(detected_event->valore_evento.cmd_received,(const unsigned char *) "APRI",4)==0){
                    if (strncmp2(detected_event->valore_evento.param_received, (const unsigned char *) "0", 1)==0){
                        printf("loop():ACTUATING RECEIVED CMD: %s -- %s\r\n",detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        vTaskDelay(2500/portTICK_RATE_MS);

                        ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)SLAVE_GARAGE_CMD_GPIO, 0)); //closing GARAGE relay             
                        vTaskDelay(CONFIG_ON_COMMAND_LENGTH/portTICK_RATE_MS);
                        ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)SLAVE_GARAGE_CMD_GPIO, 1)); //relasing GARAGE relay             
                    }     
                    if (strncmp2(detected_event->valore_evento.param_received, (const unsigned char *) "1", 1)==0){
                        printf("loop():ACTUATING RECEIVED CMD: %s -- %s\r\n",detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        vTaskDelay(2500/portTICK_RATE_MS);
                        ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)SLAVE_BIGGATE_CMD_GPIO, 0)); //closing BIG GATE relay             
                        vTaskDelay(CONFIG_ON_COMMAND_LENGTH/portTICK_RATE_MS);
                        ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)SLAVE_BIGGATE_CMD_GPIO, 1)); //relasing BIG GATE relay             
                    }     
                }
            }
        } else if (detected_event->type_of_event == FAIL_TO_SEND_CMD) {
            ESP_LOGW(TAG1,"loop(): failure to send command:\r\n");
            give_gpio_feedback(0,1); //NOK led is lit and OK led is switched off since a FAILURE happened
            
            printf("loop():detected_event->valore_evento.cmd_received: %s\r\n",detected_event->valore_evento.cmd_received);
            printf("loop():detected_event->valore_evento.param_received: %s\r\n",detected_event->valore_evento.param_received);
            printf("loop():detected_event->valore_evento.ack_rep_counts: %u\r\n",detected_event->valore_evento.ack_rep_counts);
            printf("loop():detected_event->valore_evento.pair_addr: %u\r\n",detected_event->valore_evento.pair_addr);
            vTaskDelay(2500/portTICK_RATE_MS);
        } else {
            vTaskDelay(500/portTICK_RATE_MS);
            return;
        }
    } //if (STATION_ROLE==STATIONSLAVE)

    if (STATION_ROLE==STATIONMOBILE){ //BEHAVE AS MOBILE STATION
        printf("\nloop(): ######################This is the Mobile station######################\n");

        if (detected_event->type_of_event == IO_INPUT_ACTIVE) { // CHECKING IF MY INPUT PIN TOLD ME TO SEND A CMD TO SLAVE STATION
            if (detected_event->valore_evento.value_of_input==1){ // NOTHING TO DO: 
                printf("\r\nloop(): EVENTO SU I_O PIN %u GESTITO CORRETTAMENTE!!!!!\r\n", detected_event->valore_evento.input_number);
                //ESP_LOGW(TAG1,"loop(): The detected event is a BUTTON press:\r\n");
                fflush(stdout);
                printf("loop():detected_event->type_of_event: %u\r\n",detected_event->type_of_event);
                printf("loop():detected_event->valore_evento.input_number: %u\r\n",detected_event->valore_evento.input_number);
                printf("loop():detected_event->valore_evento.value_of_input: %u\r\n",detected_event->valore_evento.value_of_input);
                
            } else { //SENDING COMMAND TO OPEN
                //num_retries_cmds_frominput=0;
                /*addr_pair=ADDR_SLAVE_STATION; //ADDR_MOBILE_STATION;
                strcpy((char *)cmd_tosend,"APRI");
                strcpy((char *)param_tosend,"0");
                invia_comando(UART_NUM_2, addr_from, addr_pair, cmd_tosend, param_tosend, 1);*/
                give_gpio_feedback(0,0); //swithces off NOK and OK led since a new command is going to be issued
                sprintf((char *)cmd_param,"%d",detected_event->valore_evento.input_number);
                //ATTENZIONE SE EMETTI I DUE COMANDI COSì ALLORA LO SLAVE POTREBBE ATTUARE 2 VOLTE IL COMANDO CON EFFETTI INDESIDERATI
                //A MENO DI TOLGIERE IN is_rcv_a_new_cmd() IL CONTROLLO SUL PAIR_ADDRESS
                invia_comando(UART_NUM_2, my_commands, ADDR_MOBILE_STATION, ADDR_SLAVE_STATION, (const unsigned char *) "APRI", (const unsigned char *) cmd_param, 1);
                invia_comando(UART_NUM_2, my_commands, ADDR_MOBILE_STATION, ADDR_MASTER_STATION, (const unsigned char *) "APRI", (const unsigned char *) cmd_param, 1);
            }
        } else if (detected_event->type_of_event == RECEIVED_MSG) { 
            ESP_LOGW(TAG1,"loop(): The detected event is a msg for me! With this content:\r\n");
            printf("loop():detected_event->valore_evento.cmd_received: %s\r\n",detected_event->valore_evento.cmd_received);
            printf("loop():detected_event->valore_evento.param_received: %s\r\n",detected_event->valore_evento.param_received);
            printf("loop():detected_event->valore_evento.ack_rep_counts: %u\r\n",detected_event->valore_evento.ack_rep_counts);
            printf("loop():detected_event->valore_evento.pair_addr: %u\r\n",detected_event->valore_evento.pair_addr);
            
            //I am sending ACK to the sending station in any case (i.e. even if a previous repetition of the same command has already been received)
            /*if (!(strncmp2((const unsigned char*)"ACK_",detected_event->valore_evento.cmd_received,4)==0)){
                strcpy2(cmd_cmdtosend,(const unsigned char *) "ACK_");
                invia_comando(UART_NUM_2, my_commands, ADDR_MASTER_STATION, detected_event->valore_evento.pair_addr, (const unsigned char *) strcat2(cmd_cmdtosend,detected_event->valore_evento.cmd_received), detected_event->valore_evento.param_received,
                detected_event->valore_evento.ack_rep_counts);
            }*/
            invia_ack(UART_NUM_2,my_commands,ADDR_MOBILE_STATION,detected_event);
            //check if this one is an actually new command and in case I track in the received commands
            if (is_rcv_a_new_cmd(rcv_commands,detected_event)){
                //if I have received a command I am forwarding it to the slave station (for now MOBILE station just forwards received commands)
                give_gpio_feedback(0,0); //swithces off NOK and OK led since a new command is going to be issued
                invia_comando(UART_NUM_2, my_commands, ADDR_MOBILE_STATION, ADDR_SLAVE_STATION, detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received, 1); //FORWARD RECEIVED COMMAND TO SLAVE STATION
                //In SLAVE STATION I insetad would expect to actuate something
            } 
        } else if (detected_event->type_of_event == RECEIVED_ACK) {
            ESP_LOGW(TAG1,"loop(): received and ACK for command:\r\n");
            give_gpio_feedback(1,0); //NOK led is switched off and OK led is lit since an ACK has been received
            printf("loop():detected_event->valore_evento.cmd_received: %s\r\n",detected_event->valore_evento.cmd_received);
            printf("loop():detected_event->valore_evento.param_received: %s\r\n",detected_event->valore_evento.param_received);
            printf("loop():detected_event->valore_evento.ack_rep_counts: %u\r\n",detected_event->valore_evento.ack_rep_counts);
            printf("loop():detected_event->valore_evento.pair_addr: %u\r\n",detected_event->valore_evento.pair_addr);
            vTaskDelay(1000/portTICK_RATE_MS);
        } else if (detected_event->type_of_event == FAIL_TO_SEND_CMD) {
            ESP_LOGW(TAG1,"loop(): failure to send command:\r\n");
            give_gpio_feedback(0,1); //NOK led is lit and OK led is switched off since a FAILURE happened
            
            printf("loop():detected_event->valore_evento.cmd_received: %s\r\n",detected_event->valore_evento.cmd_received);
            printf("loop():detected_event->valore_evento.param_received: %s\r\n",detected_event->valore_evento.param_received);
            printf("loop():detected_event->valore_evento.ack_rep_counts: %u\r\n",detected_event->valore_evento.ack_rep_counts);
            printf("loop():detected_event->valore_evento.pair_addr: %u\r\n",detected_event->valore_evento.pair_addr);
            vTaskDelay(2500/portTICK_RATE_MS);
        } else {
            vTaskDelay(500/portTICK_RATE_MS);
            return;
        }
    }  //if (STATION_ROLE==STATIONMOBILE)

    return; //to be removed since it is already done in each STATION_ROLE if statement
}

// Main application
void app_main() {
    static commands my_commands;
    static commands rcv_commands;
    
    setup(&my_commands,&rcv_commands);
    xTaskCreate(&timeout_task, "timeout_task", 2048, NULL, 5, NULL);
    ESP_LOGE(TAG1,"Entering while loop!!!!!\r\n");
    while(1)
        loop(&my_commands,&rcv_commands);
    fflush(stdout);
    return;
}