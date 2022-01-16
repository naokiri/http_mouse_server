#include "gap_handler.h"
#include "misc.h"

#include "services/gap/ble_svc_gap.h"

#include "esp_log.h"

#define HIDD_DEVICE_NAME "esp32_mouse"

#define BLE_GAP_TAG "BLE_GAP"

/**
 * Enables advertising with the following parameters:
 *     o Limited discoverable mode.
 *     o Undirected connectable mode.
 */
void begin_advertise(hid_control_t *hid_control) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    const char *name;
    int rc;

    /**
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info).
     *     o Advertising tx power.
     *     o Device name.
     *     o 16-bit service UUIDs (alert notifications).
     */

    memset(&fields, 0, sizeof fields);

    /* Advertise two flags:
     *     o Limited discovery mode
     *     o BLE-only (BR/EDR unsupported).
     */
    fields.flags = BLE_HS_ADV_F_DISC_LTD | BLE_HS_ADV_F_BREDR_UNSUP;

    /* Indicate that the TX power level field should be included; have the
     * stack fill this value automatically.  This is done by assigning the
     * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
     */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    // Required on HOGP spec to set name in adv data or scan response data.
    // Including in adv data for here.
    ble_svc_gap_device_name_set(HIDD_DEVICE_NAME);

    name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    // Required on HOGP spec to set appearance in adv data or scan response
    // data.
    fields.appearance =
        (uint16_t)0x03C2; // Mouse Appearance Value from Bluetooth spec
    fields.appearance_is_present = 1;

    fields.uuids16 = (ble_uuid16_t[]){BLE_UUID16_INIT(GATT_SERVICE_HID)};
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error setting advertisement data; rc=%d\n", rc);
        return;
    }

    /* Begin advertising. */
    ESP_LOGI(BLE_GAP_TAG, "Start advertising");
    memset(&adv_params, 0, sizeof adv_params);
    // Undirected, limited discovery mode actually set here matching flags of
    // the advertising packet. Other params from HOGP Connection Establishment
    // section
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_LTD;
    adv_params.itvl_min = BLE_GAP_INITIAL_CONN_ITVL_MIN;
    adv_params.itvl_max = BLE_GAP_INITIAL_CONN_ITVL_MAX;

    int32_t duration_ms = 3 * 60 * 1000; // 3 min
    rc = ble_gap_adv_start(own_addr_type, NULL, duration_ms, &adv_params,
                           gap_handler, hid_control);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error enabling advertisement; rc=%d\n", rc);
        return;
    }
}

/**
 * Copied from example code.
 * Mostly just printing the state and restart advertising.
 * For secure link renewal, it doesn't check the renewed connection. This is not
 * a product level code.
 *
 *
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that forms.
 * bleprph uses the same callback for all connections.
 *
 * @param event                 The type of event being signalled.
 * @param ctxt                  Various information pertaining to the event.
 * @param arg                   Application-specified argument; unuesd by
 *                                  bleprph.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
int gap_handler(struct ble_gap_event *event, void *arg) {
    struct ble_gap_conn_desc desc;
    int rc;
    hid_control_t *hid_control = (hid_control_t *)arg;

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed. */
        MODLOG_DFLT(INFO, "connection %s; status=%d ",
                    event->connect.status == 0 ? "established" : "failed",
                    event->connect.status);
        if (event->connect.status == 0) {
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);
            hid_control->conn = desc.conn_handle;
            bleprph_print_conn_desc(&desc);
        }
        MODLOG_DFLT(INFO, "\n");

        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        // https://mynewt.apache.org/master/network/ble_hs/ble_hs_return_codes.html
        MODLOG_DFLT(INFO, "disconnect; reason=%04X ", event->disconnect.reason);
        bleprph_print_conn_desc(&event->disconnect.conn);
        MODLOG_DFLT(INFO, "\n");
        // Subscribe event also should happen on unsubscribing so not required
        // to reset flags here?
        hid_control->is_indicatable = false;
        hid_control->is_notifiable = false;
        hid_control->conn = 0;
        /* Connection terminated; resume advertising. */
        begin_advertise(hid_control);
        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE:
        /* The central has updated the connection parameters. */
        MODLOG_DFLT(INFO, "connection updated; status=%d ",
                    event->conn_update.status);
        rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
        assert(rc == 0);
        hid_control->conn = event->conn_update.conn_handle;
        bleprph_print_conn_desc(&desc);
        MODLOG_DFLT(INFO, "\n");
        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        MODLOG_DFLT(INFO, "advertise complete; reason=%d",
                    event->adv_complete.reason);
        // Not restarting automatically by commenting out next line, but we may
        // start advertising again after this state.
        // begin_advertise();
        ble_gap_adv_stop();
        return 0;

    case BLE_GAP_EVENT_ENC_CHANGE:
        /* Encryption has been enabled or disabled for this connection. */
        MODLOG_DFLT(INFO, "encryption change event; status=%d ",
                    event->enc_change.status);
        rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
        assert(rc == 0);
        bleprph_print_conn_desc(&desc);
        MODLOG_DFLT(INFO, "\n");
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        hid_control->is_notifiable = event->subscribe.cur_notify;
        hid_control->is_indicatable = event->subscribe.cur_indicate;
        rc = ble_gap_conn_find(event->subscribe.conn_handle, &desc);
        bleprph_print_conn_desc(&desc);
        MODLOG_DFLT(INFO, "\n");
        MODLOG_DFLT(INFO,
                    "subscribe event; conn_handle=%d attr_handle=%d "
                    "reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
                    event->subscribe.conn_handle, event->subscribe.attr_handle,
                    event->subscribe.reason, event->subscribe.prev_notify,
                    event->subscribe.cur_notify, event->subscribe.prev_indicate,
                    event->subscribe.cur_indicate);
        return 0;

    case BLE_GAP_EVENT_NOTIFY_TX:
        MODLOG_DFLT(INFO, "notify event; detail omit");
        return 0;
    case BLE_GAP_EVENT_MTU:
        MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
                    event->mtu.conn_handle, event->mtu.channel_id,
                    event->mtu.value);
        return 0;

    case BLE_GAP_EVENT_REPEAT_PAIRING:
        /* We already have a bond with the peer, but it is attempting to
         * establish a new secure link.  This app sacrifices security for
         * convenience: just throw away the old bond and accept the new link.
         */

        /* Delete the old bond. */
        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        assert(rc == 0);
        ble_store_util_delete_peer(&desc.peer_id_addr);

        /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
         * continue with the pairing operation.
         */
        return BLE_GAP_REPEAT_PAIRING_RETRY;
    }

    return 0;
}
