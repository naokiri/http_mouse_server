#include "wifi_initializer.h"
#include "esp_wifi.h"
#include <stdio.h>
// #define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"

static const char *TAG = "WifiInitializer";
static TaskHandle_t *toNotify;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            ESP_LOGD(TAG, "Wifi started. Connecting.");
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGD(TAG, "Wifi connected.");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGD(TAG, "Disconnected. NOT trying again.");
            esp_wifi_stop();
            xTaskNotify(*toNotify, 2, eSetValueWithOverwrite);
            vTaskDelete(NULL);
            break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
        case IP_EVENT_STA_GOT_IP:
            ESP_LOGD(TAG, "Got IP");
            xTaskNotify(*toNotify, 1, eSetValueWithOverwrite);
            // UBaseType_t stacksize = uxTaskGetStackHighWaterMark(NULL);
            // ESP_LOGD(TAG, "stack size: %d", stacksize);
            vTaskDelete(NULL);
            break;
        }
    }
}

/**
 * Initialize wifi and notify.
 * Notify value 1 on success and ready to use wifi.
 * Notify value >1 on any error and can't use wifi.
 * Note that success value is not 0 to distinguish from notify wait timeout.
 * After notification, kills the task by itself.
 */
void init_wifi_task(void *taskHandlerToNotify) {
    toNotify = (TaskHandle_t *)taskHandlerToNotify;

    ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                               &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID,
                                               &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta =
            {
                .ssid = CONFIG_WIFI_SSID,
                .password = CONFIG_WIFI_PASSWORD,
                .threshold.authmode = WIFI_AUTH_WPA2_PSK,
                // PMF: Have to check if my home WiFi has configured to be WPA3
                // certified.
                .pmf_cfg = {.capable = true, .required = false},
            },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // TODO: Add config loading from nvs someday.
    // wifi_config_t wifi_config_cur;
    // ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config_cur));

    // if (strcmp((const char *)wifi_config_cur.sta.ssid, (const char
    // *)wifi_config.sta.ssid) ||
    //     strcmp((const char *)wifi_config_cur.sta.password, (const char
    //     *)wifi_config.sta.password)) { ESP_LOGI(TAG, "Save WIFI config.");
    //     ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    // }

    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    while (1) {
        // Do nothing and let event handler do the job.
        vTaskDelay(100);
    }
    vTaskDelete(NULL);
}
