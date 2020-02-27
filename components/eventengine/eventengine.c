#include "esp_log.h"
//#include "driver/gpio.h"

#include "auxiliaryfuncs.h"
#include "typeconv.h"
#include "manageUART.h"
#include "messagehandle.h"
#include "eventengine.h"

static const char *TAG = "EventE";

void list_commands_status(commands* my_commands){
    unsigned char i=0;
    /////REPORT commands_status[] BEFORE processing
    if (my_commands->num_cmd_under_processing == 0) {
        printf("clean_acknowledged_cmds():commands_status is empty\r\n");
    }
    while (i<my_commands->num_cmd_under_processing){
        printf("list_commands_status(): my_commands.commands_status[%u].cmd: %s\r\n",i,my_commands->commands_status[i].cmd);
        printf("list_commands_status(): my_commands.commands_status[%u].param: %s\r\n",i,my_commands->commands_status[i].param);
        printf("list_commands_status(): my_commands.commands_status[%u].rep_counts: %u\r\n",i,my_commands->commands_status[i].rep_counts);
        printf("list_commands_status(): my_commands.commands_status[%u].num_checks: %u\r\n",i,my_commands->commands_status[i].num_checks);
        printf("list_commands_status(): my_commands.commands_status[%u].addr_pair: %u\r\n",i,my_commands->commands_status[i].addr_pair);
        printf("list_commands_status(): my_commands.commands_status[%u].uart_controller: %u\r\n",i,my_commands->commands_status[i].uart_controller);
        i++;
    }
    return;
}

void clean_cmds_list(commands* commands_list){
    printf("clean_cmds_list(): BEFORE PROCESSING\r\n");
    list_commands_status(commands_list);

    //clean commands_status array from empty cells
    /////processing commands_status[]
    unsigned char i=0;
    while (i<commands_list->num_cmd_under_processing){
        printf("clean_cmds_list(): processing cmds no. %u\r\n",i);
        if (commands_list->commands_status[i].addr_pair == 0xFF){ //this command has been previsously acknowledeged: so it has to be removed
            printf("clean_cmds_list(): cmds no. %u is no longer of interest.....cleaning up\r\n",i);
            unsigned char j=i;
            while (j<commands_list->num_cmd_under_processing-1){
                strcpy2(commands_list->commands_status[j].cmd,commands_list->commands_status[j+1].cmd);
                strcpy2(commands_list->commands_status[j].param,commands_list->commands_status[j+1].param);
                commands_list->commands_status[j].rep_counts=commands_list->commands_status[j+1].rep_counts;
                commands_list->commands_status[j].num_checks=commands_list->commands_status[j+1].num_checks;
                commands_list->commands_status[j].addr_pair=commands_list->commands_status[j+1].addr_pair;
                commands_list->commands_status[j].uart_controller=commands_list->commands_status[j+1].uart_controller;
                j++;
            }
            commands_list->num_cmd_under_processing--;
            printf("clean_cmds_list(): commands_list->num_cmd_under_processing: %u\r\n",commands_list->num_cmd_under_processing);
            memset(commands_list->commands_status[commands_list->num_cmd_under_processing].cmd,0,sizeof(commands_list->commands_status[i].cmd));
            memset(commands_list->commands_status[commands_list->num_cmd_under_processing].param,0,sizeof(commands_list->commands_status[i].param));
            commands_list->commands_status[commands_list->num_cmd_under_processing].rep_counts=0;
            commands_list->commands_status[commands_list->num_cmd_under_processing].num_checks=0;
            commands_list->commands_status[commands_list->num_cmd_under_processing].addr_pair=0xFF;
            commands_list->commands_status[commands_list->num_cmd_under_processing].uart_controller=0;

            i=-1; //after having shifted one ACKed command then restart scanning from the beginning
        }
        i++;
    }

    /////REPORT commands_status[] AFTER processing
    printf("clean_cmds_list(): AFTER PROCESSING\r\n");
    list_commands_status(commands_list);
    return;
}

unsigned char invia_ack(uart_port_t uart_controller, commands* my_commands, unsigned char addr_from, evento* evento){
    unsigned char cmd_cmdtosend[FIELD_MAX];
    strcpy2(cmd_cmdtosend,(const unsigned char *) "ACK_");
    return(invia_comando(uart_controller, my_commands,addr_from,evento->valore_evento.pair_addr,(const unsigned char *) strcat2(cmd_cmdtosend,evento->valore_evento.cmd_received),evento->valore_evento.param_received,evento->valore_evento.ack_rep_counts));
}

unsigned char invia_comando(uart_port_t uart_controller, commands* my_commands, unsigned char addr_from, unsigned char addr_pair, const unsigned char* cmd, const unsigned char* param, unsigned char rep_counts){
    unsigned char* msg;

    printf("invia_comando():uart_controller: %u\r\n",uart_controller);
    printf("invia_comando():addr_from: %u\r\n",addr_from);
    printf("invia_comando():addr_pair: %u\r\n",addr_pair);
    printf("invia_comando():cmd_tosend: %s\r\n",cmd);
    printf("invia_comando():param_tosend: %s\r\n",param);
    printf("invia_comando():rep_counts: %u\r\n",rep_counts);
    
    msg=pack_msg(addr_from, addr_pair, cmd, param, rep_counts);

    printf("invia_comando(): msg: %s , msg_len: %u\r\n", msg, strlen2(msg));
    //printf("loop(): ***************end:%d\r\n",msg[strlen2(msg)-1]);
    msg[strlen2(msg)]='E';
    msg[strlen2(msg)]='N';
    msg[strlen2(msg)]='D';
    
    //sending msg on UART2
    printf("invia_comando(): msg: %s , msg_len: %u\r\n", msg, strlen2(msg));
    int numbytes=scriviUART(uart_controller, msg);
    
    if (numbytes<0){
        printf("invia_comando(): Fatal error in sending data on UART%u\r\n",uart_controller);
        vTaskDelay(5000 / portTICK_RATE_MS);
        return -1;
    } else {
        printf("invia_comando(): sent %d bytes on UART_NUM_%u\r\n",numbytes,uart_controller);
        vTaskDelay(500 / portTICK_RATE_MS);
        presentBlink(BLINK_GPIO,NUM_PRES_BLINKS);
    }
    
    //if this command is the first of the repetition and is not an ACK (so it is an actually new command)
    //I am inserting this new command in commands_status array to be tracked
    if ((!strncmp2(cmd,(const unsigned char*) "ACK",3)==0)&&(rep_counts==1)){
        printf("invia_comando(): This is an actual command inserting in commands_status: ^%s^\n", cmd);
        strcpy2(my_commands->commands_status[my_commands->num_cmd_under_processing].cmd, cmd);
        strcpy2(my_commands->commands_status[my_commands->num_cmd_under_processing].param,param);
        my_commands->commands_status[my_commands->num_cmd_under_processing].rep_counts=rep_counts; 
        my_commands->commands_status[my_commands->num_cmd_under_processing].num_checks=0; 
        my_commands->commands_status[my_commands->num_cmd_under_processing].addr_pair=addr_pair;
        my_commands->commands_status[my_commands->num_cmd_under_processing].uart_controller=uart_controller;
        my_commands->num_cmd_under_processing++;
    }

    return 0;
}

unsigned char manage_issuedcmd_retries(uart_port_t uart_controller, evento* detected_event, commands *my_commands){
    unsigned char ret = 0;
    unsigned char i=0;
    printf("manage_issuedcmd_retries():BEGIN\r\n");
    printf("manage_issuedcmd_retries():type of event = %u \r\n",detected_event->type_of_event);
    if (!(detected_event->type_of_event==NOTHING)){
        ESP_LOGW(TAG,"manage_issuedcmd_retries():THIS SHOULD NOT HAPPEN\r\n");
    }
    //Managing retries of issued commands
    i=0;
    while (i<my_commands->num_cmd_under_processing){
        if(!(my_commands->commands_status[i].addr_pair==0xFF)){ //if this cmd has not been acknowledeged
            if (my_commands->commands_status[i].num_checks < NUM_MAX_CHECKS_FOR_ACK) { //if it has not yet reached max checks attempts
                my_commands->commands_status[i].num_checks++;
            } else if (my_commands->commands_status[i].rep_counts<NUM_MAX_RETRIES) { //if it has reached max attempts I send a new copy with a rep_count incremented
                my_commands->commands_status[i].num_checks=0;
                my_commands->commands_status[i].rep_counts++;
                invia_comando(uart_controller, my_commands, LOCAL_STATION_ADDRESS,my_commands->commands_status[i].addr_pair,my_commands->commands_status[i].cmd,
                my_commands->commands_status[i].param,my_commands->commands_status[i].rep_counts);
                break;
            } else {
                detected_event->type_of_event=FAIL_TO_SEND_CMD;
                detected_event->valore_evento.value_of_input = 0;
                //memcpy(detected_event->valore_evento.cmd_received,my_commands->commands_status[i].cmd,sizeof(my_commands->commands_status[i].cmd));
                //memcpy(detected_event->valore_evento.param_received,my_commands->commands_status[i].param,sizeof(my_commands->commands_status[i].param));
                detected_event->valore_evento.cmd_received=my_commands->commands_status[i].cmd;
                detected_event->valore_evento.param_received=my_commands->commands_status[i].param;
                detected_event->valore_evento.ack_rep_counts=my_commands->commands_status[i].rep_counts;
                detected_event->valore_evento.pair_addr=my_commands->commands_status[i].addr_pair;
                my_commands->commands_status[i].addr_pair=0xFF;
                ret=1;
            }
        }
        i++;
    }
    return ret;
}

unsigned char manage_rcvcmds_retries(evento* detected_event, commands *rcv_commands){
    //here detected_event is considered only for debugging purpose. Consider removing it.
    unsigned char ret = 0;
    unsigned char i=0;
    printf("manage_rcvcmds_retries():BEGIN\r\n");
    printf("manage_rcvcmds_retries():type of event = %u \r\n",detected_event->type_of_event);
    if (!(detected_event->type_of_event==NOTHING)){
        ESP_LOGW(TAG,"manage_rcvcmds_retries():THIS SHOULD NOT HAPPEN\r\n");
    }
    //Managing retries of issued commands
    i=0;
    while (i<rcv_commands->num_cmd_under_processing){
        if(!(rcv_commands->commands_status[i].addr_pair==0xFF)){ //if this cmd is still valid
            if (rcv_commands->commands_status[i].num_checks < NUM_CHECKS_FOR_ALREADY_RECEIVED) { //if it has not yet reached checks attempts
                rcv_commands->commands_status[i].num_checks++;
            } else { //I consider that after NUM_CHECKS_FOR_ALREADY_RECEIVED check I should not receive a retry of the same message, so I remove from the check-list
                rcv_commands->commands_status[i].addr_pair=0xFF;
                ret=1;
            }
        }
        i++;
    }
    return ret;
}

unsigned char check_rcved_acks(evento* detected_event, commands* my_commands){
    //printf("check_rcv_acks():BEGIN\r\n");
    unsigned char ret = 1;
    unsigned char i=0;
    if (detected_event->type_of_event == RECEIVED_MSG){
        printf("check_rcv_acks():detected_event->type_of_event == RECEIVED_MSG\r\n");
        //check if current event is an ACK
        i=0;
        while (i<my_commands->num_cmd_under_processing){
            printf("check_rcv_acks():detected_event->valore_evento.cmd_received=%s\r\n",detected_event->valore_evento.cmd_received);
            printf("check_rcv_acks():my_commands->commands_status[%u].cmd=%s\r\n",i,my_commands->commands_status[i].cmd);
            printf("check_rcv_acks():detected_event->valore_evento.param_received=%s\r\n",detected_event->valore_evento.param_received);
            printf("check_rcv_acks():my_commands->commands_status[%u].param=%s\r\n",i,my_commands->commands_status[i].param);
            printf("check_rcv_acks():detected_event->valore_evento.ack_rep_counts=%u\r\n",detected_event->valore_evento.ack_rep_counts);
            printf("check_rcv_acks():my_commands->commands_status[%u].rep_counts=%u\r\n",i,my_commands->commands_status[i].rep_counts);
            printf("check_rcv_acks():.detected_event->valore_eventopair_addr=%u\r\n",detected_event->valore_evento.pair_addr);
            printf("check_rcv_acks():my_commands->commands_status[%u].addr_pair=%u\r\n",i,my_commands->commands_status[i].addr_pair);
            printf("check_rcv_acks():uart_controller=%u\r\n",detected_event->valore_evento.uart_controller);
            printf("check_rcv_acks():my_commands->commands_status[%u].uart_controller=%u\r\n",i,my_commands->commands_status[i].uart_controller);
                        
            if((strcmpACK(detected_event->valore_evento.cmd_received,my_commands->commands_status[i].cmd)==0)&&
                (strcmp2(detected_event->valore_evento.param_received,my_commands->commands_status[i].param)==0)&&
                (detected_event->valore_evento.ack_rep_counts==my_commands->commands_status[i].rep_counts)&& //sicuro che convenga lasciarlo??
                (detected_event->valore_evento.pair_addr==my_commands->commands_status[i].addr_pair)&& 
                (detected_event->valore_evento.uart_controller==my_commands->commands_status[i].uart_controller) //da togliere??
                ){
                    ret=0; //ACK_RECEIVED
                    detected_event->type_of_event=RECEIVED_ACK; //FORSE DA SPOSTARE FUORI
                    my_commands->commands_status[i].addr_pair=0xFF;
                    printf("check_rcv_acks(): CMD NUMBER %u HAS BEEN ACKNOWLEDGED!!!!!!!!\n",i);       
                    return ret; //with this return only one command is acknowleded with a received ACK, without this return all the same cmd+param+rep_count+addr_pair will be ackowledeged with one only ACK
            }
            i++;
        } //while (i<num_cmd_under_processing)
    }
    printf("check_rcv_acks(): exit with code: %u\r\n",ret);
    return ret;
}

evento* detect_event(uart_port_t uart_controller, const gpio_num_t* mygpio_input_command_pin, commands* my_commands, commands* rcv_commands){
    printf("detectEvent(): Calling function clean_acknowledged_cmds()\r\n");
    clean_cmds_list(my_commands);
    clean_cmds_list(rcv_commands);

    static evento detected_event;
    unsigned char IN[NUM_HANDLED_INPUTS];
    static unsigned char IN_PREC[NUM_HANDLED_INPUTS];
    
    for (int i=0; i<sizeof(IN); i++ )  {
        IN[i] = (unsigned char) gpio_get_level(mygpio_input_command_pin[i]);
        printf("IN[%u] =%u; gpio_input_command_pin[%u]=%u\r\n",i,IN[i],i,(unsigned char)mygpio_input_command_pin[i]);
    }
    ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)BLINK_GPIO, (uint32_t) IN[0])); //echoes PIN1 on OK_GPIO led
    //ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)NOK_GPIO, (uint32_t) (1-IN[0]))); //echoes PIN1 on OK_GPIO led

    //Reset event variable
    detected_event.type_of_event = NOTHING;
    detected_event.valore_evento.input_number = 0;
    detected_event.valore_evento.value_of_input = 0;
    detected_event.valore_evento.cmd_received=NULL;
    detected_event.valore_evento.param_received=NULL;
    detected_event.valore_evento.ack_rep_counts=0;
    detected_event.valore_evento.pair_addr=0;
    detected_event.valore_evento.uart_controller=(uart_port_t) 0;
    
    //Detect if IO_INPUT went low 
    for (int i=0; i<sizeof(IN) ; i++){
        printf("i=%u, sizeof(IN)=%u\r\n",i,sizeof(IN));
        if (!(IN[i]==IN_PREC[i])){
            detected_event.type_of_event = IO_INPUT_ACTIVE;
            printf("detectEvent(): IN[%u]: %u ; IN_PREC[%u]: %u\r\n",i,IN[i],i,IN_PREC[i]);
            printf("detectEvent(): !!!!!!! IO Input Event Occurred !!!!!!!\r\n");
            detected_event.valore_evento.input_number = i;
            detected_event.valore_evento.value_of_input = IN[i];
            detected_event.valore_evento.cmd_received=NULL;
            detected_event.valore_evento.param_received=NULL;
            detected_event.valore_evento.ack_rep_counts=0;
            detected_event.valore_evento.pair_addr=0; //myself
            detected_event.valore_evento.uart_controller=uart_controller;
            IN_PREC[i]=IN[i];
            return &detected_event; //with this return just one input at a time is detected and handled
        }
        IN_PREC[i]=IN[i];
    }

    //Detect if a message has been received from any station reading data from the uart controller
    unsigned char *line1;
    line1 = read_line(uart_controller);
    printf("detect_event(): line read from UART%u: ",uart_controller);
    stampaStringa((char *) line1);

    unsigned char station_iter=0;
    unsigned char allowed_addr_pair=LOCAL_STATION_ADDRESS;

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
        
        unsigned char ret=unpack_msg(line1, allowed_addr_from, allowed_addr_pair, &cmd_received, &param_received, &ack_rep_counts);
        printf("\ndetect_event(): unpack returned with code: %d!\r\n",ret);
        printf("detect_event(): cmd_received: %s\r\n",cmd_received);
        printf("detect_event(): param_received: %s\r\n",param_received);
        printf("detect_event(): ack_rep_counts: %d\r\n",ack_rep_counts);
        printf("detect_event(): uart_controller: %d\r\n",uart_controller);

        if (ret==0){
            detected_event.type_of_event = RECEIVED_MSG;
            printf("detectEvent(): !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!A message of interest has been detected!!!!!!!\r\n");
            detected_event.valore_evento.input_number = 0;
            detected_event.valore_evento.value_of_input = 0;
            detected_event.valore_evento.cmd_received=cmd_received;
            detected_event.valore_evento.param_received=param_received;
            detected_event.valore_evento.ack_rep_counts=ack_rep_counts;
            detected_event.valore_evento.pair_addr=allowed_addr_from; //the actual pair_addr
            detected_event.valore_evento.uart_controller=uart_controller;
            station_iter=1;//return &detected_event;
        } else {
            printf("detectEvent(): message was not of interest. RetCode=%u\r\n",ret);
        }
        station_iter++;
    }
    //before return check if:
    //- I have not received anyhting (e.g. not a message nor an input command) OR
    //- the received MSG is NOT an ACK
    //then I have to manage the retries looking for a FAIL_TO_SEND_CMD event
    check_rcved_acks(&detected_event, my_commands);
    if (detected_event.type_of_event==NOTHING) {
        //if I call this then I have not yet found an event (neither an input_cmd nor a message nor an ACK) 
        //so I am 
        manage_issuedcmd_retries(uart_controller,&detected_event, my_commands);
        manage_rcvcmds_retries(&detected_event, rcv_commands);
    }

    return &detected_event;
}