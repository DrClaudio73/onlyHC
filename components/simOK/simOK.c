#include "simOK.h"
#include "manageUART.h"

int checkDTMF(char* line) {
    //char *line1;
    char* token;

    token = strtok(line, "+DTMF: ");
    return(atoi(token));

    /*for(int k=0;k<10;k++){
        line1 = read_line(UART_NUM_1);
        if (strncmp("+DTMF: ",line1,strlen("+DTMF: "))==0){
            token = strtok(line1, "+DTMF: ");
            //sprintf(parametro_feedback,"%s",token);
            return(atoi(token));
        }
    }
    return -1; //only different feedback*/
}

int checkPhoneActivityStatus(parsed_params* parametri_globali, allowed_IDs chiamantiAbilitati) { // CHECK STATUS FOR RINGING or IN CALL
    char msg[8]="AT+CPAS";
    char condition[8]="+CPAS: ";
    // phone activity status: 0= ready, 2= unknown, 3= ringing, 4= in call
    return(verificaComando(UART_NUM_1,msg,condition,parametri_globali,chiamantiAbilitati));
}

char* parse_line(char* line, parsed_params* parametri_globali, allowed_IDs chiamantiAbilitati) {
    static char subline[2048];
    char* token;
    char* token2;

    sprintf(parametri_globali->generic_parametro_feedback,"%s"," "); //resetting generic_parametro_feedback 
    
    if (strncmp("SMS Ready\r\n",line,11)==0){
        sprintf(subline,"%s","\nSMS Ready Received!!!\n");
        printf(line);
        printf("\n");
        return subline;
    }

    if (strncmp("+CSQ: ",line,6)==0){
        //printf("\r\nSignal Quality Report!!!\r\n");
        token = strtok(line, "+CSQ: ");
        token2 = strtok(token, ",");
        parametri_globali->CSQ_qualityLevel= atoi(token2);
        sprintf(parametri_globali->generic_parametro_feedback,"%s",token2);
        sprintf(subline,"%s%s","Signal Quality Report!!! Level=",parametri_globali->generic_parametro_feedback);
        printf(line);
        printf("\n");
        return subline;
    }

    if (strncmp("+CPAS: ",line,strlen("+CPAS: "))==0){
        //printf("before: %d",parametri_globali->phoneActivityStatus);
        token = strtok(line, "+CPAS: ");
        //printf("token: %s",token);
        parametri_globali->phoneActivityStatus= atoi(token);
        sprintf(parametri_globali->generic_parametro_feedback,"%s",token);
        sprintf(subline,"%s%s","Phone Activity Status!!! Level=",parametri_globali->generic_parametro_feedback);
        printf(line);

        //printf("atoi: %d",parametri_globali->phoneActivityStatus);
        printf("\n");
        return subline;
    }

    if (strncmp("+DTMF: ",line,strlen("+DTMF: "))==0){
        parametri_globali->pending_DTMF=checkDTMF(line);
        printf(line);
        printf("\n");
        sprintf(subline,"%s",line);
        return subline;
    }

    if (strncmp("+CLIP: ",line,strlen("+CLIP: "))==0){
        printf(line);
        printf("\n");
        token = strtok(line, "CLIP: ");
        token = strtok(NULL, "CLIP: ");
        //printf("token: %s",token);
        token2 = strtok(token, "\"");
        sprintf(parametri_globali->calling_number,"%s",token2);
        //printf("token2: %s",token2);
        if (strncmp(chiamantiAbilitati.allowed1,token2,strlen(chiamantiAbilitati.allowed1))==0){
            parametri_globali->calling_number_valid=ALLOWED_CALLER; //OK: Valid Caller
        } else if (strncmp(chiamantiAbilitati.allowed2,token2,strlen(chiamantiAbilitati.allowed2))==0){
            parametri_globali->calling_number_valid=ALLOWED_CALLER; //OK: Valid Caller
        }
        else {
            parametri_globali->calling_number_valid=NOT_ALLOWED_CALLER; //Not valid Caller
        }
        sprintf(subline,"%s",line);
        return subline;
    }

    sprintf(subline,"%s",line);
    return subline;
}

int verificaComando(uart_port_t uart_controller, char* command, char* condition, parsed_params* parametri_globali, allowed_IDs chiamantiAbilitati){
    //bool skip=false;
    bool conditionMet=false;
    char *line;
    int k=0,deltaAttempts=0;
    //if (!skip){
    scriviUART(UART_NUM_1, command);
    scriviUART(UART_NUM_1, "\r"); //?? Serve davvero??
    printf("\nSending command: -- %s\n", command);
    //}

    //printf("condizione: %s",condition);
    //printf("\n");
    if (strncmp("SMS Ready",condition,strlen("SMS Ready"))==0){
        deltaAttempts=40; //If condition looked for is SMS Ready we have to flush more lines, since at start up SMS Ready comes only after many lines. In addtion we should also manage boot timeouts
        //printf("deltaattempts=20");
    } else {
        deltaAttempts=0;
        //printf("deltaattempts=0");
    }

    while ((!conditionMet)&&(k<MAX_ATTEMPTS+deltaAttempts)) {
        k++;
        //skip=false;
        vTaskDelay(450 / portTICK_RATE_MS); //wait a while before processing reply feedback after having issued the command
        line = read_line(UART_NUM_1); 
        char *subline=parse_line(line,parametri_globali,chiamantiAbilitati); //process feedback and popluate "parametri_globali"
        stampaStringa(subline); //for debug puropose
        /*printf("*******echo\n");
        for (int i=0; i<strlen(line);i++) {
            printf("%c",line[i]);
        }
        printf("\nend_echo*******\n");*/
        if (strncmp(condition,line,strlen(condition))==0){
            //printf("\r\nCondition OK !!!\r\n");
            conditionMet=true;
            int svuotaUART=NUM_LINES_TO_FLUSH;
            while (svuotaUART-->=0){
                line = read_line(UART_NUM_1);
                char *subline=parse_line(line,parametri_globali,chiamantiAbilitati); //again process feedback to popluate parametri_globali
                stampaStringa(subline);
            }
        }
        /*
        if (strncmp("\r\n",line,2)==0){
            skip=true;
            //printf("skipping cr+lf........\n");
        }
        if (strncmp("\n",line,1)==0){
            skip=true;
            //printf("skipping lf........\n");
        }
        if (strncmp(text,line,strlen(text))==0){
            skip=true;
            //printf("skipping echo........\n");
        }*/
    }
    
    if (k==MAX_ATTEMPTS+deltaAttempts){ //if exited from while without having met condition looked for then an error occured 
        ESP_LOGE(TAG,"\r\nmax_ATTEMPTS %d\r\n",k);
        return(-1); //ERROR
    } else {
        return 0; //OK
    }
}

void reset_module(uart_port_t uart_controller, parsed_params* parametri_globali, allowed_IDs chiamantiAbilitati){
    ESP_LOGW(TAG,"Resetting module via CFUN command....\n");
    if (verificaComando(UART_NUM_1,"AT+CFUN=1,1","OK\r\n",parametri_globali,chiamantiAbilitati)==0){ //AT+CFUN=1,1
        ESP_LOGI(TAG,"\nSIM 808 got RESET command !!!\n");
        ESP_LOGW(TAG,"Waiting 7.5 s , for SIM800 booting....\n");
        vTaskDelay(7500 / portTICK_RATE_MS);
        if (verificaComando(UART_NUM_1,"AT","SMS Ready\r\n",parametri_globali,chiamantiAbilitati)==0){ //AT+CFUN=1,1
            ESP_LOGI(TAG,"\nSIM 808 got RESET command !!!\n");
        } else {
            ESP_LOGW(TAG,"Software Reset was not successfull, trying with hardware reset....\n");
            gpio_set_level((gpio_num_t)RESET_SIM800GPIO, RESET_ACTIVE);
            vTaskDelay(399 / portTICK_RATE_MS);
            gpio_set_level((gpio_num_t)RESET_SIM800GPIO, RESET_IDLE);
            ESP_LOGW(TAG,"Waiting 7.5 s , for SIM800 booting....\n");
            vTaskDelay(7500 / portTICK_RATE_MS);
            verificaComando(UART_NUM_1,"AT","SMS Ready\r\n",parametri_globali,chiamantiAbilitati);
        }
    } else {
        ESP_LOGW(TAG,"Software Reset was not successfull, trying with hardware reset....\n");
        gpio_set_level((gpio_num_t)RESET_SIM800GPIO, RESET_ACTIVE);
        vTaskDelay(399 / portTICK_RATE_MS);
        gpio_set_level((gpio_num_t)RESET_SIM800GPIO, RESET_IDLE);
        ESP_LOGW(TAG,"Waiting 7.5 s , for SIM800 booting....\n");
        vTaskDelay(7500 / portTICK_RATE_MS);
        verificaComando(UART_NUM_1,"AT","SMS Ready\r\n",parametri_globali,chiamantiAbilitati);
        //ESP_LOGW(TAG,"After hardware reset, applying another software reset....\n");
        //if (verificaComando(UART_NUM_1,"AT+CFUN=1,1","SMS Ready\r\n",parametri_globali)==0){
        //    ESP_LOGI(TAG,"\nSIM 808 got RESET command !!!\n");
    } 
}

/*
enum typeOfCaller checkCallingNumber(parsed_params* parametri_globali){
    char *line1;
    char* token;
    char* token2;
    enum typeOfCaller is_calling_number_valid;
    is_calling_number_valid=NO_CALLER; //-1=Number Calling not allowed ; 0=Number Calling allowed; ; +1=No calling number
    ESP_LOGI(TAG,"checkCallingNumber()\n");
    for(int k=0;k<10;k++){
        line1 = read_line(UART_NUM_1);
        printf("line read is:%s",line1);
        //+CLIP: "+3912345678",145,"",0,"ccccccc",0
        if (strncmp("+CLIP: ",line1,strlen("+CLIP: "))==0){
            token = strtok(line1, "CLIP: ");
            token = strtok(NULL, "CLIP: ");
            //printf("token: %s",token);
            token2 = strtok(token, "\"");
            sprintf(parametri_globali->parametro_feedback,"%s",token2);
            //printf("token2: %s",token2);
            if (strncmp(parametri_globali->allowed1,token2,strlen(parametri_globali->allowed1))==0){
                is_calling_number_valid=ALLOWED_CALLER;
            } else if (strncmp(parametri_globali->allowed2,token2,strlen(parametri_globali->allowed2))==0) {
                is_calling_number_valid=ALLOWED_CALLER;
            } else {
                is_calling_number_valid=NOT_ALLOWED_CALLER;
            }
        }
    }

    return(is_calling_number_valid);
    // USELESS
    if (is_calling_number_valid==ALLOWED_CALLER){
        return(is_calling_number_valid); // OK numbero valid
    }
    
    if (parametri_globali.calling_number_valid==ALLOWED_CALLER){
        sprintf(parametri_globali.parametro_feedback,"%s",parametri_globali.calling_number);
        parametri_globali.calling_number_valid=-1;
        parametri_globali.calling_number[0]=0;
        return(0); // OK number valid
    }

    return is_calling_number_valid; //Only different feedback
    //ENDOFUSELESS
}*/
int simOK(uart_port_t uart_controller, parsed_params* parametri_globali, allowed_IDs chiamantiAbilitati) { // SIM CHECK OK
    ESP_LOGI(TAG,"\n***************************************************************\nEntered in simOK()...\n***************************************************************");
    ESP_LOGI(TAG,"\n***************************************************************\nGeneral Settings and Checking for sim800 module and SIM card...\n***************************************************************\n");
    vTaskDelay(2500 / portTICK_RATE_MS);
    if (verificaComando(UART_NUM_1,"AT","OK\r\n", parametri_globali, chiamantiAbilitati)==0){
        ESP_LOGI(TAG,"\nSIM800 module found no need to reset!!!");
    } else {
        reset_module(uart_controller, parametri_globali, chiamantiAbilitati);
    }

    // time to startup 3 sec
    for (int i = 0; i < 6; i++) {
    //presentation blinking
        gpio_set_level((gpio_num_t)BLINK_GPIO, 1);
        vTaskDelay(50 / portTICK_RATE_MS);
        gpio_set_level((gpio_num_t)BLINK_GPIO, 0);
        vTaskDelay(50 /portTICK_RATE_MS);
    }

    ESP_LOGW(TAG,"\n********Setting fixed baud rate....");
    if (verificaComando(UART_NUM_1,"AT+IPR?","+IPR", parametri_globali,chiamantiAbilitati)==0){
        ESP_LOGI(TAG,"\ngot baud rate !!!");
        } else {
            ESP_LOGE(TAG,"\n********Error in checking baud rate !!!");
    }/*
    if (verificaComando(UART_NUM_1,"AT+IPR=115200;&W","OK\r\n", parametri_globali,chiamantiAbilitati)==0){
        ESP_LOGI(TAG,"\nbaud rate has been set !!!");
        } else {
            ESP_LOGE(TAG,"\n********Error in setting baud rate !!!");
            AT+IPR=9600;&W
    }*/

    ESP_LOGW(TAG,"\n********Checking if sim800 responds....");
    if (verificaComando(UART_NUM_1,"AT","OK\r\n", parametri_globali,chiamantiAbilitati)==0){
        ESP_LOGI(TAG,"\nSIM800 module found !!!");

        vTaskDelay(1000 / portTICK_RATE_MS); // wait for sim800 to settle a bit

        ESP_LOGW(TAG,"\n********Disabling Echo....");
        if (verificaComando(UART_NUM_1,"ATE 0","OK\r\n", parametri_globali,chiamantiAbilitati)==0){
            ESP_LOGI(TAG,"\n********Echo disabled !!!");
        } else {
            ESP_LOGE(TAG,"\n********Error in disabling echo !!!");
        }

        ESP_LOGW(TAG,"\n********Checking if SIM card inserted...");
        if (verificaComando(UART_NUM_1,"AT+CSMINS?","+CSMINS: 0,1\r\n", parametri_globali,chiamantiAbilitati)==0){
            ESP_LOGI(TAG,"\n********SIM card is inserted!!!");
        } else {
            ESP_LOGE(TAG,"\n********No SIM card found, stop here!\n********"); // continue if SIM card found
            return -1; //ERROR
        }

        //ESP_LOGW(TAG,"\n********Allow some time for SIM to register on the network...\n********");

        ESP_LOGW(TAG,"\n********Checking battery level....");
        if (verificaComando(UART_NUM_1,"AT+CBC","OK\r\n", parametri_globali,chiamantiAbilitati)==0){
            ESP_LOGI(TAG,"\n********Answer from module relevant to battery level !!!");
        } else {
            ESP_LOGE(TAG,"\n********Error in answer from module relevant to battery level !!!");
        }

        ESP_LOGW(TAG,"\n********Setting speaker volume to 80 [0-100]....");
        if (verificaComando(UART_NUM_1,"AT+CLVL=80","OK\r\n", parametri_globali,chiamantiAbilitati)==0){
            ESP_LOGI(TAG,"\n********Speaker Volume set !!!");
        } else {
            ESP_LOGE(TAG,"\n********Error in setting speaker volume !!!");
        }

        ESP_LOGW(TAG,"\n********Setting ringer volume to 80 [0-100]....");
        if (verificaComando(UART_NUM_1,"AT+CRSL=80","OK\r\n", parametri_globali,chiamantiAbilitati)==0){
            ESP_LOGI(TAG,"\n********Ringer Volume set !!!");
        } else {
            ESP_LOGE(TAG,"\n********Error in setting ringer volume !!!");
        }

        ESP_LOGW(TAG,"\n********Setting mic to gain level 10 [0-15]....");
        if (verificaComando(UART_NUM_1,"AT+CMIC=0,10","OK\r\n", parametri_globali,chiamantiAbilitati)==0){
            ESP_LOGI(TAG,"\n********Mic  gain level set !!!");
        } else {
            ESP_LOGE(TAG,"\n********Error in setting mic gain level !!!");
        }

        // ring tone AT+CALS=5,1 to switch on tone 5 5,0 to switch off
        //sim800.println(“AT+CALS=19”); // set alert/ring tone
        //simReply();
        
        //printf("\nDisabling automatic answer call after 2 ring(s)....\n");
        ESP_LOGW(TAG,"\n********Disabling automatic answer call after some ring(s)....");
        if (verificaComando(UART_NUM_1,"ATS0=0","OK\r\n", parametri_globali,chiamantiAbilitati)==0){
            ESP_LOGI(TAG,"\n********Module will not automaticallly answer !!!");
        } else {
            ESP_LOGE(TAG,"\n********Error in disabling auto answer mode !!!");
        }

        ESP_LOGW(TAG,"\n********Enabling DTMF recognition....");
        if (verificaComando(UART_NUM_1,"AT+DDET=1,1000,0,0","OK\r\n", parametri_globali,chiamantiAbilitati)==0){
            ESP_LOGI(TAG,"\n********DTMF recognition is on !!!");
        } else {
            ESP_LOGE(TAG,"\n********Error in setting DDET mode !!!");
        }

        ESP_LOGW(TAG,"\n********Setting SMS text mode....");
        if (verificaComando(UART_NUM_1,"AT+CMGF=1","OK\r\n", parametri_globali,chiamantiAbilitati)==0){
            ESP_LOGI(TAG,"\n********SMS text Mode enabled !!!");
        } else {
            ESP_LOGE(TAG,"\n********Not possible to enable SMS text Mode !!!");            
        }

        /*printf("\nEnabling DTMF recognition....\n");
        if (verificaComando(UART_NUM_1,"AT+DDET=1","OK\r\n")==0){
            printf("\nDTMF recognition is on !!!\n");
        }*/

        ESP_LOGW(TAG,"\n********Checking Signal Quality....");
        if (verificaComando(UART_NUM_1,"AT+CSQ","+CSQ", parametri_globali,chiamantiAbilitati)==0){
            ESP_LOGI(TAG,"\n********Quality report gotten !!! Quality level=%d\n",parametri_globali->CSQ_qualityLevel);
        } else {
            ESP_LOGE(TAG,"\n********No valid quality report gotten !!!\n");
        }
        
        /*ESP_LOGW(TAG,"\n********Checking Caller Line Number Presentation....");
        if (verificaComando(UART_NUM_1,"AT+CLIP=?","OK\r\n", parametri_globali)==0){
            ESP_LOGI(TAG,"\n********Quality report gotten !!! Quality level=%d\n",parametri_globali->quality);
        } else {
            ESP_LOGE(TAG,"\n********No valid quality report gotten !!!\n");
        }*/

        ESP_LOGW(TAG,"\n********Enabling Caller Line Number Presentation....");
        if (verificaComando(UART_NUM_1,"AT+CLIP=1","OK\r\n", parametri_globali,chiamantiAbilitati)==0){
            ESP_LOGI(TAG,"\n********Enabled line number presentatio to TE !!! \n");
        } else {
            ESP_LOGE(TAG,"\n********Not able to enable line number presentatio to TE!!!\n");
        }

        return 0; //OK
    } else {
        ESP_LOGE(TAG,"\n********NO ANSWER FROM MODULE: Stuck here!!!!!\n********");
        return -1; //ERROR
    }
}