
#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <esp_http_server.h>

httpd_handle_t start_webserver(void);
void register_mouse_notification_queue(QueueHandle_t theHandle);

typedef struct {
    int8_t x;
    int8_t y;
    uint8_t button;
} mouse_notification_t;

#endif