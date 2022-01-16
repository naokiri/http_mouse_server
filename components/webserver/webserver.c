#include "webserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"

#define WEB_SERVER_TAG "webserver"

QueueHandle_t notificationQueue = NULL;

void register_mouse_notification_queue(QueueHandle_t theHandle) {
    notificationQueue = theHandle;
}

/* Our URI handler function to be called during GET /uri request */
esp_err_t get_handler(httpd_req_t *req) {
    char *buf;
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGD(WEB_SERVER_TAG, "Found URL query => %s", buf);
            char param[8];
            mouse_notification_t mouse_ev;
            memset(&mouse_ev, 0, sizeof(mouse_ev));

            // ESP_ERR_HTTPD_RESULT_TRUNC or NOT_FOUND will just fall back to
            // default
            if (httpd_query_key_value(buf, "x", param, sizeof(param)) ==
                ESP_OK) {
                ESP_LOGD(WEB_SERVER_TAG, "x => %s", param);
                mouse_ev.x = atoi(param);
            }
            if (httpd_query_key_value(buf, "y", param, sizeof(param)) ==
                ESP_OK) {
                ESP_LOGD(WEB_SERVER_TAG, "y => %s", param);
                mouse_ev.y = atoi(param);
            }
            if (httpd_query_key_value(buf, "click", param, sizeof(param)) ==
                ESP_OK) {
                ESP_LOGD(WEB_SERVER_TAG, "click => %s", param);
                if (strcmp(param, "true") == 0) {
                    mouse_ev.button = 0x01;
                }
            }

            if (notificationQueue != NULL) {
                xQueueSend(notificationQueue, &mouse_ev, 0);
            }
        }
        free(buf);
    }

    /* Send a simple response */
    const char resp[] = "URI GET Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* URI handler structure for GET /uri */
httpd_uri_t uri_get = {.uri = "/mouse",
                       .method = HTTP_GET,
                       .handler = get_handler,
                       .user_ctx = NULL};
                       
/* Function for starting the webserver */
httpd_handle_t start_webserver(void) {
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the httpd server */
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_get);
        // httpd_register_uri_handler(server, &uri_post);
    }
    /* If server failed to start, handle will be NULL */
    return server;
}

/* Function for stopping the webserver */
void stop_webserver(httpd_handle_t server) {
    if (server) {
        /* Stop the httpd server */
        httpd_stop(server);
    }
}
