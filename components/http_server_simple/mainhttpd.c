/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
//#include <nvs_flash.h>
#include <sys/param.h>
//#include "nvs_flash.h"
#include "esp_netif.h"
//#include "esp_eth.h"
//#include "protocol_examples_common.h"
#include <esp_http_server.h>
#include "freertos/FreeRTOS.h"

#include <time.h>
#include <sys/time.h>
#include "string.h"
#include <stdlib.h>     /* atoi */

/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server. */

static const char *TAG_HTTPD = "HTTPD";
static char iploc[30];

#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN
static httpd_handle_t start_webserver(void);

/* An HTTP GET handler */
static esp_err_t hello_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG_HTTPD, "Found header => Host: %s", buf);
            strncpy(iploc,buf,sizeof(iploc));
        }

        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG_HTTPD, "Found header => Test-Header-2: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG_HTTPD, "Found header => Test-Header-1: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    char *token;
    static int segno=0;
    static int delta_clock;
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG_HTTPD, "Found URL query => %s", buf);
            char param[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "query1", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG_HTTPD, "Found URL query parameter => query1=%s", param);
                
                token = strtok(param,"=");
                while( token != NULL ) {
                    ESP_LOGV(TAG_HTTPD," %s", token );
                    if (strncmp(token,"P",1)==0){
                        segno=1;
                    } else if (strncmp(token,"M",1)==0) {
                        segno=-1;
                    } else {
                        segno=0;
                    }
                    token = strtok(NULL, "=");
                }
            }

            if (httpd_query_key_value(buf, "query3", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG_HTTPD, "Found URL query parameter => query3=%s", param);
            }

            if (httpd_query_key_value(buf, "query2", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG_HTTPD, "Found URL query parameter => query2=%s", param);
                token = strtok(param,"=");
                while( token != NULL ) {
                    ESP_LOGD(TAG_HTTPD," %s", token );
                    if((delta_clock=atoi(token))>0){
                        ESP_LOGD(TAG_HTTPD,"\r\n********\r\nYou told me: %d!\r\n**************", delta_clock);
                        time_t now;
                        struct tm timeinfo;
                        //char buffer[100];
                        struct timespec res;
                        int ret_=clock_gettime(CLOCK_REALTIME,&res);
                        ESP_LOGD(TAG_HTTPD,"retCode clock_gettime: %d\r\n",ret_);
                        ESP_LOGD(TAG_HTTPD,"res.tv_sec: %ld\r\n",res.tv_sec);
                        ESP_LOGD(TAG_HTTPD,"res.tv_nsec: %ld\r\n",res.tv_nsec);
                        res.tv_sec+=delta_clock*segno;
                        ret_=clock_settime(CLOCK_REALTIME,&res);
                        ESP_LOGD(TAG_HTTPD,"retCode clock_gettime: %d\r\n",ret_);
                        ESP_LOGD(TAG_HTTPD,"res.tv_sec: %ld\r\n",res.tv_sec);
                        ESP_LOGD(TAG_HTTPD,"res.tv_nsec: %ld\r\n",res.tv_nsec);    
                        time(&now);
                        localtime_r(&now, &timeinfo);
                    } else if((strncmp(buf,"P",1)==0)) {
                        ESP_LOGD(TAG_HTTPD,"\r\n********\r\nPLUS SIGN DETECTED!!!!!!!\r\n**************\r\n");
                        segno=+1;
                    } else if((strncmp(buf,"M",1)==0)) {
                        ESP_LOGD(TAG_HTTPD,"\r\n********\r\nMINUS SIGN DETECTED!!!!!!!\r\n**************\r\n");
                        segno=-1;
                    } else {
                        ESP_LOGD(TAG_HTTPD,"\r\n********\r\nthis is not an integer, I won't do anything!!!!!!!\r\n**************\r\n");
                        segno=0;
                    }
                    token = strtok(NULL, "=");
                }
            }
        }
        free(buf);
    }

    /* Set some custom headers */
    httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
    httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    //const char* resp_str = (const char*) req->user_ctx;

    time_t now;
    struct tm timeinfo;
    char resp[1000], buffer[100];

    //Preparing info for HTML page
    memset(resp,0,sizeof(resp));
    memset(buffer,0,sizeof(buffer));
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(buffer, sizeof(buffer), "%c", &timeinfo);

    //Formatting HTML page
    strcat(resp,"<!DOCTYPE html><html lang=\"it\"><head><meta charset=\"utf-8\"><title>Set ESP32 Slave Clock</title><style>body {color: #ff6600;}</style></head><body><h1>");
    strcat(resp,req->user_ctx);
    strcat(resp,"</h1>");
    strcat(resp,"<h2>");
    strcat(resp,buffer);
    strcat(resp,"</h2>");
    strcat(resp,"<h2>Please specify parameters to adjust clock</h2><div><form action=\"/echo\" method=\"post\"> \
  <select id=\"operation\" name=\"operation\"><option value=\"P\">Forward</option><option value=\"M\">Backward</option></select><br><br> \
  <label for=\"value\">Delta:</label><input type=\"text\" id=\"delta\" name=\"delta\"><br><br> \
  <input type=\"submit\" value=\"Submit\"></form></div></body></html>");

    const char* resp_str = (const char*) resp;
    ESP_LOGD(TAG_HTTPD,"\r\n+*****\r\nresp_str: %s\r\n******", resp_str);
    httpd_resp_send(req, resp_str, strlen(resp_str));

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG_HTTPD, "Request headers lost");
    }

    return ESP_OK;
}

static const httpd_uri_t hello = {
    .uri       = "/hello",
    .method    = HTTP_GET,
    .handler   = hello_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "Hello World!"
};

/* An HTTP POST handler */
static esp_err_t echo_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                        MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        /* Send back the same data */
        //httpd_resp_send_chunk(req, buf, ret);
        remaining -= ret;
        /* Log data received */
        ESP_LOGI(TAG_HTTPD, "=========== RECEIVED DATA ==========");
        ESP_LOGI(TAG_HTTPD, "%.*s", ret, buf);
        ESP_LOGI(TAG_HTTPD, "====================================");

        int delta_clock=0;
        static int segno;
        char*  token = strtok(buf,"&=");
        while( token!= NULL ) {
            ESP_LOGD(TAG_HTTPD," %s\n", token );
            if (strncmp(token,"P",1)==0){
                segno=1;
            } else if (strncmp(token,"M",1)==0) {
                segno=-1;
            } 
            if((delta_clock=atoi(token))>0){
                ESP_LOGD(TAG_HTTPD,"\r\n********\r\nYou told me: %d*%d!\r\n**************", segno,delta_clock);
                time_t now;
                struct tm timeinfo;
                //char buffer[100];
                struct timespec res;
                int ret_= clock_gettime(CLOCK_REALTIME,&res);
                ESP_LOGD(TAG_HTTPD,"retCode clock_gettime: %d",ret_);
                ESP_LOGD(TAG_HTTPD,"res.tv_sec: %ld",res.tv_sec);
                ESP_LOGD(TAG_HTTPD,"res.tv_nsec: %ld",res.tv_nsec);
                res.tv_sec+=delta_clock*segno;
                ret_=clock_settime(CLOCK_REALTIME,&res);
                ESP_LOGD(TAG_HTTPD,"retCode clock_gettime: %d",ret_);
                ESP_LOGD(TAG_HTTPD,"res.tv_sec: %ld",res.tv_sec);
                ESP_LOGD(TAG_HTTPD,"res.tv_nsec: %ld",res.tv_nsec);    
                time(&now);
                localtime_r(&now, &timeinfo);
            }
            token = strtok(NULL, "&=");
        }
        time_t now;
        struct tm timeinfo;
        char resp[1000], buffer[200];

        //Preparing info for HTML page
        memset(resp,0,sizeof(resp));
        memset(buffer,0,sizeof(buffer));
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(buffer, sizeof(buffer), "New time is now: %c", &timeinfo);

        //Formatting HTML page
        strcat(resp,"<!DOCTYPE html><html lang=\"it\"><head><meta charset=\"utf-8\"><title>Set ESP32 Slave Clock</title><style>body {color: #ff0000;}</style></head><body><h1>");
        strcat(resp,buffer);
        strcat(resp,"</h1><h2>Operation performed:</h2><div><h3>Direction: ");
        sprintf(buffer,"%d</h3><h3>Value: %d</h3></div><div><a href=\"http://%s/hello\">Set again!</a></div></body></html>",segno,delta_clock,iploc);
        strcat(resp,buffer);

        const char* resp_str = (const char*) resp;
        ESP_LOGD(TAG_HTTPD,"\r\n+*****\r\nresp_str: %s\r\n******", resp_str);
        httpd_resp_send(req, resp_str, strlen(resp_str));

    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t echo = {
    .uri       = "/echo",
    .method    = HTTP_POST,
    .handler   = echo_post_handler,
    .user_ctx  = NULL
};

/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /ctrl, the /hello and /echo URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /hello or /echo is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * either closes the underlying socket (when requested URI is /echo)
 * or keeps it open (when requested URI is /hello). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/hello", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    } else if (strcmp("/echo", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/echo URI is not available");
        /* Return ESP_FAIL to close underlying socket */
        return ESP_FAIL;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

/* An HTTP PUT handler. This demonstrates realtime
 * registration and deregistration of URI handlers
 */
static esp_err_t ctrl_put_handler(httpd_req_t *req)
{
    char buf;
    int ret;

    if ((ret = httpd_req_recv(req, &buf, 1)) <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    if (buf == '0') {
        /* URI handlers can be unregistered using the uri string */
        ESP_LOGI(TAG_HTTPD, "Unregistering /hello and /echo URIs");
        httpd_unregister_uri(req->handle, "/hello");
        httpd_unregister_uri(req->handle, "/echo");
        /* Register the custom error handler */
        httpd_register_err_handler(req->handle, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    else {
        ESP_LOGI(TAG_HTTPD, "Registering /hello and /echo URIs");
        httpd_register_uri_handler(req->handle, &hello);
        httpd_register_uri_handler(req->handle, &echo);
        /* Unregister custom error handler */
        httpd_register_err_handler(req->handle, HTTPD_404_NOT_FOUND, NULL);
    }

    /* Respond with empty body */
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t ctrl = {
    .uri       = "/ctrl",
    .method    = HTTP_PUT,
    .handler   = ctrl_put_handler,
    .user_ctx  = NULL
};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG_HTTPD, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG_HTTPD, "Registering URI handlers");
        httpd_register_uri_handler(server, &hello);
        httpd_register_uri_handler(server, &echo);
        httpd_register_uri_handler(server, &ctrl);
        return server;
    }

    ESP_LOGI(TAG_HTTPD, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base, 
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGE(TAG_HTTPD, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    } else {
        ESP_LOGE(TAG_HTTPD, "STA DISCONNECTED BUT server is NULL");
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base, 
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGE(TAG_HTTPD, "Starting webserver");
        *server = start_webserver();
    }else {
        ESP_LOGE(TAG_HTTPD, "STA CONNECTED BUT server is NULL");
    }
}


//WIFI METHODS
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG_HTTPD, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
        //connect_handler(arg, event_base, event_id, event_data);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG_HTTPD, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
        //disconnect_handler(arg, event_base, event_id, event_data);
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .channel = 11
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_HTTPD, "wifi_init_softap finished. SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);

}


void httpd_app_main(void)
{    
    memset(iploc,0,sizeof(iploc));
    /*do{
        time_t now;
        struct tm timeinfo;
        char buffer[100];
        struct timespec res;
        printf("retCode clock_gettime: %d\r\n",clock_gettime(CLOCK_REALTIME,&res));
        printf("res.tv_sec: %ld\r\n",res.tv_sec);
        printf("res.tv_nsec: %ld\r\n",res.tv_nsec);
        res.tv_sec+=1583049153;
        printf("retCode clock_gettime: %d\r\n",clock_settime(CLOCK_REALTIME,&res));
        printf("res.tv_sec: %ld\r\n",res.tv_sec);
        printf("res.tv_nsec: %ld\r\n",res.tv_nsec);    time(&now);
        localtime_r(&now, &timeinfo);
            
        printf("Actual UTC time:\n");
        strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
        printf("- %s\n", buffer);
        strftime(buffer, sizeof(buffer), "%A, %d %B %Y", &timeinfo);
        printf("- %s\n", buffer);
        strftime(buffer, sizeof(buffer), "Today is day %j of year %Y", &timeinfo);
        printf("- %s\n", buffer);
        printf("\n");

        // change the timezone to Italy
        setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
        tzset();

        time(&now);
        localtime_r(&now, &timeinfo);
        printf("Actual Italy time:\n");
        strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
        printf("- %s\n", buffer);
        strftime(buffer, sizeof(buffer), "%A, %d %B %Y", &timeinfo);
        printf("- %s\n", buffer);
        strftime(buffer, sizeof(buffer), "Today is day %j of year %Y", &timeinfo);
        printf("- %s\n", buffer);
        printf("\n");
        strftime(buffer, sizeof(buffer), "%c", &timeinfo);
        ESP_LOGI(TAG_HTTPD, "The current date/time in Italy is: %s", buffer);
    } while(0); */
    
    static httpd_handle_t server = NULL;

    //ESP_ERROR_CHECK(nvs_flash_init());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    //ESP_ERROR_CHECK(example_connect());
    wifi_init_softap();
    /* Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
     * and re-start it upon connection.
     */
#ifdef CONFIG_EXAMPLE_CONNECT_WIFI
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));   
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, WIFI_EVENT_AP_STACONNECTED, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &disconnect_handler, &server));   

#endif // CONFIG_EXAMPLE_CONNECT_WIFI
#ifdef CONFIG_EXAMPLE_CONNECT_ETHERNET
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, &server));
#endif // CONFIG_EXAMPLE_CONNECT_ETHERNET

    /* Start the server for the first time */
    server = start_webserver();
}