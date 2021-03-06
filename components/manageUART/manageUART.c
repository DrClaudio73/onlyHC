#include "manageUART.h"
#include "esp_log.h"
#include "typeconv.h"

static const char *TAG_M="manageUART:";

int scriviUART(uart_port_t uart_controller, unsigned char *text)
{
    int ret = uart_write_bytes(uart_controller, (char *)text, strlen((char *)text));
    //printf("%s ---- %d",text,strlen(text));
    return ret;
}

// read a line from the UART controller
unsigned char *read_line(uart_port_t uart_controller)
{
    static unsigned char line[LINE_MAX];
    memset(line,0,sizeof(line));
    unsigned char *ptr = line;
    //printf("\nread_line on UART: %d\n", (int) uart_controller);
    while (1)
    {
        int num_read = uart_read_bytes(uart_controller, (unsigned char *)ptr, 1, 45 / portTICK_RATE_MS); //portMAX_DELAY);
        if (num_read == 1)
        {
            //printf("received char: %c", *ptr);
            if (*ptr == 0x7f)
            { // new line found, terminate the string and return
                ptr++;
                *ptr = '\0';
                return line;
            }
            ptr++; // else move to the next char
        }
        else
        { //if no characters are retrived from UART return "\n"
            //printf("num_read=%d",num_read);
            *ptr = 10;
            ptr++;
            *ptr = 0;
            return line;
        }
    }
}

void stampaStringa(char *line)
{
    if (strlen(line) > 1)
    {
        ESP_LOGI(TAG_M,"stampaStringa: %s", line);
        ESP_LOG_BUFFER_HEXDUMP(TAG_M, line, strlen2((const unsigned char *)line), ESP_LOG_INFO);
    }
}