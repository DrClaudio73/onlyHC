#include "sdkconfig.h"

#define NUM_PRES_BLINKS CONFIG_NUM_PRES_BLINKS
#define NUM_HANDLED_INPUTS 2
#define GPIO_INPUT_COMMAND_PIN_UNO CONFIG_COMMAND_PIN_UNO
#define GPIO_INPUT_COMMAND_PIN_DUE CONFIG_COMMAND_PIN_DUE
#define GPIO_INPUT_COMMAND_PINS  ((1ULL<<GPIO_INPUT_COMMAND_PIN_UNO) | (1ULL<<GPIO_INPUT_COMMAND_PIN_DUE))

#define BLINK_GPIO CONFIG_BLINK_GPIO
#define OK_GPIO CONFIG_GPIO_OK
#define NOK_GPIO CONFIG_GPIO_NOK
#define GPIO_OUTPUT_LEDS ((1ULL<<BLINK_GPIO) | (1ULL<<OK_GPIO) | (1ULL<<NOK_GPIO))


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

#define HC12SETGPIO CONFIG_HC12SETGPIO
#define HC12TXGPIO CONFIG_HC12TXGPIO
#define HC12RXGPIO CONFIG_HC12RXGPIO
#define CH_NO_HC12  CONFIG_CH_NO_HC12
////////////////////////////////////////////// AUXILIARY FUNCTIONS //////////////////////////////////////////////
void foreverRed(unsigned char blink_gpio);
void presentBlink(unsigned char LedPinNo, unsigned char num_pres_blinks);
void setupmyRadioHC12(void);