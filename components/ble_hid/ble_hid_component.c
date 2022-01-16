#include "ble_hid_component.h"
#include "gap_handler.h"
#include "gatt_handler.h"
#include "hid_service.h"
#include "misc.h"
#include <stdio.h>

#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "nvs_flash.h"

#define BLE_HID_TAG "BLE_HID"


static void callback_on_reset(int reason) {
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

static bool sync_done = false;

/**
 * @brief Startup or reset of host controller
 *
 */
static void callback_on_sync(void) {
    int rc;

    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Printing ADDR */
    uint8_t addr_val[6] = {0};
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);

    MODLOG_DFLT(INFO, "Device Address: ");
    print_addr(addr_val);
    MODLOG_DFLT(INFO, "\n");
    sync_done = true;
}

void bleprph_host_task(void *param) {
    ESP_LOGD(BLE_HID_TAG, "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();

    nimble_port_freertos_deinit();
}

void init_ble_hid(hid_control_t *hid_control) {
    // Initialize NVS
    // It is said to be required to store PHY calibration data.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_nimble_hci_and_controller_init());
    nimble_port_init();

    ble_hs_cfg.reset_cb = callback_on_reset;
    ble_hs_cfg.sync_cb = callback_on_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.gatts_register_arg = hid_control;
    // Using round robin because it's not actual commercial product :)
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    // If using security
    // ble_hs_cfg.sm_sc = 1;

    ESP_LOGD(BLE_HID_TAG, "Start gatt server");
    ret = init_gatts_server();
    ESP_ERROR_CHECK(ret);

    nimble_port_freertos_init(bleprph_host_task);

    init_hid_control();
    while (sync_done == false) {
        vTaskDelay(100);
        ESP_LOGD(BLE_HID_TAG, "Waiting for sync.");
    }

    /* Begin advertising. */
    begin_advertise(hid_control);
    // ble_gap_deinit();
}

void init_hid_control() { init_hid_control_internal(); }

int send_mouse_event(hid_control_t *hid_control, uint8_t mouse_button,
                     int8_t mickeys_x, int8_t mickeys_y, int8_t wheel) {
    return send_mouse_event_internal(hid_control, mouse_button, mickeys_x,
                                     mickeys_y, wheel);
}