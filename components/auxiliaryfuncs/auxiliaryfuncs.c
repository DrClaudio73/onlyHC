#include "auxiliaryfuncs.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

////////////////////////////////////////////// AUXILIARY FUNCTIONS //////////////////////////////////////////////
void foreverRed(unsigned char blink_gpio) {
    int k=0;
    for (;;) { // blink red LED forever
        gpio_set_level((gpio_num_t)blink_gpio, 1);
        vTaskDelay(100 / portTICK_RATE_MS);
        gpio_set_level((gpio_num_t)blink_gpio, 0);
        vTaskDelay(100 / portTICK_RATE_MS);
        k++;
        k=k % 1000;
        printf("%d: forever Red!!!!\r\n",k);
    }
}

void presentBlink(unsigned char LedPinNo, unsigned char num_pres_blinks){
    for (unsigned char k=0; k<num_pres_blinks; k++){
        gpio_set_level((gpio_num_t)LedPinNo, 1);
        vTaskDelay(150 / portTICK_RATE_MS);
        gpio_set_level((gpio_num_t) LedPinNo, 0);
        vTaskDelay(150 / portTICK_RATE_MS);
    }   
}

