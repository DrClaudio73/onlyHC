#include "funzioni_basic.h"
#include <stdio.h> 
#include <string.h>

unsigned char calcola_CRC(int totale) {
    return((unsigned char) totale%256);
}

void pack_str(unsigned char* msg_to_send, const unsigned char* valore, unsigned char* k, int* totale){
    printf("----START pack str----\r\n");
    for (int i=0; i<strlen(valore); i++){
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
    pack_num(msg_to_send,strlen(cmd),&k,&totale);
    pack_num(msg_to_send,strlen(param),&k,&totale);
    pack_str(msg_to_send,cmd,&k,&totale);
    pack_str(msg_to_send,param,&k,&totale);
    msg_to_send[k++]=calcola_CRC(totale);
    msg_to_send[k]=(unsigned char) 0x7f;

    if (DEBUG) {
        for(int i=0 ; i<strlen(msg_to_send); i++){
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

    for (unsigned char i=0; i<strlen(msg);i++) {
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

int main(void){
    printf("Ciao!!!!!\r\n");
    unsigned char  addr_from=ADDR_MASTER_STATION;
    unsigned char  addr_to=ADDR_SLAVE_STATION;
    unsigned char* cmd_tosend="abdcefgh";
    unsigned char* param_tosend="ijklmnop0";
    unsigned char* msg;
    unsigned char* cmd_received;
    unsigned char* param_received;
    
    printf("cmd: %s\r\n",cmd_tosend);
    printf("param: %s\r\n",param_tosend);
    
    msg=pack_msg(1, addr_from, addr_to, cmd_tosend, param_tosend);
    //msg[strlen(msg)-1]=33;
    printf("***************end:%d\r\n",msg[strlen(msg)-1]);
    msg[strlen(msg)]='E';
    msg[strlen(msg)]='N';
    msg[strlen(msg)]='D';
    for (int i=0; i<strlen(msg);i++){
        printf("msg[%d]= %d - %c\r\n", i, msg[i], msg[i]);
    }
    

    unsigned char allowed_addr_from=ADDR_MASTER_STATION;
    unsigned char allowed_addr_to=ADDR_SLAVE_STATION; //in realta nell'utilizzo reale saranno da swappare ma per ora in fase di sviluppo codice mi viene comodo così (infatti saranno generati da una pack_msg che gira sulla slave station)

    unsigned char ret=unpack_msg(msg, allowed_addr_from, allowed_addr_to, &cmd_received, &param_received);
    printf("unpack returned with code: %d!\r\n",ret);
    printf("cmd_received: %s\r\n",cmd_received);
    printf("param_received: %s\r\n",param_received);
    
    return 0;
}