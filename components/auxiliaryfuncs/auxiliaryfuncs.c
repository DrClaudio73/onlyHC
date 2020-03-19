#include "auxiliaryfuncs.h"
#include "driver/gpio.h"
// UART driver
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

////////////////////////////////////////////// SETUP FUNCTIONS //////////////////////////////////////////////
void setupmyRadioHC12(void)
{
    gpio_pad_select_gpio((gpio_num_t)HC12SETGPIO);
    /* Set the GPIO as a push/pull output  */
    gpio_set_direction((gpio_num_t)HC12SETGPIO, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)HC12SETGPIO, 1); //enter transparent mode

    uart_config_t uart_configHC12 = {
        .baud_rate = BAUDETRANSPARENTMODE,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .rx_flow_ctrl_thresh = 122,
        .use_ref_tick = false};
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
////////////////////////////////////////////// AUXILIARY FUNCTIONS //////////////////////////////////////////////
void foreverRed(unsigned char blink_gpio)
{
    int k = 0;
    for (;;)
    { // blink red LED forever
        gpio_set_level((gpio_num_t)blink_gpio, 1);
        vTaskDelay(100 / portTICK_RATE_MS);
        gpio_set_level((gpio_num_t)blink_gpio, 0);
        vTaskDelay(100 / portTICK_RATE_MS);
        k++;
        k = k % 1000;
        printf("%d: forever Red!!!!\r\n", k);
    }
}

void presentBlink(unsigned char LedPinNo, unsigned char num_pres_blinks)
{
    for (unsigned char k = 0; k < num_pres_blinks; k++)
    {
        gpio_set_level((gpio_num_t)LedPinNo, 1);
        vTaskDelay(50 / portTICK_RATE_MS);
        gpio_set_level((gpio_num_t)LedPinNo, 0);
        vTaskDelay(50 / portTICK_RATE_MS);
    }
}
