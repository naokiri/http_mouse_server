/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "ble_hid_component.h"
#include "driver/uart.h"
#include "esp_eth.h"
#include "esp_netif.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "webserver.h"
#include "wifi_initializer.h"
#include <esp_event.h>
#include <esp_wifi.h>
#include <stdio.h>

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include <esp_log.h>

#define CONSOLE_UART_NUM 0
#define UART_TAG "UART"
#define MAIN_TAG "MAIN"
#define SERVER_TASK_TAG "Server_task"


hid_control_t control;

static TaskHandle_t xTaskToNotify;
static QueueHandle_t http_mouse_queue;

void uart_console_task(void *pvParameters) {
    char character;
    // Install UART driver, and get the queue.
    uart_driver_install(CONSOLE_UART_NUM, UART_FIFO_LEN * 2, UART_FIFO_LEN * 2,
                        0, NULL, 0);

    ESP_LOGI(UART_TAG, "console UART processing task started");

    while (1) {
        // read single byte
        uart_read_bytes(CONSOLE_UART_NUM, (uint8_t *)&character, 1,
                        portMAX_DELAY);

        uint8_t button = 0;
        int8_t x = 0, y = 0, wheel = 0;
        switch (character) {
        case 'a':
            x = -20;
            ESP_LOGI(UART_TAG, "mouse: a");
            break;
        case 's':
            y = 20;
            ESP_LOGI(UART_TAG, "mouse: s");
            break;
        case 'd':
            x = 20;
            ESP_LOGI(UART_TAG, "mouse: d");
            break;
        case 'w':
            y = -20;
            ESP_LOGI(UART_TAG, "mouse: w");
            break;
        default:
            ESP_LOGI(UART_TAG, "received: %d, no HID action", character);
            break;
        }
        if (control.is_notifiable || control.is_indicatable) {
            send_mouse_event(&control, button, x, y, wheel);
        }
    }
}

void webserver_command_task(void *pvParameters) {
    QueueHandle_t *queue = (QueueHandle_t *)pvParameters;
    mouse_notification_t mouse_ev;

    while (1) {
        if (xQueueReceive(*queue, &mouse_ev, (TickType_t)10) == pdPASS) {
            if (control.is_notifiable || control.is_indicatable) {
                ESP_LOGD(SERVER_TASK_TAG, "move %d, %d", mouse_ev.x, mouse_ev.y);
                send_mouse_event(&control, mouse_ev.button, mouse_ev.x,
                                 mouse_ev.y, 0);
            }
        }
    }
}

void app_main(void) {
    printf("Hello world!\n");
    memset(&control, 0, sizeof control);

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s, ", CONFIG_IDF_TARGET,
           chip_info.cores, (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded"
                                                         : "external");

    printf("Minimum free heap size: %d bytes\n",
           esp_get_minimum_free_heap_size());

    fflush(stdout);

    init_ble_hid(&control);
    xTaskCreate(&uart_console_task, "uart_console_task", 4096, NULL, 10, NULL);

    // Relies on btle side nvs init, no nvs init code here.
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    xTaskToNotify = xTaskGetCurrentTaskHandle();
    xTaskCreate(&init_wifi_task, "wifi_initializer", 5000, &xTaskToNotify, 1,
                NULL);
    vTaskDelay(1);
    uint32_t ret = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(30000));
    if (ret != 1) {
        ESP_LOGW(MAIN_TAG, "Wifi connection failed. Abort for this time");
    } else {
        ESP_LOGD(MAIN_TAG, "Wifi Success");
    }

    http_mouse_queue = xQueueCreate(10, sizeof(mouse_notification_t *));
    start_webserver();
    xTaskCreate(&webserver_command_task, "webserver_command", 5000,
                &http_mouse_queue, 1, NULL);
    register_mouse_notification_queue(http_mouse_queue);
}
