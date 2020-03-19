#include "sdkconfig.h"

#define NUM_PRES_BLINKS CONFIG_NUM_PRES_BLINKS
#define NUM_HANDLED_INPUTS 2

//GPIO RELEVANT TO MASTER STATION
//INPUTS
#define MASTER_GPIO_INPUT_COMMAND_PIN_UNO CONFIG_MASTER_INPUT_COMMAND_PIN_UNO
#define MASTER_GPIO_INPUT_COMMAND_PIN_DUE CONFIG_MASTER_INPUT_COMMAND_PIN_DUE
#define MASTER_GPIO_INPUT_COMMAND_PINS ((1ULL << MASTER_GPIO_INPUT_COMMAND_PIN_UNO) | (1ULL << MASTER_GPIO_INPUT_COMMAND_PIN_DUE))

//OUTPUTS
#define MASTER_BLINK_GPIO CONFIG_MASTER_BLINK_GPIO
#define MASTER_OK_GPIO CONFIG_MASTER_GPIO_OK
#define MASTER_NOK_GPIO CONFIG_MASTER_GPIO_NOK

#define MASTER_GPIO_OUTPUT_LEDS ((1ULL << MASTER_BLINK_GPIO) | (1ULL << MASTER_OK_GPIO) | (1ULL << MASTER_NOK_GPIO))
#define MASTER_GPIO_OUTPUT_PINS MASTER_GPIO_OUTPUT_LEDS

//GPIO RELEVANT TO SLAVE STATION
//INPUTS
#define SLAVE_GPIO_INPUT_COMMAND_PIN_UNO CONFIG_SLAVE_INPUT_COMMAND_PIN_UNO
#define SLAVE_GPIO_INPUT_COMMAND_PIN_DUE CONFIG_SLAVE_INPUT_COMMAND_PIN_DUE
#define SLAVE_GPIO_INPUT_COMMAND_PINS ((1ULL << SLAVE_GPIO_INPUT_COMMAND_PIN_UNO) | (1ULL << SLAVE_GPIO_INPUT_COMMAND_PIN_DUE))
//OUTPUTS
#define SLAVE_BLINK_GPIO CONFIG_SLAVE_BLINK_GPIO
#define SLAVE_GARAGE_CMD_GPIO CONFIG_SLAVE_GARAGE_CMD_GPIO
#define SLAVE_BIGGATE_CMD_GPIO CONFIG_SLAVE_BIGGATE_CMD_GPIO

#define SLAVE_GPIO_OUTPUT_LEDS (1ULL << BLINK_GPIO)
#define SLAVE_GPIO_OUTPUT_CMDS ((1ULL << SLAVE_GARAGE_CMD_GPIO) | (1ULL << SLAVE_BIGGATE_CMD_GPIO))
#define SLAVE_GPIO_OUTPUT_PINS SLAVE_GPIO_OUTPUT_LEDS | SLAVE_GPIO_OUTPUT_CMDS

//GPIO RELEVANT TO MOBILE STATION
//INPUTS
#define MOBILE_GPIO_INPUT_COMMAND_PIN_UNO CONFIG_MOBILE_INPUT_COMMAND_PIN_UNO
#define MOBILE_GPIO_INPUT_COMMAND_PIN_DUE CONFIG_MOBILE_INPUT_COMMAND_PIN_DUE
#define MOBILE_GPIO_INPUT_COMMAND_PINS ((1ULL << MOBILE_GPIO_INPUT_COMMAND_PIN_UNO) | (1ULL << MOBILE_GPIO_INPUT_COMMAND_PIN_DUE))

//OUTPUTS
#define MOBILE_BLINK_GPIO CONFIG_MOBILE_BLINK_GPIO
#define MOBILE_OK_GPIO CONFIG_MOBILE_GPIO_OK
#define MOBILE_NOK_GPIO CONFIG_MOBILE_GPIO_NOK

#define MOBILE_GPIO_OUTPUT_LEDS ((1ULL << MOBILE_BLINK_GPIO) | (1ULL << MOBILE_OK_GPIO) | (1ULL << MOBILE_NOK_GPIO))
#define MOBILE_GPIO_OUTPUT_PINS MOBILE_GPIO_OUTPUT_LEDS

//HC12 BAUDERATE
#ifdef CONFIG_BAUDTRANS1200
#define BAUDETRANSPARENTMODE 1200
#endif
#ifdef CONFIG_BAUDTRANS2400
#define BAUDETRANSPARENTMODE 2400
#endif
#ifdef CONFIG_BAUDTRANS4800
#define BAUDETRANSPARENTMODE 4800
#endif
#ifdef CONFIG_BAUDTRANS9600
#define BAUDETRANSPARENTMODE 9600
#endif
#ifdef CONFIG_BAUDTRANS19200
#define BAUDETRANSPARENTMODE 19200
#endif
#ifdef CONFIG_BAUDTRANS38400
#define BAUDETRANSPARENTMODE 38400
#endif
#ifdef CONFIG_BAUDTRANS57600
#define BAUDETRANSPARENTMODE 57600
#endif
#ifdef CONFIG_BAUDTRANS115200
#define BAUDETRANSPARENTMODE 115200
#endif

//HC12 PIN EQUAL FOR ALL STATIONS
#define HC12SETGPIO CONFIG_HC12SETGPIO
#define HC12TXGPIO CONFIG_HC12TXGPIO
#define HC12RXGPIO CONFIG_HC12RXGPIO
#define CH_NO_HC12 CONFIG_CH_NO_HC12
////////////////////////////////////////////// AUXILIARY FUNCTIONS //////////////////////////////////////////////
void foreverRed(unsigned char blink_gpio);
void presentBlink(unsigned char LedPinNo, unsigned char num_pres_blinks);
void setupmyRadioHC12(void);