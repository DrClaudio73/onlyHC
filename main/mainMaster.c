//main

#include <stdio.h>
// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// UART driver
#include "driver/uart.h"

// Error library
#include "esp_err.h"
#include "string.h"
//#include <ctype.h>
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "cripter.h"
size_t bytes;

//mbed library
//#include "mbedtls/config.h"
//#include "mbedtls/platform.h"
//#include "mbedtls/sha1.h"
//#include "mbedtls/aes.h"

//Personal modules includes
#include "funzioni.h"
#include "typeconv.h"
#include "messagehandle.h"
#include "auxiliaryfuncs.h"
#include "eventengine.h"
#include <time.h>
#include <sys/time.h>

#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
//#include "tipostazione.h"

#ifdef DEVOPS_THIS_IS_STATION_SLAVE
#include "miohttpd.h"
#include "sntp/sntp.h"
#endif

#define TIME_1 50
#define TIME_2 50
#define TIME_3 50
#define TIME_4 50
#define TIME_5 50


////////////////////////////////////////////// GLOBAL VARIABLES //////////////////////////////////////////////
gpio_num_t miogpio_input_command_pin[NUM_HANDLED_INPUTS];
static EventGroupHandle_t my_event_group;
const int TRIGGERED = BIT0;
static const char *TAG1 = "OnlyHC12App";
////////////////////////////////////////////// HELPER FUNCTIONS //////////////////////////////////////////////
void stampa_db(miodb_t *db)
{
    ESP_LOGD(TAG1, "*******");
    ESP_LOGD(TAG1, "DATA= %s", db->DATA);
    ESP_LOGD(TAG1, "ORA = %s", db->ORA);
    ESP_LOGD(TAG1, "num_APRI_received = %u", db->num_APRI_received);
    ESP_LOGD(TAG1, "num_TOT_received = %u", db->num_TOT_received);
    ESP_LOGD(TAG1, "*******");
}

void print_struct_tm(char *tag, struct tm *t)
{
    printf("\033[1;36m\r\n%s.tm_sec: %d, ", tag, t->tm_sec); // seconds
    printf("\033[1;33m%s.tm_min: %d, ", tag, t->tm_min);     // minutes
    printf("\033[1;36m%s.tm_hour: %d, ", tag, t->tm_hour);   // hours
    printf("\033[1;33m%s.tm_mday: %d, ", tag, t->tm_mday);   // day of the month
    printf("\033[1;36m%s.tm_mon: %d, ", tag, t->tm_mon);     // month
    printf("\033[1;33m%s.tm_year: %d, ", tag, t->tm_year);   // year
    printf("\033[1;36m%s.tm_wday: %d, ", tag, t->tm_wday);   // day of the week
    printf("\033[1;33m%s.tm_yday: %d, ", tag, t->tm_yday);   // day in the year
    printf("\033[1;36m%s.tm_isdst: %d, ", tag, t->tm_isdst); // daylight saving time
    printf("\033[1;31msize %s: %u\r\n", tag, sizeof(struct tm));
    return;
}

void give_gpio_feedback(char unsigned OK_val, char unsigned NOK_val)
{
#ifdef OK_GPIO
    ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)NOK_GPIO, NOK_val));
    ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)OK_GPIO, OK_val));
#endif
    if (OK_val || NOK_val)
    { //if I lit any LED then shall trigger TIMER (timeout task)
        xEventGroupSetBits(my_event_group, TRIGGERED);
    }
    return;
}

/*
int endian(void)
{
 //
 // Variables definition
 //
    short magic, test;
    char *ptr;
    long long magicone = 0XFFEEDDCCBBAA9988;
    long long magicine = 0XBBAA9988;
    magicone += magicine;

    time_t now = 1582987191;
    struct tm timeinfo;

    printf("size of magic: %d\r\n", sizeof(magic));
    printf("size of test: %d\r\n", sizeof(test));
    printf("size of magicone: %d\r\n", sizeof(magicone));
    printf("size of now: %d\r\n", sizeof(now));
    printf("size of timeinfo: %d\r\n", sizeof(timeinfo));

    ptr = (char *)&magicone;
    for (int i = 0; i < sizeof(magicone); i++)
    {
        printf("magicone[%d]=%2hhX\n", i, ptr[i]);
    }

    magic = 0xABCD; // endianess magic number 
    ptr = (char *)&magic;
    test = (ptr[1] << 8) + (ptr[0] & 0xFF); // build value byte by byte 
    return (magic == test);                 // if the same is little endian 
}
*/

////////////////////////////////////////////// SETUP FUNCTIONS //////////////////////////////////////////////
void setup(commands_t *my_commands, commands_t *rcv_commands, miodb_t *db)
{
    ESP_LOGI(TAG1, "==============================");
    ESP_LOGI(TAG1, "Welcome to HC12 control App");
    ESP_LOGI(TAG1, "Setting up...................");

    /* CONFIGURING PRESENTATION BLINK LED */
    //gpio_pad_select_gpio((gpio_num_t)BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    //gpio_set_direction((gpio_num_t)BLINK_GPIO, GPIO_MODE_OUTPUT);
    //gpio_set_level((gpio_num_t)BLINK_GPIO, 1); //switch the led OFF
    //ESP_LOGW(TAG1,"Set up presentation led ended");

    /* CONFIGURING OUTPUT LEDS */
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PINS;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    ESP_LOGI(TAG1, "Set up of output leds ended");

    /* CONFIGURING COMMAND INPUT */
    //bit mask of the pins, use GPIO4 here
    io_conf.pin_bit_mask = GPIO_INPUT_COMMAND_PINS;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //diisable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    ESP_LOGI(TAG1, "Set up of command inputs ended");

    miogpio_input_command_pin[0] = (gpio_num_t)GPIO_INPUT_COMMAND_PIN_UNO;
    miogpio_input_command_pin[1] = (gpio_num_t)GPIO_INPUT_COMMAND_PIN_DUE;

    for (int i = 0; i < NUM_HANDLED_INPUTS; i++)
    {
        ESP_LOGV(TAG1, "IN[%u] =%u; gpio_input_command_pin[%u]=%u", i, (unsigned char)gpio_get_level(miogpio_input_command_pin[i]), i, miogpio_input_command_pin[i]);
    }

    //Initializing Radio HC12
    setupmyRadioHC12();
    ESP_LOGI(TAG1, "Set up of HC12 Radio ended");

    my_event_group = xEventGroupCreate();
    ESP_LOGI(TAG1, "Event Group Created");

    //Initializing DB
    for (unsigned char i = 0; i < NUM_MAX_CMDS; i++)
    {
        memset(my_commands->commands_status[i].cmd, 0, sizeof(my_commands->commands_status[i].cmd));
        memset(my_commands->commands_status[i].param, 0, sizeof(my_commands->commands_status[i].param));
        my_commands->commands_status[i].rep_counts = 0;
        my_commands->commands_status[i].num_checks = 0;
        my_commands->commands_status[i].addr_pair = 0xFF;       //command is not of interest (is not yet valid)
        my_commands->commands_status[i].uart_controller = 0x00; //uart is unknown

        memset(rcv_commands->commands_status[i].cmd, 0, sizeof(rcv_commands->commands_status[i].cmd));
        memset(rcv_commands->commands_status[i].param, 0, sizeof(rcv_commands->commands_status[i].param));
        rcv_commands->commands_status[i].rep_counts = 0;
        rcv_commands->commands_status[i].num_checks = 0;
        rcv_commands->commands_status[i].addr_pair = 0xFF;       //command is not of interest (is not yet valid)
        rcv_commands->commands_status[i].uart_controller = 0x00; //uart is unknown
    }
    my_commands->num_cmd_under_processing = 0;
    rcv_commands->num_cmd_under_processing = 0;

#ifdef DEVOPS_THIS_IS_STATION_SLAVE
    /* 
        do { //TO TEST ENDIANNESS
            int i, val = 0xABCDEF01;
            char a='a';
            char * ptr;

            printf("size of i: %d\r\n", sizeof(i));
            printf("size of val: %d\r\n", sizeof(val));
            printf("size of ptr: %d\r\n", sizeof(ptr));
            printf("size of a: %d\r\n", sizeof(a));

            printf("Using value %X\n", val);
            ptr = (char *) &val;
            for (i=0; i<sizeof(int); i++) {
                printf("val[%d]=%2hhX\n", i, ptr[i]);
            }

            if (endian()==0){
                printf("big endian\r\n");
            } else {
                printf("little endian\r\n");
            }
        } while (0);*/

    /*do{ //TO TEST ENVIRONMENT AT LEAST FOR USE WITH tzset()
            extern char ** environ ;
            int i=0;
            printf("trying to print environ\r\n");
            while (environ != NULL){
                if (*environ == NULL) break;
                printf("environ[%d]=%s\r\n",i,*(++environ));
                i++;
            }
            // change the timezone to Italy
            setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
            tzset();
            i=0;
            printf("trying to print environ\r\n");
            while (environ != NULL){
                if (*environ == NULL) break;
                printf("environ[%d]=%s\r\n",i,*(environ++));
                i++;
            }

        } while (0); */

    /*do { //TO TEST TIME FUNCTIONS
            //time_p
            time_t time_p;
            time_t time2_p;
            for (int i=0; i<2 ; i++){ //with and without tzset()
                printf("%dth_time***************************\r\n",i+1);
                time_p = time(&time_p);
                printf("time_p: %d, size: %u\r\n",(int)time_p,sizeof(time_p));
                
                //tm (i.e. breakdown structures)
                struct tm gmt_, tm_, calendario;
                char asctime_[100],ctime_[100];

                gmtime_r(&time_p,&gmt_);
                print_struct_tm("gmt_",&gmt_);
                
                localtime_r(&time_p,&tm_);
                print_struct_tm("tm_",&tm_);
                
                //ascii
                asctime_r(&tm_,asctime_);
                printf("asctime_: %s\r\n",asctime_);

                ctime_r(&time_p,ctime_);
                printf("ctime_: %s\r\n",ctime_);

                //back to time_p
                memset(&calendario,0,sizeof(calendario));
                calendario.tm_year=120;
                calendario.tm_mon=3;
                calendario.tm_mday=0;
                calendario.tm_min=15; 
                print_struct_tm("calendario",&calendario);
                time2_p=mktime(&calendario);
                printf("time2_p: %ld, size: %u\r\n",(long int)time2_p,sizeof(time2_p));

                //get system clock and then set it different
                struct timespec res;

                printf("retCode clock_getres: %d\r\n",clock_getres(CLOCK_REALTIME,&res));
                printf("res.tv_sec: %ld\r\n",res.tv_sec);
                printf("res.tv_nsec: %ld\r\n",res.tv_nsec);
                
                printf("retCode clock_gettime: %d\r\n",clock_gettime(CLOCK_REALTIME,&res));
                printf("res.tv_sec: %ld\r\n",res.tv_sec);
                printf("res.tv_nsec: %ld\r\n",res.tv_nsec);

                //res.tv_sec+=3600;
                printf("retCode clock_gettime: %d\r\n",clock_settime(CLOCK_REALTIME,&res));
                printf("res.tv_sec: %ld\r\n",res.tv_sec);
                printf("res.tv_nsec: %ld\r\n",res.tv_nsec);

                // change the timezone to Italy
                setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
                tzset();
            }

            printf("***************************\r\n");

            //get&settimeofday()
            struct timeval tempo,tempoforz, tempo2;
            int ret=gettimeofday(&tempo,NULL);
            printf("Now, current system date is %ld-%ld; retCode:%d\n",tempo.tv_sec,tempo.tv_usec, ret);

            tempoforz.tv_sec = 1582876576;
            tempoforz.tv_usec  = 0;
            ret=settimeofday(&tempoforz,NULL);
            printf("retCode of settimeofday: %d\n", ret);
            fflush(stdout);
            vTaskDelay(TIME_10/portTICK_RATE_MS);
            ret=gettimeofday(&tempo2,NULL);
            printf("Now, current system date is %ld-%ld, retCode: %d\n",tempo2.tv_sec,tempo2.tv_usec, ret);

        } while (0);*/

    struct timespec res;
    int ret_ = clock_gettime(CLOCK_REALTIME, &res);
    ESP_LOGV(TAG1, "retCode clock_gettime: %d", ret_);
    ESP_LOGV(TAG1, "res.tv_sec: %ld", res.tv_sec);
    ESP_LOGV(TAG1, "res.tv_nsec: %ld", res.tv_nsec);
    res.tv_sec += 1587281658; // 1584425168; //1583819573;
    ret_ = clock_settime(CLOCK_REALTIME, &res);
    ESP_LOGV(TAG1, "retCode clock_gettime: %d", ret_);
    ESP_LOGV(TAG1, "res.tv_sec: %ld", res.tv_sec);
    ESP_LOGV(TAG1, "res.tv_nsec: %ld", res.tv_nsec);

    time_t now;
    struct tm timeinfo;
    char buffer[100];

    time(&now);
    localtime_r(&now, &timeinfo);

    ESP_LOGI(TAG1, "Actual UTC time:");
    strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
    ESP_LOGV(TAG1, "- %s", buffer);
    strftime(buffer, sizeof(buffer), "%A, %d %B %Y", &timeinfo);
    ESP_LOGV(TAG1, "- %s", buffer);
    strftime(buffer, sizeof(buffer), "Today is day %j of year %Y", &timeinfo);
    ESP_LOGV(TAG1, "- %s", buffer);
    ESP_LOGV(TAG1, "\n");

    // change the timezone to Italy
    setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
    tzset();

    time(&now);
    localtime_r(&now, &timeinfo);
    ESP_LOGV(TAG1, "Actual Italy time:\n");
    strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
    ESP_LOGV(TAG1, "- %s\n", buffer);
    strftime(buffer, sizeof(buffer), "%A, %d %B %Y", &timeinfo);
    ESP_LOGV(TAG1, "- %s\n", buffer);
    strftime(buffer, sizeof(buffer), "Today is day %j of year %Y", &timeinfo);
    ESP_LOGV(TAG1, "- %s\n", buffer);
    ESP_LOGV(TAG1, "\n");
    strftime(buffer, sizeof(buffer), "%c", &timeinfo);
    ESP_LOGI(TAG1, "The current date/time in Italy is: %s", buffer);

    //do something only for STATIONSLAVE
    strftime((char *)db->DATA, sizeof(db->DATA), "%Y%m%d", &timeinfo);
    strftime((char *)db->ORA, sizeof(db->ORA), "%H%M%S", &timeinfo);
    db->num_APRI_received = 0;
    db->num_TOT_received = 0;
#else
    // change the timezone to Italy
    setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
    tzset();

    strcpy2(db->DATA, (const unsigned char *)"19000101");
    strcpy2(db->ORA, (const unsigned char *)"010000");
    db->num_APRI_received = 0;
    db->num_TOT_received = 0;
#endif

    stampa_db(db);
    //presentation blinking
    presentBlink(BLINK_GPIO, NUM_PRES_BLINKS);
    ESP_LOGW(TAG1, "Set up of everything ended");
    ESP_LOGW(TAG1, "==============================");

    return;
}
////////////////////////////////////////////// TIMEOUT FUNCTIONS //////////////////////////////////////////////
void timeout_task(void *pvParameter)
{

    while (1)
    {
        // wait the timeout period
        EventBits_t uxBits;
        uxBits = xEventGroupWaitBits(my_event_group, TRIGGERED, true, true, CONFIG_TIMEOUT / portTICK_PERIOD_MS);

        // if found bit was not set, the function returned after the timeout period
        if ((uxBits & TRIGGERED) == 0)
        {
            // turn both leds off
            give_gpio_feedback(0, 0);
        }
    }
}

int ijk = 0;
////////////////////////////////////////////// CORE FUNCTIONS //////////////////////////////////////////////
int loop(commands_t *my_commands, commands_t *rcv_commands, miodb_t *db)
{
    evento_t *detected_event;
    unsigned char cmd_param[FIELD_MAX];
    memset(cmd_param, 0, sizeof(cmd_param));
    unsigned char cmd_cmdtosend[FIELD_MAX];
    memset(cmd_cmdtosend, 0, sizeof(cmd_param));

    /*do
    {
        int i, ret;
        unsigned char digest[20];
        memset(digest, 0, sizeof(digest));

        char KEY_SEED[40];
        memset(KEY_SEED, 0, sizeof(KEY_SEED));
        strcpy(KEY_SEED, "35113553344345512bfcfdfef010130a");
        toUpperstr(KEY_SEED, strlen(KEY_SEED), "KEY_SEED");

        char ACA_NID[17];
        char BLOB_STR[512];
        unsigned char BLOB_BYTES[512];

        memset(ACA_NID, 0, sizeof(ACA_NID));
        memset(BLOB_STR, 0, sizeof(BLOB_STR));
        memset(BLOB_BYTES, 0, sizeof(BLOB_BYTES));
        char ACA[13];
        memset(ACA, 0, sizeof(ACA));
        strcpy(ACA, "bc1212131314");
        if (strlen(ACA) == 12)
        {
            mbedtls_printf("Correct NID (%s) length: %d\r\n", ACA, strlen(ACA));
            toUpperstr(ACA, strlen(ACA), "ACA");
            strcpy(ACA_NID, ACA);
            mbedtls_printf("ACA_NID: %s\r\n", ACA);
        }
        else
        {
            mbedtls_printf("NID (%s) length (%d) is NOT correct\r\n", ACA, strlen(ACA));
            return (MBEDTLS_EXIT_FAILURE);
        }

        strcpy(BLOB_STR, "PP");
        strcat(BLOB_STR, KEY_SEED);
        strcat(BLOB_STR, ACA_NID);
        mbedtls_printf("BLOB_STR: %s length (%d).\r\n", BLOB_STR, strlen(BLOB_STR));
        fflush(stdout);
        mbedtls_printf("\n  SHA1('%s') = ", BLOB_STR);
        fflush(stdout);

        if ((ret = mbedtls_sha1_ret((unsigned char *)BLOB_STR, strlen(BLOB_STR), digest)) != 0)
            return (MBEDTLS_EXIT_FAILURE);

        for (i = 0; i < 20; i++)
            mbedtls_printf("%02X", digest[i]);

        unsigned char key[16];
        unsigned char P[MAX_BLOCKS][16];
        unsigned char C[MAX_BLOCKS][16];
        unsigned char D[MAX_BLOCKS][16];
        unsigned char IV[16];

        memset(key, 0, sizeof(key));
        memset(IV, 0, sizeof(IV));
        memset(P, 0, sizeof(P)); //P will host the bytes to be encypted
        memset(C, 0, sizeof(C)); //C will host the bytes encrypted
        memset(D, 0, sizeof(D)); //D will host the bytes decrypted

        mbedtls_printf("\n");
        mbedtls_printf("\n  key = ");
        for (i = 0; i < 16; i++)
        {
            key[i] = digest[i];
            mbedtls_printf("%02X", key[i]);
        }
        mbedtls_printf("\r\n");

        esp_aes_context aes_ctx_crit;
        mbedtls_aes_init(&aes_ctx_crit);
        mbedtls_aes_setkey_enc(&aes_ctx_crit, key, 128);
        esp_aes_context aes_ctx_decrit;
        mbedtls_aes_init(&aes_ctx_decrit);
        mbedtls_aes_setkey_enc(&aes_ctx_decrit, key, 128);

        char VIN[33];
        memset(VIN, 0, sizeof(VIN));
        strcpy(VIN, "bc12121313143aF87b7740d8f4c70001");
        toUpperstr(VIN, strlen(VIN), "VIN");
        my_str_to_bytes(IV, VIN, 16, "VIN");
        mbedtls_printf("\n  IV = ");
        for (i = 0; i < 16; i++)
        {
            mbedtls_printf("%02X", IV[i]);
        }
        mbedtls_printf("\r\n");

        char line1[1024];
        memset(line1, 0, sizeof(line1));
        strcpy(line1, "03010101017F000A6503abacadffbc121213131403010101017F000A6503ABACADFFBC1212131314ffbc12121313141a");
        toUpperstr(line1, strlen(line1), "line1");
        int n_blocks = my_pad(P, line1, strlen(line1));

        for (int j = 0; j < n_blocks; j++)
        {
            mbedtls_printf("\n  P[%d] = ", j);
            for (i = 0; i < 16; i++)
                mbedtls_printf("%02X", P[j][i]);
        }
        mbedtls_printf("\n");
        // encrypting
        mbedtls_printf("\n  AES_ECB('%s') = \n", line1);
        crittalinea(n_blocks, P, C, IV, key);

        for (int j = 0; j < n_blocks; j++)
        {
            mbedtls_printf("\n  C[%d] = ", j);
            for (i = 0; i < 16; i++)
                mbedtls_printf("%02X", C[j][i]);
        }
        mbedtls_printf("\r\n");

        decrittalinea(n_blocks, C, D, IV, key);
        for (int j = 0; j < n_blocks; j++)
        {
            mbedtls_printf("\n  D[%d] = ", j);
            for (i = 0; i < 16; i++)
                mbedtls_printf("%02X", D[j][i]);
        }

        mbedtls_printf("\n\n");
    } while (0);*/
    printf("loop(): *******************************************Entering loop() function!!!!!");
    time_t now;
    struct tm timeinfo;
    char buffer[100];
    time(&now);
    localtime_r(&now, &timeinfo);
    ESP_LOGD(TAG1, "Actual Italy time:");
    strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
    ESP_LOGD(TAG1, "- %s", buffer);
    strftime(buffer, sizeof(buffer), "%A, %d %B %Y", &timeinfo);
    ESP_LOGD(TAG1, "- %s", buffer);
    strftime(buffer, sizeof(buffer), "Today is day %j of year %Y", &timeinfo);
    ESP_LOGD(TAG1, "- %s", buffer);
    ESP_LOGD(TAG1, "\n");
    strftime(buffer, sizeof(buffer), "%c", &timeinfo);
    ESP_LOGI(TAG1, "The current date/time in Italy is: %s", buffer);

    //PER INVIARE I VALORI NUMERICI CON IL PROTOCOLLO CHE TI SEI SCELTO DEVI USARE
    //sprintf(param,"val[%d]=%2hhX\n", i, ptr[i]);
    //COME NEI TEST DELLA ENDIANNESS
    //list_commands_status(my_commands,"my_commands");

    /*for (unsigned char l = 0; l < NUM_HANDLED_INPUTS; l++)
    {
        printf("loop(): miogpio_input_command_pin[%u]: %u\r\n", l, (unsigned char)miogpio_input_command_pin[l]);
    }*/

    stampa_db(db);

    //Listening for any event to occour
    detected_event = detect_event(UART_NUM_2, miogpio_input_command_pin, my_commands, rcv_commands);

    if (STATION_ROLE == STATIONMASTER)
    { //BEHAVE AS MASTER STATION
        ESP_LOGI(TAG1, "\nloop(): ######################This is the Master station######################");
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
        if (beginsWith((unsigned char *)buffer, "01:00:30") || beginsWith((unsigned char *)buffer, "01:00:31"))
        { //update clock from SLAVE once a day
            strcpy2(cmd_cmdtosend, (const unsigned char *)"RPT");
            strcpy2(cmd_param, (const unsigned char *)"TIME");
            invia_comando(UART_NUM_2, my_commands, ADDR_MASTER_STATION, ADDR_SLAVE_STATION, (const unsigned char *)cmd_cmdtosend, (const unsigned char *)cmd_param, 1);
        }

//#define AUTO_ISSUE_CMD_FOR_DEBUG_MASTER   //// DELETE FOR PRODUCTION  //////////
#ifdef AUTO_ISSUE_CMD_FOR_DEBUG_MASTER
        switch (ijk)
        {
        case 0:
            strcpy2(cmd_cmdtosend, (const unsigned char *)"APRI");
            sprintf((char *)cmd_param, "%d", detected_event->valore_evento.input_number);
            break;

        case 1:
            strcpy2(cmd_cmdtosend, (const unsigned char *)"APRI");
            sprintf((char *)cmd_param, "%d", detected_event->valore_evento.input_number + 1); //to avoid having to change the connected input for debug purpose
            break;

        case 2:
            strcpy2(cmd_cmdtosend, (const unsigned char *)"RPT");
            strcpy2(cmd_param, (const unsigned char *)"DATE");
            break;

        case 3:
            strcpy2(cmd_cmdtosend, (const unsigned char *)"RPT");
            strcpy2(cmd_param, (const unsigned char *)"HOUR");
            break;

        case 4:
            strcpy2(cmd_cmdtosend, (const unsigned char *)"RPT");
            strcpy2(cmd_param, (const unsigned char *)"TIME");
            break;

        case 5:
            strcpy2(cmd_cmdtosend, (const unsigned char *)"RPT");
            strcpy2(cmd_param, (const unsigned char *)"NOR");
            break;

        case 6:
            strcpy2(cmd_cmdtosend, (const unsigned char *)"RPT");
            strcpy2(cmd_param, (const unsigned char *)"NTC");
            break;
        default:
            break;
        }
        ijk = (ijk + 1) % 7;
        invia_comando(UART_NUM_2, my_commands, ADDR_MASTER_STATION, ADDR_SLAVE_STATION, (const unsigned char *)cmd_cmdtosend, (const unsigned char *)cmd_param, 1);
#endif //// DELETE END //////////

        if (detected_event->type_of_event == IO_INPUT_ACTIVE)
        { // CHECKING IF MY INPUT PIN TOLD ME TO SEND A CMD TO SLAVE STATION
            if (detected_event->valore_evento.value_of_input == 1)
            { // NOTHING TO DO:
                ESP_LOGI(TAG1, "loop(): EVENTO SU I_O PIN %u GESTITO CORRETTAMENTE!!!!!", detected_event->valore_evento.input_number);
                fflush(stdout);
                ESP_LOGD(TAG1, "loop():detected_event->type_of_event: %u", detected_event->type_of_event);
                ESP_LOGD(TAG1, "loop():detected_event->valore_evento.input_number: %u", detected_event->valore_evento.input_number);
                ESP_LOGD(TAG1, "loop():detected_event->valore_evento.value_of_input: %u", detected_event->valore_evento.value_of_input);
            }
            else //SENDING COMMAND TO OPEN
            {
                give_gpio_feedback(0, 0); //swithces off NOK and OK led since a new command is going to be issued

#define USE_ONE_INPUT_FOR_ALL_COMMANDS_MASTER //// DELETE FOR PRODUCTION /////
#ifdef USE_ONE_INPUT_FOR_ALL_COMMANDS_MASTER
                switch (ijk)
                {
                case 0:
                    strcpy2(cmd_cmdtosend, (const unsigned char *)"APRI");
                    sprintf((char *)cmd_param, "%d", detected_event->valore_evento.input_number);
                    break;

                case 1:
                    strcpy2(cmd_cmdtosend, (const unsigned char *)"APRI");
                    sprintf((char *)cmd_param, "%d", detected_event->valore_evento.input_number + 1); //to avoid having to change the connected input for debug purpose
                    break;

                case 2:
                    strcpy2(cmd_cmdtosend, (const unsigned char *)"RPT");
                    strcpy2(cmd_param, (const unsigned char *)"DATE");
                    break;

                case 3:
                    strcpy2(cmd_cmdtosend, (const unsigned char *)"RPT");
                    strcpy2(cmd_param, (const unsigned char *)"HOUR");
                    break;

                case 4:
                    strcpy2(cmd_cmdtosend, (const unsigned char *)"RPT");
                    strcpy2(cmd_param, (const unsigned char *)"TIME");
                    break;

                case 5:
                    strcpy2(cmd_cmdtosend, (const unsigned char *)"RPT");
                    strcpy2(cmd_param, (const unsigned char *)"NOR");
                    break;

                case 6:
                    strcpy2(cmd_cmdtosend, (const unsigned char *)"RPT");
                    strcpy2(cmd_param, (const unsigned char *)"NTC");
                    break;
                default:
                    break;
                }
                ijk = (ijk + 1) % 7;
                invia_comando(UART_NUM_2, my_commands, ADDR_MASTER_STATION, ADDR_SLAVE_STATION, (const unsigned char *)cmd_cmdtosend, (const unsigned char *)cmd_param, 1);
#else
                sprintf((char *)cmd_param, "%d", detected_event->valore_evento.input_number);
                invia_comando(UART_NUM_2, my_commands, ADDR_MASTER_STATION, ADDR_SLAVE_STATION, (const unsigned char *)"APRI", (const unsigned char *)cmd_param, 1);
#endif
            }
        }
        else if (detected_event->type_of_event == RECEIVED_MSG)
        {
            ESP_LOGI(TAG1, "loop(): The detected event is a msg for me! With this content:");
            ESP_LOGI(TAG1, "loop():detected_event->valore_evento.cmd_received: %s", detected_event->valore_evento.cmd_received);
            ESP_LOGI(TAG1, "loop():detected_event->valore_evento.param_received: %s", detected_event->valore_evento.param_received);
            ESP_LOGI(TAG1, "loop():detected_event->valore_evento.ack_rep_counts: %u", detected_event->valore_evento.ack_rep_counts);
            ESP_LOGI(TAG1, "loop():detected_event->valore_evento.pair_addr: %u", detected_event->valore_evento.pair_addr);

            //I am sending ACK to the sending station in any case (i.e. even if a previous repetition of the same command has already been received????)
            if (!beginsWith(detected_event->valore_evento.cmd_received, "ACK"))
            {
                invia_ack(UART_NUM_2, my_commands, ADDR_MASTER_STATION, detected_event);
            }
            //check if this one is an actual new command and in case I track in the received commands list to avoid actuating repetitions
            if (is_rcv_a_new_cmd(rcv_commands, detected_event))
            {
                if (!beginsWith(detected_event->valore_evento.cmd_received, "RSP"))
                {
                    //if I have received a command I am forwarding it to the slave station (for now MASTER station just forwards received commands)
                    give_gpio_feedback(0, 0); //swithces off NOK and OK led since a new command is going to be issued
                    db->num_TOT_received++;
                    invia_comando(UART_NUM_2, my_commands, ADDR_MASTER_STATION, ADDR_SLAVE_STATION, detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received, 1); //FORWARD RECEIVED COMMAND TO SLAVE STATION
                    //In SLAVE STATION I insetad would expect to actuate something
                }
                else
                {
                    //If this is a RESP to a previously issued 'RPT' CMD then print the content
                    ESP_LOGI(TAG1, "received reply to CMD: %s, PARAM_RCV: %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                    if (beginsWith(detected_event->valore_evento.param_received, "DATE"))
                    {
                        ESP_LOGI(TAG1, "loop():EXTRACTING DATE FROM RSP CMD: %s -- %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        print_struct_tm("timeinfo_before", &timeinfo);
                        unsigned char tmp[FIELD_MAX];
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_year = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("DATE"), 4)) - 1900; //YYYY is 4 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_mon = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("DATE2020"), 2)) - 1; //MM is 2 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_mday = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("DATE202001"), 2)); //DD is 2 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_isdst = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("DATE20200101"), 1));
                        print_struct_tm("timeinfo_after", &timeinfo);
                        struct timespec res;
                        int ret_ = clock_gettime(CLOCK_REALTIME, &res);
                        ESP_LOGV(TAG1, "retCode clock_gettime: %d", ret_);
                        res.tv_sec = mktime(&timeinfo);
                        ret_ = clock_settime(CLOCK_REALTIME, &res);
                        ESP_LOGV(TAG1, "retCode clock_settime: %d", ret_);
                        vTaskDelay(TIME_1 / portTICK_RATE_MS);
                        //updating DB wall clock
                        time(&now);
                        localtime_r(&now, &timeinfo);
                        strftime((char *)db->DATA, sizeof(db->DATA), "%Y%m%d", &timeinfo);
                        strftime((char *)db->ORA, sizeof(db->ORA), "%H%M%S", &timeinfo);
                    }
                    if (beginsWith(detected_event->valore_evento.param_received, "HOUR"))
                    {
                        ESP_LOGD(TAG1, "loop():EXTRACTING HOUR FROM RSP CMD: %s -- %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        print_struct_tm("timeinfo_before", &timeinfo);
                        unsigned char tmp[FIELD_MAX];
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_hour = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("HOUR"), 2)); //HH is 2 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_min = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("HOUR00"), 2)); //MM is 2 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_sec = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("HOUR0000"), 2)); //SS is 2 chars long
                        print_struct_tm("timeinfo_after", &timeinfo);
                        struct timespec res;
                        int ret_ = clock_gettime(CLOCK_REALTIME, &res);
                        ESP_LOGD(TAG1, "retCode clock_gettime: %d", ret_);
                        res.tv_sec = mktime(&timeinfo);
                        ret_ = clock_settime(CLOCK_REALTIME, &res);
                        ESP_LOGD(TAG1, "retCode clock_settime: %d", ret_);
                        vTaskDelay(TIME_1 / portTICK_RATE_MS);
                        //updating DB wall clock
                        time(&now);
                        localtime_r(&now, &timeinfo);
                        strftime((char *)db->DATA, sizeof(db->DATA), "%Y%m%d", &timeinfo);
                        strftime((char *)db->ORA, sizeof(db->ORA), "%H%M%S", &timeinfo);
                    }
                    if (beginsWith(detected_event->valore_evento.param_received, "TIME"))
                    {
                        ESP_LOGD(TAG1, "loop():EXTRACTING TIME FROM RSP CMD: %s -- %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        print_struct_tm("timeinfo_before", &timeinfo);
                        unsigned char tmp[FIELD_MAX];
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_year = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("TIME"), 4)) - 1900; //YYYY is 4 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_mon = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("TIME2020"), 2)) - 1; //MM is 2 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_mday = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("TIME202001"), 2)); //DD is 2 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_isdst = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("TIME20200101"), 1));
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_hour = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("TIME202001010"), 2)); //HH is 2 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_min = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("TIME20200101012"), 2)); //MM is 2 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_sec = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("TIME2020010101200"), 2)); //SS is 2 chars long
                        print_struct_tm("timeinfo_after", &timeinfo);
                        struct timespec res;
                        int ret_ = clock_gettime(CLOCK_REALTIME, &res);
                        ESP_LOGV(TAG1, "retCode clock_gettime: %d", ret_);
                        res.tv_sec = mktime(&timeinfo);
                        ret_ = clock_settime(CLOCK_REALTIME, &res);
                        ESP_LOGV(TAG1, "retCode clock_settime: %d", ret_);
                        //updating DB wall clock
                        time(&now);
                        localtime_r(&now, &timeinfo);
                        strftime((char *)db->DATA, sizeof(db->DATA), "%Y%m%d", &timeinfo);
                        strftime((char *)db->ORA, sizeof(db->ORA), "%H%M%S", &timeinfo);
                        vTaskDelay(TIME_1 / portTICK_RATE_MS);
                    }
                    if (beginsWith(detected_event->valore_evento.param_received, "NOR"))
                    {
                        ESP_LOGI(TAG1, "loop():EXTRACTING NUM_APRI_RCVED FROM RSP CMD: %s -- %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        unsigned char tmp[FIELD_MAX];
                        memset(tmp, 0, sizeof(tmp));
                        ESP_LOGI(TAG1, "loop(): SLAVE HAS RECEIVED %ld APRI COMMANDS", strtol((const char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("NOR"), 4), NULL, 16)); //NOR BRINGS A 4 BYTES HEX CODED FIELD
                        vTaskDelay(TIME_1 / portTICK_RATE_MS);
                    }
                    if (beginsWith(detected_event->valore_evento.param_received, "NTC"))
                    {
                        ESP_LOGI(TAG1, "loop():EXTRACTING NUM_TOTALCMDS_RCVED FROM RSP CMD: %s -- %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        unsigned char tmp[FIELD_MAX];
                        memset(tmp, 0, sizeof(tmp));
                        ESP_LOGI(TAG1, "loop(): SLAVE HAS RECEIVED AN OVERALL %ld COMMANDS", strtol((const char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("NTC"), 4), NULL, 16)); //NTC BRINGS A 4 BYTES HEX CODED FIELD
                        vTaskDelay(TIME_1 / portTICK_RATE_MS);
                    }
                    vTaskDelay(TIME_2 / portTICK_RATE_MS);
                }
            }
        }
        else if (detected_event->type_of_event == RECEIVED_ACK)
        {
            ESP_LOGI(TAG1, "loop(): received and ACK for command:");
            give_gpio_feedback(1, 0); //NOK led is switched off and OK led is lit since an ACK has been received
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.cmd_received: %s", detected_event->valore_evento.cmd_received);
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.param_received: %s", detected_event->valore_evento.param_received);
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.ack_rep_counts: %u", detected_event->valore_evento.ack_rep_counts);
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.pair_addr: %u", detected_event->valore_evento.pair_addr);
            vTaskDelay(TIME_2 / portTICK_RATE_MS);
        }
        else if (detected_event->type_of_event == FAIL_TO_SEND_CMD)
        {
            ESP_LOGE(TAG1, "loop(): failure to send command:");
            give_gpio_feedback(0, 1); //NOK led is lit and OK led is switched off since a FAILURE happened

            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.cmd_received: %s", detected_event->valore_evento.cmd_received);
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.param_received: %s", detected_event->valore_evento.param_received);
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.ack_rep_counts: %u", detected_event->valore_evento.ack_rep_counts);
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.pair_addr: %u", detected_event->valore_evento.pair_addr);
            vTaskDelay(TIME_3 / portTICK_RATE_MS);
        }
        else
        {
            vTaskDelay(TIME_4 / portTICK_RATE_MS);
            return 0;
        }
    } //if (STATION_ROLE==STATIONMASTER)

    if (STATION_ROLE == STATIONSLAVE)
    { //BEHAVE AS SLAVE STATION
        ESP_LOGI(TAG1, "\nn######################This is the Slave station######################\n");

        if (detected_event->type_of_event == RECEIVED_MSG)
        {
            ESP_LOGI(TAG1, "loop(): This was the msg for me:");
            ESP_LOGI(TAG1, "loop():cmd_received: %s", detected_event->valore_evento.cmd_received);
            ESP_LOGI(TAG1, "loop():param_received: %s", detected_event->valore_evento.param_received);
            ESP_LOGI(TAG1, "loop():ack_rep_counts: %u", detected_event->valore_evento.ack_rep_counts);
            ESP_LOGI(TAG1, "loop():pair_addr: %u", detected_event->valore_evento.pair_addr);

            //I am sending ACK to the sending station in any case (i.e. even if a previous repetition of the same command has already been received????)
            if (!beginsWith(detected_event->valore_evento.cmd_received, "ACK"))
            {
                invia_ack(UART_NUM_2, my_commands, ADDR_SLAVE_STATION, detected_event);
            }
            //check if this one is an actual new command and in case I track in the received commands to avoid implementing repetitions
            if (is_rcv_a_new_cmd(rcv_commands, detected_event))
            {
                //Actuating APRI commands
                if (beginsWith(detected_event->valore_evento.cmd_received, "APRI"))
                {
                    db->num_APRI_received++;
                    db->num_TOT_received++;
                    if (beginsWith(detected_event->valore_evento.param_received, "0"))
                    {
                        ESP_LOGI(TAG1, "loop():ACTUATING RECEIVED CMD: %s -- %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        vTaskDelay(TIME_3 / portTICK_RATE_MS);
                        ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)SLAVE_GARAGE_CMD_GPIO, 0)); //closing GARAGE relay
                        vTaskDelay(CONFIG_ON_COMMAND_LENGTH / portTICK_RATE_MS);
                        ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)SLAVE_GARAGE_CMD_GPIO, 1)); //relasing GARAGE relay
                    }
                    if (beginsWith(detected_event->valore_evento.param_received, "1"))
                    {
                        ESP_LOGI(TAG1, "loop():ACTUATING RECEIVED CMD: %s -- %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        vTaskDelay(TIME_3 / portTICK_RATE_MS);
                        ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)SLAVE_BIGGATE_CMD_GPIO, 0)); //closing BIG GATE relay
                        vTaskDelay(CONFIG_ON_COMMAND_LENGTH / portTICK_RATE_MS);
                        ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)SLAVE_BIGGATE_CMD_GPIO, 1)); //relasing BIG GATE relay
                    }
                    if (beginsWith(detected_event->valore_evento.param_received, "2"))
                    {
                        ESP_LOGI(TAG1, "loop():ACTUATING RECEIVED CMD: %s -- %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        vTaskDelay(TIME_3 / portTICK_RATE_MS);
                        ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)SLAVE_GARAGE_CMD_GPIO, 0));  //closing GARAGE relay
                        ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)SLAVE_BIGGATE_CMD_GPIO, 0)); //closing BIG GATE relay
                        vTaskDelay(CONFIG_ON_COMMAND_LENGTH / portTICK_RATE_MS);
                        ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)SLAVE_GARAGE_CMD_GPIO, 1));  //relasing GARAGE relay
                        ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)SLAVE_BIGGATE_CMD_GPIO, 1)); //relasing BIG GATE relay
                    }
                    return 0;
                }
                //REPORT COMMAND HANDLING
                if (beginsWith(detected_event->valore_evento.cmd_received, "RPT"))
                {
                    //updating DB wall clock
                    time(&now);
                    localtime_r(&now, &timeinfo);
                    strftime((char *)db->DATA, sizeof(db->DATA), "%Y%m%d", &timeinfo);
                    strftime((char *)db->ORA, sizeof(db->ORA), "%H%M%S", &timeinfo);

                    if (beginsWith(detected_event->valore_evento.param_received, "DATE"))
                    {
                        db->num_TOT_received++;
                        ESP_LOGI(TAG1, "loop():ACTUATING RECEIVED CMD: %s -- %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        strncpy2(cmd_param, detected_event->valore_evento.param_received, sizeof(cmd_param));
                        strncat2(cmd_param, db->DATA, sizeof(cmd_param));
                        cmd_param[strlen2(cmd_param)] = timeinfo.tm_isdst + 0x30; //Adding DST at the end of cmd_poaram string
                        invia_comando(UART_NUM_2, my_commands, ADDR_SLAVE_STATION, detected_event->valore_evento.pair_addr, (const unsigned char *)"RSP", cmd_param, 1);
                        vTaskDelay(TIME_1 / portTICK_RATE_MS);
                    }
                    if (beginsWith(detected_event->valore_evento.param_received, "HOUR"))
                    {
                        db->num_TOT_received++;
                        ESP_LOGI(TAG1, "loop():ACTUATING RECEIVED CMD: %s -- %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        strncpy2(cmd_param, detected_event->valore_evento.param_received, sizeof(cmd_param));
                        strncat2(cmd_param, db->ORA, sizeof(cmd_param));
                        invia_comando(UART_NUM_2, my_commands, ADDR_SLAVE_STATION, detected_event->valore_evento.pair_addr, (const unsigned char *)"RSP", cmd_param, 1);
                        vTaskDelay(TIME_1 / portTICK_RATE_MS);
                    }
                    if (beginsWith(detected_event->valore_evento.param_received, "TIME"))
                    {
                        db->num_TOT_received++;
                        ESP_LOGI(TAG1, "loop():ACTUATING RECEIVED CMD: %s -- %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        strncpy2(cmd_param, detected_event->valore_evento.param_received, sizeof(cmd_param));
                        strncat2(cmd_param, db->DATA, sizeof(cmd_param));
                        cmd_param[strlen2(cmd_param)] = timeinfo.tm_isdst + 0x30; //Adding DST at the end of cmd_poaram string
                        strncat2(cmd_param, db->ORA, sizeof(cmd_param));
                        invia_comando(UART_NUM_2, my_commands, ADDR_SLAVE_STATION, detected_event->valore_evento.pair_addr, (const unsigned char *)"RSP", cmd_param, 1);
                        vTaskDelay(TIME_1 / portTICK_RATE_MS);
                    }
                    if (beginsWith(detected_event->valore_evento.param_received, "NOR"))
                    {
                        db->num_TOT_received++;
                        ESP_LOGI(TAG1, "loop():ACTUATING RECEIVED CMD: %s -- %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        strncpy2(cmd_param, detected_event->valore_evento.param_received, sizeof(cmd_param));
                        snprintf((char *)(cmd_param + strlen2(cmd_param)), sizeof(cmd_param) - strlen2(cmd_param), "%4hhX\n", db->num_APRI_received);
                        //strncat2(cmd_param,db->num_APRI_received,sizeof(cmd_param));
                        invia_comando(UART_NUM_2, my_commands, ADDR_SLAVE_STATION, detected_event->valore_evento.pair_addr, (const unsigned char *)"RSP", cmd_param, 1);
                        vTaskDelay(TIME_1 / portTICK_RATE_MS);
                    }
                    if (beginsWith(detected_event->valore_evento.param_received, "NTC"))
                    {
                        db->num_TOT_received++;
                        ESP_LOGI(TAG1, "loop():ACTUATING RECEIVED CMD: %s -- %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        strncpy2(cmd_param, detected_event->valore_evento.param_received, sizeof(cmd_param));
                        snprintf((char *)(cmd_param + strlen2(cmd_param)), sizeof(cmd_param) - strlen2(cmd_param), "%4hhX\n", db->num_TOT_received);
                        //strncat2(cmd_param,db->num_TOT_received,sizeof(cmd_param));
                        invia_comando(UART_NUM_2, my_commands, ADDR_SLAVE_STATION, detected_event->valore_evento.pair_addr, (const unsigned char *)"RSP", cmd_param, 1);
                        vTaskDelay(TIME_1 / portTICK_RATE_MS);
                    }
                    return 0;
                }
            }
        }
        else if (detected_event->type_of_event == RECEIVED_ACK)
        { //for now this should not happen in SLAVE station since it does not emit commands
            ESP_LOGI(TAG1, "loop(): received and ACK for command:");
            give_gpio_feedback(1, 0); //NOK led is switched off and OK led is lit since an ACK has been received
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.cmd_received: %s", detected_event->valore_evento.cmd_received);
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.param_received: %s", detected_event->valore_evento.param_received);
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.ack_rep_counts: %u", detected_event->valore_evento.ack_rep_counts);
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.pair_addr: %u", detected_event->valore_evento.pair_addr);
            vTaskDelay(TIME_2 / portTICK_RATE_MS);
        }
        else if (detected_event->type_of_event == FAIL_TO_SEND_CMD)
        {
            ESP_LOGE(TAG1, "loop(): failure to send command:");
            give_gpio_feedback(0, 1); //NOK led is lit and OK led is switched off since a FAILURE happened

            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.cmd_received: %s", detected_event->valore_evento.cmd_received);
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.param_received: %s", detected_event->valore_evento.param_received);
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.ack_rep_counts: %u", detected_event->valore_evento.ack_rep_counts);
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.pair_addr: %u", detected_event->valore_evento.pair_addr);
            vTaskDelay(TIME_3 / portTICK_RATE_MS);
        }
        else
        {
            vTaskDelay(TIME_4 / portTICK_RATE_MS);
            return 0;
        }
    } //if (STATION_ROLE==STATIONSLAVE)

    if (STATION_ROLE == STATIONMOBILE)
    { //BEHAVE AS MOBILE STATION
        ESP_LOGI(TAG1, "\nloop(): ######################This is the Mobile station######################\n");
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
        if (strncmp(buffer, "01:01:30", sizeof(buffer)) == 0)
        { //update clock from SLAVE once a day
            strcpy2(cmd_cmdtosend, (const unsigned char *)"RPT");
            strcpy2(cmd_param, (const unsigned char *)"TIME");
            invia_comando(UART_NUM_2, my_commands, ADDR_MOBILE_STATION, ADDR_SLAVE_STATION, (const unsigned char *)cmd_cmdtosend, (const unsigned char *)cmd_param, 1);
        }
        if (detected_event->type_of_event == IO_INPUT_ACTIVE)
        { // CHECKING IF MY INPUT PIN TOLD ME TO SEND A CMD TO SLAVE STATION
            if (detected_event->valore_evento.value_of_input == 1)
            { // NOTHING TO DO:
                ESP_LOGI(TAG1, "\r\nloop(): EVENTO SU I_O PIN %u GESTITO CORRETTAMENTE!!!!!", detected_event->valore_evento.input_number);
                fflush(stdout);
                ESP_LOGD(TAG1, "loop():detected_event->type_of_event: %u", detected_event->type_of_event);
                ESP_LOGD(TAG1, "loop():detected_event->valore_evento.input_number: %u", detected_event->valore_evento.input_number);
                ESP_LOGD(TAG1, "loop():detected_event->valore_evento.value_of_input: %u", detected_event->valore_evento.value_of_input);
            }
            else //SENDING COMMAND TO OPEN
            {
                give_gpio_feedback(0, 0); //swithces off NOK and OK led since a new command is going to be issued
#define USE_ONE_INPUT_FOR_ALL_COMMANDS_MOBILE
#ifdef USE_ONE_INPUT_FOR_ALL_COMMANDS_MOBILE
                switch (ijk)
                {
                case 0:
                    strcpy2(cmd_cmdtosend, (const unsigned char *)"APRI");
                    sprintf((char *)cmd_param, "%d", detected_event->valore_evento.input_number);
                    break;

                case 1:
                    strcpy2(cmd_cmdtosend, (const unsigned char *)"APRI");
                    sprintf((char *)cmd_param, "%d", detected_event->valore_evento.input_number + 1); //to avoid having to change the connected input for debug purpose
                    break;

                case 2:
                    strcpy2(cmd_cmdtosend, (const unsigned char *)"RPT");
                    strcpy2(cmd_param, (const unsigned char *)"DATE");
                    break;

                case 3:
                    strcpy2(cmd_cmdtosend, (const unsigned char *)"RPT");
                    strcpy2(cmd_param, (const unsigned char *)"HOUR");
                    break;

                case 4:
                    strcpy2(cmd_cmdtosend, (const unsigned char *)"RPT");
                    strcpy2(cmd_param, (const unsigned char *)"TIME");
                    break;

                case 5:
                    strcpy2(cmd_cmdtosend, (const unsigned char *)"RPT");
                    strcpy2(cmd_param, (const unsigned char *)"NOR");
                    break;

                case 6:
                    strcpy2(cmd_cmdtosend, (const unsigned char *)"RPT");
                    strcpy2(cmd_param, (const unsigned char *)"NTC");
                    break;
                default:
                    break;
                }
                ijk = (ijk + 1) % 7;
#else
                invia_comando(UART_NUM_2, my_commands, ADDR_MASTER_STATION, ADDR_SLAVE_STATION, (const unsigned char *)cmd_cmdtosend, (const unsigned char *)cmd_param, 1);
                //sprintf((char *)cmd_param,"%d",detected_event->valore_evento.input_number);
                //ATTENZIONE SE EMETTI I DUE COMANDI COSì ALLORA LO SLAVE POTREBBE ATTUARE 2 VOLTE IL COMANDO CON EFFETTI INDESIDERATI
                //A MENO DI TOLGIERE IN is_rcv_a_new_cmd() IL CONTROLLO SUL PAIR_ADDRESS
                invia_comando(UART_NUM_2, my_commands, ADDR_MOBILE_STATION, ADDR_SLAVE_STATION, (const unsigned char *)"APRI", (const unsigned char *)cmd_param, 1);
                invia_comando(UART_NUM_2, my_commands, ADDR_MOBILE_STATION, ADDR_MASTER_STATION, (const unsigned char *)"APRI", (const unsigned char *)cmd_param, 1);
#endif
            }
        }
        else if (detected_event->type_of_event == RECEIVED_MSG)
        {
            ESP_LOGI(TAG1, "loop(): The detected event is a msg for me! With this content:");
            ESP_LOGI(TAG1, "loop():detected_event->valore_evento.cmd_received: %s", detected_event->valore_evento.cmd_received);
            ESP_LOGI(TAG1, "loop():detected_event->valore_evento.param_received: %s", detected_event->valore_evento.param_received);
            ESP_LOGI(TAG1, "loop():detected_event->valore_evento.ack_rep_counts: %u", detected_event->valore_evento.ack_rep_counts);
            ESP_LOGI(TAG1, "loop():detected_event->valore_evento.pair_addr: %u", detected_event->valore_evento.pair_addr);

            //I am sending ACK to the sending station in any case (i.e. even if a previous repetition of the same command has already been received????)
            if (!beginsWith(detected_event->valore_evento.cmd_received, "ACK"))
            {
                invia_ack(UART_NUM_2, my_commands, ADDR_MOBILE_STATION, detected_event);
            }

            //check if this one is an actual new command and in case I track in the received commands list to avoid actuating repetitions
            if (is_rcv_a_new_cmd(rcv_commands, detected_event))
            {
                if (!beginsWith(detected_event->valore_evento.cmd_received, "RSP"))
                {
                    //if I have received a command I am forwarding it to the slave station (for now MOBILE station just forwards received commands)
                    give_gpio_feedback(0, 0); //swithces off NOK and OK led since a new command is going to be issued
                    db->num_TOT_received++;
                    invia_comando(UART_NUM_2, my_commands, ADDR_MOBILE_STATION, ADDR_SLAVE_STATION, detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received, 1); //FORWARD RECEIVED COMMAND TO SLAVE STATION
                    //In SLAVE STATION I insetad would expect to actuate something
                }
                else
                {
                    //If this is a RESP to a previously issued 'RPT' CMD then print the content
                    ESP_LOGI(TAG1, "received reply to CMD: %s, PARAM_RCV: %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                    if (beginsWith(detected_event->valore_evento.param_received, "DATE"))
                    {
                        ESP_LOGI(TAG1, "loop():EXTRACTING DATE FROM RSP CMD: %s -- %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        print_struct_tm("timeinfo_before", &timeinfo);
                        unsigned char tmp[FIELD_MAX];
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_year = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("DATE"), 4)) - 1900; //YYYY is 4 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_mon = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("DATE2020"), 2)) - 1; //MM is 2 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_mday = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("DATE202001"), 2)); //DD is 2 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_isdst = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("DATE20200101"), 1));
                        print_struct_tm("timeinfo_after", &timeinfo);
                        struct timespec res;
                        int ret_ = clock_gettime(CLOCK_REALTIME, &res);
                        ESP_LOGV(TAG1, "retCode clock_gettime: %d", ret_);
                        res.tv_sec = mktime(&timeinfo);
                        ret_ = clock_settime(CLOCK_REALTIME, &res);
                        ESP_LOGV(TAG1, "retCode clock_settime: %d", ret_);
                        vTaskDelay(TIME_1 / portTICK_RATE_MS);
                        //updating DB wall clock
                        time(&now);
                        localtime_r(&now, &timeinfo);
                        strftime((char *)db->DATA, sizeof(db->DATA), "%Y%m%d", &timeinfo);
                        strftime((char *)db->ORA, sizeof(db->ORA), "%H%M%S", &timeinfo);
                    }
                    if (beginsWith(detected_event->valore_evento.param_received, "HOUR"))
                    {
                        ESP_LOGI(TAG1, "loop():EXTRACTING HOUR FROM RSP CMD: %s -- %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        print_struct_tm("timeinfo_before", &timeinfo);
                        unsigned char tmp[FIELD_MAX];
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_hour = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("HOUR"), 2)); //HH is 2 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_min = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("HOUR00"), 2)); //MM is 2 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_sec = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("HOUR0000"), 2)); //SS is 2 chars long
                        print_struct_tm("timeinfo_after", &timeinfo);
                        struct timespec res;
                        int ret_ = clock_gettime(CLOCK_REALTIME, &res);
                        ESP_LOGV(TAG1, "retCode clock_gettime: %d", ret_);
                        res.tv_sec = mktime(&timeinfo);
                        ret_ = clock_settime(CLOCK_REALTIME, &res);
                        ESP_LOGV(TAG1, "retCode clock_settime: %d", ret_);
                        vTaskDelay(TIME_1 / portTICK_RATE_MS);
                        //updating DB wall clock
                        time(&now);
                        localtime_r(&now, &timeinfo);
                        strftime((char *)db->DATA, sizeof(db->DATA), "%Y%m%d", &timeinfo);
                        strftime((char *)db->ORA, sizeof(db->ORA), "%H%M%S", &timeinfo);
                    }
                    if (beginsWith(detected_event->valore_evento.param_received, "TIME"))
                    {
                        ESP_LOGI(TAG1, "loop():EXTRACTING TIME FROM RSP CMD: %s -- %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        print_struct_tm("timeinfo_before", &timeinfo);
                        unsigned char tmp[FIELD_MAX];
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_year = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("TIME"), 4)) - 1900; //YYYY is 4 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_mon = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("TIME2020"), 2)) - 1; //MM is 2 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_mday = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("TIME202001"), 2)); //DD is 2 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_isdst = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("TIME20200101"), 1));
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_hour = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("TIME202001010"), 2)); //HH is 2 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_min = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("TIME20200101012"), 2)); //MM is 2 chars long
                        memset(tmp, 0, sizeof(tmp));
                        timeinfo.tm_sec = atoi((char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("TIME2020010101200"), 2)); //SS is 2 chars long
                        print_struct_tm("timeinfo_after", &timeinfo);
                        struct timespec res;
                        int ret_ = clock_gettime(CLOCK_REALTIME, &res);
                        ESP_LOGV(TAG1, "retCode clock_gettime: %d", ret_);
                        res.tv_sec = mktime(&timeinfo);
                        ret_ = clock_settime(CLOCK_REALTIME, &res);
                        ESP_LOGV(TAG1, "retCode clock_settime: %d", ret_);
                        //updating DB wall clock
                        time(&now);
                        localtime_r(&now, &timeinfo);
                        strftime((char *)db->DATA, sizeof(db->DATA), "%Y%m%d", &timeinfo);
                        strftime((char *)db->ORA, sizeof(db->ORA), "%H%M%S", &timeinfo);
                        vTaskDelay(TIME_1 / portTICK_RATE_MS);
                    }
                    if (beginsWith(detected_event->valore_evento.param_received, "NOR"))
                    {
                        ESP_LOGI(TAG1, "loop():EXTRACTING NUM_APRI_RCVED FROM RSP CMD: %s -- %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        unsigned char tmp[FIELD_MAX];
                        memset(tmp, 0, sizeof(tmp));
                        ESP_LOGI(TAG1, "loop(): SLAVE HAS RECEIVED %ld APRI COMMANDS", strtol((const char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("NOR"), 4), NULL, 16)); //NOR BRINGS A 4 BYTES HEX CODED FIELD
                        vTaskDelay(TIME_1 / portTICK_RATE_MS);
                    }
                    if (beginsWith(detected_event->valore_evento.param_received, "NTC"))
                    {
                        ESP_LOGI(TAG1, "loop():EXTRACTING NUM_TOTALCMDS_RCVED FROM RSP CMD: %s -- %s", detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received);
                        unsigned char tmp[FIELD_MAX];
                        memset(tmp, 0, sizeof(tmp));
                        ESP_LOGI(TAG1, "loop(): SLAVE HAS RECEIVED AN OVERALL %ld COMMANDS", strtol((const char *)strncpy2(tmp, detected_event->valore_evento.param_received + strlen("NTC"), 4), NULL, 16)); //NTC BRINGS A 4 BYTES HEX CODED FIELD
                        vTaskDelay(TIME_1 / portTICK_RATE_MS);
                    }
                    vTaskDelay(TIME_2 / portTICK_RATE_MS);
                }
            }
        }
        else if (detected_event->type_of_event == RECEIVED_ACK)
        {
            ESP_LOGI(TAG1, "loop(): received and ACK for command:");
            give_gpio_feedback(1, 0); //NOK led is switched off and OK led is lit since an ACK has been received
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.cmd_received: %s", detected_event->valore_evento.cmd_received);
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.param_received: %s", detected_event->valore_evento.param_received);
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.ack_rep_counts: %u", detected_event->valore_evento.ack_rep_counts);
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.pair_addr: %u", detected_event->valore_evento.pair_addr);
            vTaskDelay(TIME_2 / portTICK_RATE_MS);
        }
        else if (detected_event->type_of_event == FAIL_TO_SEND_CMD)
        {
            ESP_LOGE(TAG1, "loop(): failure to send command:");
            give_gpio_feedback(0, 1); //NOK led is lit and OK led is switched off since a FAILURE happened

            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.cmd_received: %s", detected_event->valore_evento.cmd_received);
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.param_received: %s", detected_event->valore_evento.param_received);
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.ack_rep_counts: %u", detected_event->valore_evento.ack_rep_counts);
            ESP_LOGD(TAG1, "loop():detected_event->valore_evento.pair_addr: %u", detected_event->valore_evento.pair_addr);
            vTaskDelay(TIME_3 / portTICK_RATE_MS);
        }
        else
        {
            vTaskDelay(TIME_4 / portTICK_RATE_MS);
            return 0;
        }
    } //if (STATION_ROLE==STATIONMOBILE)

    /*    if (STATION_ROLE == STATIONMOBILE)
    { //BEHAVE AS MOBILE STATION
    
        printf("\nloop(): ######################This is the Mobile station######################\n");

        if (detected_event->type_of_event == IO_INPUT_ACTIVE)
        { // CHECKING IF MY INPUT PIN TOLD ME TO SEND A CMD TO SLAVE STATION
            if (detected_event->valore_evento.value_of_input == 1)
            { // NOTHING TO DO:
                mbedtls_printf("\r\nloop(): EVENTO SU I_O PIN %u GESTITO CORRETTAMENTE!!!!!\r\n", detected_event->valore_evento.input_number);
                //ESP_LOGW(TAG1,"loop(): The detected event is a BUTTON press:\r\n");
                fflush(stdout);
                mbedtls_printf("loop():detected_event->type_of_event: %u\r\n", detected_event->type_of_event);
                mbedtls_printf("loop():detected_event->valore_evento.input_number: %u\r\n", detected_event->valore_evento.input_number);
                mbedtls_printf("loop():detected_event->valore_evento.value_of_input: %u\r\n", detected_event->valore_evento.value_of_input);
            }
            else
            {                             //SENDING COMMAND TO OPEN
                give_gpio_feedback(0, 0); //swithces off NOK and OK led since a new command is going to be issued
                sprintf((char *)cmd_param, "%d", detected_event->valore_evento.input_number);
                //ATTENZIONE SE EMETTI I DUE COMANDI COSì ALLORA LO SLAVE POTREBBE ATTUARE 2 VOLTE IL COMANDO CON EFFETTI INDESIDERATI
                //A MENO DI TOLGIERE IN is_rcv_a_new_cmd() IL CONTROLLO SUL PAIR_ADDRESS
                invia_comando(UART_NUM_2, my_commands, ADDR_MOBILE_STATION, ADDR_SLAVE_STATION, (const unsigned char *)"APRI", (const unsigned char *)cmd_param, 1);
                invia_comando(UART_NUM_2, my_commands, ADDR_MOBILE_STATION, ADDR_MASTER_STATION, (const unsigned char *)"APRI", (const unsigned char *)cmd_param, 1);
            }
        }
        else if (detected_event->type_of_event == RECEIVED_MSG)
        {
            ESP_LOGW(TAG1, "loop(): The detected event is a msg for me! With this content:\r\n");
            mbedtls_printf("loop():detected_event->valore_evento.cmd_received: %s\r\n", detected_event->valore_evento.cmd_received);
            mbedtls_printf("loop():detected_event->valore_evento.param_received: %s\r\n", detected_event->valore_evento.param_received);
            mbedtls_printf("loop():detected_event->valore_evento.ack_rep_counts: %u\r\n", detected_event->valore_evento.ack_rep_counts);
            mbedtls_printf("loop():detected_event->valore_evento.pair_addr: %u\r\n", detected_event->valore_evento.pair_addr);

            //I am sending ACK to the sending station in any case (i.e. even if a previous repetition of the same command has already been received)
            invia_ack(UART_NUM_2, my_commands, ADDR_MOBILE_STATION, detected_event);
            //check if this one is an actually new command and in case I track in the received commands to avoid implementing repetitions
            if (is_rcv_a_new_cmd(rcv_commands, detected_event))
            {
                //if I have received a command I am forwarding it to the slave station (for now MOBILE station just forwards received commands)
                give_gpio_feedback(0, 0);                                                                                                                                                     //swithces off NOK and OK led since a new command is going to be issued
                invia_comando(UART_NUM_2, my_commands, ADDR_MOBILE_STATION, ADDR_SLAVE_STATION, detected_event->valore_evento.cmd_received, detected_event->valore_evento.param_received, 1); //FORWARD RECEIVED COMMAND TO SLAVE STATION
                //In SLAVE STATION I insetad would expect to actuate something
            }
        }
        else if (detected_event->type_of_event == RECEIVED_ACK)
        {
            ESP_LOGW(TAG1, "loop(): received and ACK for command:\r\n");
            give_gpio_feedback(1, 0); //NOK led is switched off and OK led is lit since an ACK has been received
            mbedtls_printf("loop():detected_event->valore_evento.cmd_received: %s\r\n", detected_event->valore_evento.cmd_received);
            mbedtls_printf("loop():detected_event->valore_evento.param_received: %s\r\n", detected_event->valore_evento.param_received);
            mbedtls_printf("loop():detected_event->valore_evento.ack_rep_counts: %u\r\n", detected_event->valore_evento.ack_rep_counts);
            mbedtls_printf("loop():detected_event->valore_evento.pair_addr: %u\r\n", detected_event->valore_evento.pair_addr);
            vTaskDelay(TIME_2 / portTICK_RATE_MS);
        }
        else if (detected_event->type_of_event == FAIL_TO_SEND_CMD)
        {
            ESP_LOGW(TAG1, "loop(): failure to send command:\r\n");
            give_gpio_feedback(0, 1); //NOK led is lit and OK led is switched off since a FAILURE happened
            mbedtls_printf("loop():detected_event->valore_evento.cmd_received: %s\r\n", detected_event->valore_evento.cmd_received);
            mbedtls_printf("loop():detected_event->valore_evento.param_received: %s\r\n", detected_event->valore_evento.param_received);
            mbedtls_printf("loop():detected_event->valore_evento.ack_rep_counts: %u\r\n", detected_event->valore_evento.ack_rep_counts);
            mbedtls_printf("loop():detected_event->valore_evento.pair_addr: %u\r\n", detected_event->valore_evento.pair_addr);
            vTaskDelay(TIME_3 / portTICK_RATE_MS);
        }
        else
        {
            vTaskDelay(TIME_1 / portTICK_RATE_MS);
            return;
        }
    }       //if (STATION_ROLE==STATIONMOBILE)
    return; //to be removed since it is already done in each STATION_ROLE if statement
*/
    return 0;
}

// Main application
void app_main()
{
    // Initialize
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

#ifdef DEVOPS_THIS_IS_STATION_SLAVE
    httpd_app_main();
#endif

    static commands_t my_commands;
    static commands_t rcv_commands;
    static miodb_t db;
    //esp_log_level_set("*", ESP_LOG_INFO);        // set all components to ERROR level
    setup(&my_commands, &rcv_commands, &db);
    xTaskCreate(&timeout_task, "timeout_task", 2048, NULL, 5, NULL);

    ESP_LOGE(TAG1, "Entering while loop!!!!!");
    while (1)
        loop(&my_commands, &rcv_commands, &db);
    fflush(stdout);
    return;
}