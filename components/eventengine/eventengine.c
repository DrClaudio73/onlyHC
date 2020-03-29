#include "esp_log.h"
//#include "driver/gpio.h"

#include "auxiliaryfuncs.h"
#include "typeconv.h"
#include "manageUART.h"
#include "messagehandle.h"
#include "eventengine.h"

static const char *TAG_EVT = "EvtEng";

unsigned char is_rcv_a_new_cmd(commands_t *rcv_commands, evento_t *last_event)
{ //return 0 if already present; 1 if the event is a new command
    //if this command is the first of the repetition and is not an ACK (so it is an actually new command)
    //I am inserting this new command in commands_status array to be tracked
    //I also accept a command if the received repetition count is less then of what I have previously received
    unsigned char already_rcv = 0;
    if (!(strncmp2(last_event->valore_evento.cmd_received, (const unsigned char *)"ACK", 3) == 0))
    { //this is only a safeguard I am not considering ACKs
        for (int i = 0; i < rcv_commands->num_cmd_under_processing; i++)
        {
            if ((strncmp2(rcv_commands->commands_status[i].cmd, last_event->valore_evento.cmd_received, strlen2(last_event->valore_evento.cmd_received)) == 0) &&
                (strncmp2(rcv_commands->commands_status[i].param, last_event->valore_evento.param_received, strlen2(last_event->valore_evento.param_received)) == 0) &&
                (rcv_commands->commands_status[i].addr_pair == last_event->valore_evento.pair_addr) //&& se togli questo ti levi il problema che potresti aver ricevuto un comando doppio perchè forwardato
                //(rcv_commands->commands_status[i].rep_counts < last_event->valore_evento.ack_rep_counts ) //this last '&&' condition is for accepting commands with lower repetition counts: this means that the command has been volontarily re-issued
            )
            {
                already_rcv = 1;
                ESP_LOGI(TAG_EVT,"is_rcv_a_new_cmd(): This is an already received command discarding it: cmd: \"%s\"\n prm: %s\n rep counts:%u\n pair_addr: %u\n uart: %u", last_event->valore_evento.cmd_received,
                       last_event->valore_evento.param_received, last_event->valore_evento.ack_rep_counts, last_event->valore_evento.pair_addr, last_event->valore_evento.uart_controller);
                vTaskDelay(500 / portTICK_RATE_MS);
            }
        }

        if (!already_rcv)
        {
            if (rcv_commands->num_cmd_under_processing < NUM_MAX_CMDS)
            {
                ESP_LOGI(TAG_EVT,"is_rcv_a_new_cmd(): This is an actual NEW command inserting in commands_received list: ^%s^\n", last_event->valore_evento.cmd_received);
                vTaskDelay(500 / portTICK_RATE_MS);
                strcpy2(rcv_commands->commands_status[rcv_commands->num_cmd_under_processing].cmd, last_event->valore_evento.cmd_received);
                strcpy2(rcv_commands->commands_status[rcv_commands->num_cmd_under_processing].param, last_event->valore_evento.param_received);
                rcv_commands->commands_status[rcv_commands->num_cmd_under_processing].rep_counts = last_event->valore_evento.ack_rep_counts;
                rcv_commands->commands_status[rcv_commands->num_cmd_under_processing].num_checks = 0;
                rcv_commands->commands_status[rcv_commands->num_cmd_under_processing].addr_pair = last_event->valore_evento.pair_addr;
                rcv_commands->commands_status[rcv_commands->num_cmd_under_processing].uart_controller = last_event->valore_evento.uart_controller;
                rcv_commands->num_cmd_under_processing++;
            }
            else
            {
                ESP_LOGW(TAG_EVT,"is_rcv_a_new_cmd(): This is an actual NEW command but I cannot insert it in commands_received list: ^%s^\n", last_event->valore_evento.cmd_received);
            }
        }
    }
    return (1 - already_rcv);
}

void list_commands_status(commands_t *my_commands, char *name)
{
    unsigned char i = 0;
    /////REPORT commands_status[] BEFORE processing
    if (my_commands->num_cmd_under_processing == 0)
    {
        ESP_LOGD(TAG_EVT,"list_commands_status(): %s commands list is empty", name);
    }
    while (i < my_commands->num_cmd_under_processing)
    {
        ESP_LOGD(TAG_EVT,"list_commands_status(): %s.commands_status[%u].cmd: %s", name, i, my_commands->commands_status[i].cmd);
        ESP_LOGD(TAG_EVT,"list_commands_status(): %s.commands_status[%u].param: %s", name, i, my_commands->commands_status[i].param);
        ESP_LOGD(TAG_EVT,"list_commands_status(): %s.commands_status[%u].rep_counts: %u", name, i, my_commands->commands_status[i].rep_counts);
        ESP_LOGD(TAG_EVT,"list_commands_status(): %s.commands_status[%u].num_checks: %u", name, i, my_commands->commands_status[i].num_checks);
        ESP_LOGD(TAG_EVT,"list_commands_status(): %s.commands_status[%u].addr_pair: %u", name, i, my_commands->commands_status[i].addr_pair);
        ESP_LOGD(TAG_EVT,"list_commands_status(): %s.commands_status[%u].uart_controller: %u", name, i, my_commands->commands_status[i].uart_controller);
        i++;
    }
    return;
}

void clean_cmds_list(commands_t *commands_list, char *name)
{
    ESP_LOGV(TAG_EVT,"clean_cmds_list(): BEFORE PROCESSING");
    list_commands_status(commands_list, name);

    //clean commands_status array from empty cells
    /////processing commands_status[]
    unsigned char i = 0;
    while (i < commands_list->num_cmd_under_processing)
    {
        ESP_LOGV(TAG_EVT,"clean_cmds_list(): processing cmds no. %u", i);
        if (commands_list->commands_status[i].addr_pair == 0xFF)
        { //this command has been previsously acknowledeged: so it has to be removed
            ESP_LOGI(TAG_EVT,"clean_cmds_list(): cmds no. %u is no longer of interest.....cleaning up", i);
            unsigned char j = i;
            while (j < commands_list->num_cmd_under_processing - 1)
            {
                strcpy2(commands_list->commands_status[j].cmd, commands_list->commands_status[j + 1].cmd);
                strcpy2(commands_list->commands_status[j].param, commands_list->commands_status[j + 1].param);
                commands_list->commands_status[j].rep_counts = commands_list->commands_status[j + 1].rep_counts;
                commands_list->commands_status[j].num_checks = commands_list->commands_status[j + 1].num_checks;
                commands_list->commands_status[j].addr_pair = commands_list->commands_status[j + 1].addr_pair;
                commands_list->commands_status[j].uart_controller = commands_list->commands_status[j + 1].uart_controller;
                j++;
            }
            commands_list->num_cmd_under_processing--;
            ESP_LOGV(TAG_EVT,"clean_cmds_list(): %s->num_cmd_under_processing: %u", name, commands_list->num_cmd_under_processing);
            memset(commands_list->commands_status[commands_list->num_cmd_under_processing].cmd, 0, sizeof(commands_list->commands_status[i].cmd));
            memset(commands_list->commands_status[commands_list->num_cmd_under_processing].param, 0, sizeof(commands_list->commands_status[i].param));
            commands_list->commands_status[commands_list->num_cmd_under_processing].rep_counts = 0;
            commands_list->commands_status[commands_list->num_cmd_under_processing].num_checks = 0;
            commands_list->commands_status[commands_list->num_cmd_under_processing].addr_pair = 0xFF;
            commands_list->commands_status[commands_list->num_cmd_under_processing].uart_controller = 0;

            i = -1; //after having shifted one ACKed command then restart scanning from the beginning
        }
        i++;
    }

    /////REPORT commands_status[] AFTER processing
    ESP_LOGV(TAG_EVT,"clean_cmds_list(): AFTER PROCESSING");
    list_commands_status(commands_list, name);
    return;
}

unsigned char invia_comando(uart_port_t uart_controller, commands_t *my_commands, unsigned char addr_from, unsigned char addr_pair, const unsigned char *cmd, const unsigned char *param, unsigned char rep_counts)
{
    unsigned char *msg;

    ESP_LOGD(TAG_EVT,"invia_comando():uart_controller: %u", uart_controller);
    ESP_LOGD(TAG_EVT,"invia_comando():addr_from: %u", addr_from);
    ESP_LOGD(TAG_EVT,"invia_comando():addr_pair: %u", addr_pair);
    ESP_LOGD(TAG_EVT,"invia_comando():cmd_tosend: %s", cmd);
    ESP_LOGD(TAG_EVT,"invia_comando():param_tosend: %s", param);
    ESP_LOGD(TAG_EVT,"invia_comando():rep_counts: %u", rep_counts);

    msg = pack_msg(addr_from, addr_pair, cmd, param, rep_counts);

    ESP_LOGI(TAG_EVT,"invia_comando(): msg: %s , msg_len: %u", msg, strlen2(msg));
    
    //printf("loop(): ***************end:%d",msg[strlen2(msg)-1]);
    msg[strlen2(msg)] = 'E';
    msg[strlen2(msg)] = 'N';
    msg[strlen2(msg)] = 'D';
    ESP_LOG_BUFFER_HEXDUMP(TAG_EVT, msg, strlen2(msg), ESP_LOG_INFO);
    //sending msg on UARTn (uart_controller)
    ESP_LOGD(TAG_EVT,"invia_comando(): msg: %s , msg_len: %u", msg, strlen2(msg));
    int numbytes = scriviUART(uart_controller, msg);

    if (numbytes < 0)
    {
        ESP_LOGE(TAG_EVT,"invia_comando(): Fatal error in sending data on UART%u", uart_controller);
        vTaskDelay(5000 / portTICK_RATE_MS);
        return -1;
    }
    else
    {
        ESP_LOGI(TAG_EVT,"invia_comando(): sent %d bytes on UART_NUM_%u", numbytes, uart_controller);
        vTaskDelay(500 / portTICK_RATE_MS);
        presentBlink(BLINK_GPIO, NUM_PRES_BLINKS);
    }

    //if this command is the first of the repetition and is not an ACK (so it is an actually new command)
    //I am inserting this new command in commands_status array to be tracked
    if ((!strncmp2(cmd, (const unsigned char *)"ACK", 3) == 0) && (rep_counts == 1))
    {
        if (my_commands->num_cmd_under_processing < NUM_MAX_CMDS)
        {
            ESP_LOGI(TAG_EVT,"invia_comando(): This is an actual command inserting in commands_status: ^%s^\n", cmd);
            strcpy2(my_commands->commands_status[my_commands->num_cmd_under_processing].cmd, cmd);
            strcpy2(my_commands->commands_status[my_commands->num_cmd_under_processing].param, param);
            my_commands->commands_status[my_commands->num_cmd_under_processing].rep_counts = rep_counts;
            my_commands->commands_status[my_commands->num_cmd_under_processing].num_checks = 0;
            my_commands->commands_status[my_commands->num_cmd_under_processing].addr_pair = addr_pair;
            my_commands->commands_status[my_commands->num_cmd_under_processing].uart_controller = uart_controller;
            my_commands->num_cmd_under_processing++;
            return 0;
        }
        else
        {
            ESP_LOGW(TAG_EVT,"--------------------I can not send the command because commands_list is full");
            return -1;
        }
    }
    return 0;
}

unsigned char invia_ack(uart_port_t uart_controller, commands_t *my_commands, unsigned char addr_from, evento_t *evento)
{
    unsigned char cmd_cmdtosend[FIELD_MAX];
    strcpy2(cmd_cmdtosend, (const unsigned char *)"ACK_");
    return (invia_comando(uart_controller, my_commands, addr_from, evento->valore_evento.pair_addr, (const unsigned char *)strcat2(cmd_cmdtosend, evento->valore_evento.cmd_received), evento->valore_evento.param_received, evento->valore_evento.ack_rep_counts));
}

unsigned char manage_issuedcmd_retries(uart_port_t uart_controller, evento_t *detected_event, commands_t *my_commands)
{
    unsigned char ret = 0;
    unsigned char i = 0;
    ESP_LOGV(TAG_EVT,"manage_issuedcmd_retries():BEGIN");
    ESP_LOGD(TAG_EVT,"manage_issuedcmd_retries():type of event = %u ", detected_event->type_of_event);
    if (!(detected_event->type_of_event == NOTHING))
    {
        ESP_LOGW(TAG_EVT, "manage_issuedcmd_retries():THIS SHOULD NOT HAPPEN");
    }
    //Managing retries of issued commands
    i = 0;
    while (i < my_commands->num_cmd_under_processing)
    {
        if (!(my_commands->commands_status[i].addr_pair == 0xFF))
        { //if this cmd has not been acknowledeged
            if (my_commands->commands_status[i].num_checks < NUM_MAX_CHECKS_FOR_ACK)
            { //if it has not yet reached max checks attempts
                my_commands->commands_status[i].num_checks++;
            }
            else if (my_commands->commands_status[i].rep_counts < NUM_MAX_RETRIES)
            { //if it has reached max attempts I send a new copy with a rep_count incremented
                my_commands->commands_status[i].num_checks = 0;
                my_commands->commands_status[i].rep_counts++;
                invia_comando(uart_controller, my_commands, LOCAL_STATION_ADDRESS, my_commands->commands_status[i].addr_pair, my_commands->commands_status[i].cmd,
                              my_commands->commands_status[i].param, my_commands->commands_status[i].rep_counts);
                break;
            }
            else
            {
                detected_event->type_of_event = FAIL_TO_SEND_CMD;
                detected_event->valore_evento.value_of_input = 0;
                //memcpy(detected_event->valore_evento.cmd_received,my_commands->commands_status[i].cmd,sizeof(my_commands->commands_status[i].cmd));
                //memcpy(detected_event->valore_evento.param_received,my_commands->commands_status[i].param,sizeof(my_commands->commands_status[i].param));
                detected_event->valore_evento.cmd_received = my_commands->commands_status[i].cmd;
                detected_event->valore_evento.param_received = my_commands->commands_status[i].param;
                detected_event->valore_evento.ack_rep_counts = my_commands->commands_status[i].rep_counts;
                detected_event->valore_evento.pair_addr = my_commands->commands_status[i].addr_pair;
                my_commands->commands_status[i].addr_pair = 0xFF;
                ret = 1;
            }
        }
        i++;
    }
    return ret;
}

unsigned char manage_rcvcmds_retries(commands_t *rcv_commands)
{
    //here detected_event is considered only for debugging purpose. Consider removing it.
    unsigned char ret = 0;
    unsigned char i = 0;
    ESP_LOGV(TAG_EVT,"manage_rcvcmds_retries():BEGIN");
    //Managing retries of issued commands
    i = 0;
    while (i < rcv_commands->num_cmd_under_processing)
    {
        if (!(rcv_commands->commands_status[i].addr_pair == 0xFF))
        { //if this cmd is still valid
            if (rcv_commands->commands_status[i].num_checks < NUM_CHECKS_FOR_ALREADY_RECEIVED)
            { //if it has not yet reached checks attempts
                rcv_commands->commands_status[i].num_checks++;
            }
            else
            { //I consider that after NUM_CHECKS_FOR_ALREADY_RECEIVED check I should not receive a retry of the same message, so I remove from the check-list
                rcv_commands->commands_status[i].addr_pair = 0xFF;
                ret = 1;
            }
        }
        i++;
    }
    return ret;
}

unsigned char check_rcved_acks(evento_t *detected_event, commands_t *my_commands)
{
    //printf("check_rcv_acks():BEGIN");
    unsigned char ret = 1;
    unsigned char i = 0;
    if (detected_event->type_of_event == RECEIVED_MSG)
    {
        ESP_LOGI(TAG_EVT,"check_rcv_acks():detected_event->type_of_event == (%d) RECEIVED_MSG",detected_event->type_of_event);
        ESP_LOGD(TAG_EVT,"check_rcv_acks():detected_event->valore_evento.cmd_received=%s", detected_event->valore_evento.cmd_received);
        ESP_LOGD(TAG_EVT,"check_rcv_acks():detected_event->valore_evento.param_received=%s", detected_event->valore_evento.param_received);
        ESP_LOGD(TAG_EVT,"check_rcv_acks():detected_event->valore_evento.ack_rep_counts=%u", detected_event->valore_evento.ack_rep_counts);
        ESP_LOGD(TAG_EVT,"check_rcv_acks():detected_event->valore_eventopair_addr=%u", detected_event->valore_evento.pair_addr);
        ESP_LOGD(TAG_EVT,"check_rcv_acks():uart_controller=%u", detected_event->valore_evento.uart_controller);
        //check if current event is an ACK
        i = 0;
        while (i < my_commands->num_cmd_under_processing)
        {
            //printf("check_rcv_acks():detected_event->valore_evento.cmd_received=%s", detected_event->valore_evento.cmd_received);
            ESP_LOGV(TAG_EVT,"check_rcv_acks():my_commands->commands_status[%u].cmd=%s", i, my_commands->commands_status[i].cmd);
            //printf("check_rcv_acks():detected_event->valore_evento.param_received=%s", detected_event->valore_evento.param_received);
            ESP_LOGV(TAG_EVT,"check_rcv_acks():my_commands->commands_status[%u].param=%s", i, my_commands->commands_status[i].param);
            //printf("check_rcv_acks():detected_event->valore_evento.ack_rep_counts=%u", detected_event->valore_evento.ack_rep_counts);
            ESP_LOGV(TAG_EVT,"check_rcv_acks():my_commands->commands_status[%u].rep_counts=%u", i, my_commands->commands_status[i].rep_counts);
            //printf("check_rcv_acks():detected_event->valore_eventopair_addr=%u", detected_event->valore_evento.pair_addr);
            ESP_LOGV(TAG_EVT,"check_rcv_acks():my_commands->commands_status[%u].addr_pair=%u", i, my_commands->commands_status[i].addr_pair);
            //printf("check_rcv_acks():uart_controller=%u", detected_event->valore_evento.uart_controller);
            ESP_LOGV(TAG_EVT,"check_rcv_acks():my_commands->commands_status[%u].uart_controller=%u", i, my_commands->commands_status[i].uart_controller);

            if ((strcmpACK(detected_event->valore_evento.cmd_received, my_commands->commands_status[i].cmd) == 0) &&
                (strcmp2(detected_event->valore_evento.param_received, my_commands->commands_status[i].param) == 0) &&
                (detected_event->valore_evento.ack_rep_counts == my_commands->commands_status[i].rep_counts) && //sicuro che convenga lasciarlo??
                (detected_event->valore_evento.pair_addr == my_commands->commands_status[i].addr_pair) &&
                (detected_event->valore_evento.uart_controller == my_commands->commands_status[i].uart_controller) //da togliere??
            )
            {
                ret = 0;                                      //ACK_RECEIVED
                detected_event->type_of_event = RECEIVED_ACK; //FORSE DA SPOSTARE FUORI
                my_commands->commands_status[i].addr_pair = 0xFF;
                ESP_LOGI(TAG_EVT,"check_rcv_acks(): CMD NUMBER %u HAS BEEN ACKNOWLEDGED!!!!!!!!\n", i);
                return ret; //with this return only one command is acknowleded with a received ACK, without this return all the same cmd+param+rep_count+addr_pair will be ackowledeged with one only ACK
            }
            i++;
        } //while (i<num_cmd_under_processing)
    }
    ESP_LOGD(TAG_EVT,"check_rcv_acks(): exit with code: %u", ret);
    return ret;
}

evento_t *detect_event(uart_port_t uart_controller, const gpio_num_t *mygpio_input_command_pin, commands_t *my_commands, commands_t *rcv_commands)
{
    //printf("detectEvent(): Calling function clean_acknowledged_cmds()");
    clean_cmds_list(my_commands, "my_commands");
    clean_cmds_list(rcv_commands, "rcv_commands");

    static evento_t detected_event;
    unsigned char IN[NUM_HANDLED_INPUTS];
    static unsigned char IN_PREC[NUM_HANDLED_INPUTS];

    for (int i = 0; i < sizeof(IN); i++)
    {
        IN[i] = (unsigned char)gpio_get_level(mygpio_input_command_pin[i]);
        //printf("IN[%u] =%u; gpio_input_command_pin[%u]=%u",i,IN[i],i,(unsigned char)mygpio_input_command_pin[i]);
    }
    ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)BLINK_GPIO, (uint32_t)IN[0])); //echoes PIN1 on OK_GPIO led
    //ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)NOK_GPIO, (uint32_t) (1-IN[0]))); //echoes PIN1 on OK_GPIO led

    //Initialize event variable
    detected_event.type_of_event = NOTHING;
    detected_event.valore_evento.input_number = 0;
    detected_event.valore_evento.value_of_input = 0;
    detected_event.valore_evento.cmd_received = NULL;
    detected_event.valore_evento.param_received = NULL;
    detected_event.valore_evento.ack_rep_counts = 0;
    detected_event.valore_evento.pair_addr = 0;
    detected_event.valore_evento.uart_controller = (uart_port_t)0;

    //Detect if IO_INPUT went low
    for (int i = 0; i < sizeof(IN); i++)
    {
        //printf("i=%u, sizeof(IN)=%u",i,sizeof(IN));
        if (!(IN[i] == IN_PREC[i]))
        { //if IN[i] went low then an event occurred
            detected_event.type_of_event = IO_INPUT_ACTIVE;
            ESP_LOGV(TAG_EVT,"detectEvent(): IN[%u]: %u ; IN_PREC[%u]: %u", i, IN[i], i, IN_PREC[i]);
            ESP_LOGD(TAG_EVT,"detectEvent(): !!!!!!! IO Input Event Occurred !!!!!!!");
            detected_event.valore_evento.input_number = i;
            detected_event.valore_evento.value_of_input = IN[i];
            detected_event.valore_evento.cmd_received = NULL;
            detected_event.valore_evento.param_received = NULL;
            detected_event.valore_evento.ack_rep_counts = 0;
            detected_event.valore_evento.pair_addr = 0; //myself
            detected_event.valore_evento.uart_controller = uart_controller;
            IN_PREC[i] = IN[i];
            return &detected_event; //with this return just one input at a time is detected and handled
        }
        IN_PREC[i] = IN[i];
    }

    //Detect if a message has been received from any station reading data from the UART controller
    unsigned char *line1;
    line1 = read_line(uart_controller);
    ESP_LOGD(TAG_EVT,"detect_event(): line read from UART%u: ", uart_controller);
    stampaStringa((char *)line1);

    unsigned char station_iter = 0;
    unsigned char allowed_addr_pair = LOCAL_STATION_ADDRESS;

    while (station_iter < 2)
    {
        ESP_LOGV(TAG_EVT,"detect_event(): Parsing line read -  iter: %u! ", station_iter);
        unsigned char allowed_addr_from;
        if (station_iter == 0)
        { //check if someone sent me a message
            //First I try with one of the other 2 actors
            if (STATION_ROLE == STATIONMASTER)
            {
                allowed_addr_from = ADDR_SLAVE_STATION;
            }
            if (STATION_ROLE == STATIONSLAVE)
            {
                allowed_addr_from = ADDR_MASTER_STATION;
            }
            if (STATION_ROLE == STATIONMOBILE)
            {
                allowed_addr_from = ADDR_MASTER_STATION;
            }
        }
        else
        {
            //Then I try with the other of the other 2 actors
            if (STATION_ROLE == STATIONMASTER)
            {
                allowed_addr_from = ADDR_MOBILE_STATION;
            }
            if (STATION_ROLE == STATIONSLAVE)
            {
                allowed_addr_from = ADDR_MOBILE_STATION;
            }
            if (STATION_ROLE == STATIONMOBILE)
            {
                allowed_addr_from = ADDR_SLAVE_STATION;
            }
        }

        unsigned char *cmd_received;
        unsigned char *param_received;
        unsigned char ack_rep_counts = 0;

        unsigned char ret = unpack_msg(line1, allowed_addr_from, allowed_addr_pair, &cmd_received, &param_received, &ack_rep_counts);
        ESP_LOGV(TAG_EVT,"\ndetect_event(): unpack returned with code: %d!", ret);
        ESP_LOGV(TAG_EVT,"detect_event(): cmd_received: %s", cmd_received);
        ESP_LOGV(TAG_EVT,"detect_event(): param_received: %s", param_received);
        ESP_LOGV(TAG_EVT,"detect_event(): ack_rep_counts: %d", ack_rep_counts);
        ESP_LOGV(TAG_EVT,"detect_event(): uart_controller: %d", uart_controller);

        if (ret == 0)
        {
            detected_event.type_of_event = RECEIVED_MSG;
            ESP_LOGI(TAG_EVT,"detectEvent(): !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!A message of interest has been detected!!!!!!!");
            detected_event.valore_evento.input_number = 0;
            detected_event.valore_evento.value_of_input = 0;
            detected_event.valore_evento.cmd_received = cmd_received;
            detected_event.valore_evento.param_received = param_received;
            detected_event.valore_evento.ack_rep_counts = ack_rep_counts;
            detected_event.valore_evento.pair_addr = allowed_addr_from; //the actual pair_addr
            detected_event.valore_evento.uart_controller = uart_controller;
            station_iter = 1; //return &detected_event;
        }
        else
        {
            ESP_LOGI(TAG_EVT,"detectEvent(): message was not of interest. RetCode=%u", ret);
        }
        station_iter++;
    }
    //before return check if:
    //- I have not received anyhting (e.g. neither a message nor an input command) OR
    //- the received MSG is NOT an ACK
    //then I have to manage the retries looking for a FAIL_TO_SEND_CMD event
    check_rcved_acks(&detected_event, my_commands);
    if (detected_event.type_of_event == NOTHING)
    {
        //if I call this then I have not yet found an actual event (neither an IO input_cmd nor a message nor an ACK)
        //so I am incrementing checks number
        manage_issuedcmd_retries(uart_controller, &detected_event, my_commands);
        manage_rcvcmds_retries(rcv_commands);
    }

    return &detected_event;
}