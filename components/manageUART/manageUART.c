#include "manageUART.h"

void scriviUART(uart_port_t uart_controller, char* text){
    uart_write_bytes(uart_controller, text, strlen(text));    
    //printf("%s ---- %d",text,strlen(text));
    return;
}

// read a line from the UART controller
char* read_line(uart_port_t uart_controller) {
    static char line[2048];
    char *ptr = line;
    //printf("\nread_line on UART: %d\n", (int) uart_controller);
    while(1) {
        int num_read = uart_read_bytes(uart_controller, (unsigned char *)ptr, 1, 45/portTICK_RATE_MS); //portMAX_DELAY); 
        if(num_read == 1) {
            //printf("received char: %c", *ptr);
            if(*ptr == '\n') { // new line found, terminate the string and return 
                ptr++;
                *ptr = '\0';
                return line;
            }
            ptr++; // else move to the next char
        } else {  //if no characters are retrived from UART return "\n"
            //printf("num_read=%d",num_read);
            *ptr=10;
            ptr++;
            *ptr=0;
            return line;
        } 
    }
}

void stampaStringa(char* line){
        //printf("Rcv%d: %s", numline,line);
        if(strlen(line)>1){
            printf("%s",line);
        }
}