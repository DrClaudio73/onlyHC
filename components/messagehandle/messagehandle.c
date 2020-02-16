#include <stdio.h>
#include "string.h"
#include "messagehandle.h"
#include "typeconv.h"

////////////////////////////////////////////// MESSAGE HANDLING FUNCTIONS //////////////////////////////////////////////
int strcmpACK(const unsigned char* rcv, const unsigned char* cmd){ //the rcv should match "ACK_"+cmd to be a valid ACK
    unsigned char expected_ACK[FIELD_MAX];
    strcpy2(expected_ACK,(const unsigned char*) "ACK_");
    strcat2(expected_ACK,cmd);
    return (strcmp2(rcv, expected_ACK));
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

    memset(msg_to_send,0,255);

    //Frame structure
    //CMD: 'B'+'G'+F7+ADDR_FROM(1 byte)+ADDR_TO(1 byte)+REP_COUNTS+CMD_LEN(1)+PARAM_LEN(1)+CMD(var byte)+PARAM(var byte)+CRC(1 byte)+7F
    //ANSWER: 'B'+'G'+F7+ADDR_FROM(1 byte)+ADDR_TO(1 byte)+ACKNOWLEDGED_REP_COUNTS+CMD_LEN(1)+PARAM_LEN(1)+CMD(var byte)+PARAM(var byte)+CRC(1 byte)+7F
    //CRC is evaluated starting at ADDDR_FROM up to PARAM
    
    //SUPPORTED CMDs:
    //---APRI+PARAM=XXXX
    //---REPORT+PARAM=SPECIFIC to be implemented 
    //---REPORT+PARAM=ALL      to be implemented
    //SUPPORTED REPLIES:
    //---ACK_APRI+RCV_PARAM+RCV_REP_COUNTS
    //---ACK_REPORT+PARAM=SPECIFIC  to be implemented
    //---ACK_REPORT+PARAM=ALL   to be implemented, in this case an handshake to trasnfer N items in transaction should be implemented

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

    #ifdef DEBUG
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
    #endif

    return msg_to_send;

}

unsigned char unpack_msg(const unsigned char* msg, unsigned char allowed_addr_from, unsigned char allowed_addr_to, unsigned char** cmd, unsigned char** param, unsigned char* acknowldged_rep_counts){
    // RETCODES
    // 0 = OK
    // 1 = INVALID CRC
    // 2 = MESSAGE RECEIVED FROM NOT allowed_addr_from
    // 3 = MESSAGE ADDRESSED to OTHER STATION
    // 4 = MESSAGE NOT PROPERLY TERMINATED
    // 5 = MESSAGE SHORTER THAN EXPECTED

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

    printf("unpack_msg(): processing msg= %s\r\n", msg);

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