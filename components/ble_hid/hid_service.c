#include "hid_service.h"
#include "esp_log.h"
#include "gatt_handler.h"
#include "host/ble_att.h"
#include "host/ble_hs.h"
#include <stdint.h>

#define HID_TAG "hidservice"

#define MOUSE_REPORT_ID 0x01

// HID service and some HOGP requested services' impl

// static struct mouse_report_t {
//     uint8_t button;
//     int8_t x;
//     int8_t y;
//     int8_t wheel;
// } mouse_report;

uint8_t mouse_report_buffer[4];

// HID Report Map characteristic value
static const uint8_t hidReportMap[] = {
    0x05, 0x01,            // Usage Page (Generic Desktop)
    0x09, 0x02,            // Usage (Mouse)
    0xA1, 0x01,            // Collection (Application)
    0x85, MOUSE_REPORT_ID,  // Report Id (1)
    0x09, 0x01,            //   Usage (Pointer)
    0xA1, 0x00,            //   Collection (Physical)
    0x05, 0x09,            //     Usage Page (Buttons)
    0x19, 0x01,            //     Usage Minimum (01) - Button 1
    0x29, 0x03,            //     Usage Maximum (03) - Button 3
    0x15, 0x00,            //     Logical Minimum (0)
    0x25, 0x01,            //     Logical Maximum (1)
    0x75, 0x01,            //     Report Size (1)
    0x95, 0x03,            //     Report Count (3)
    0x81, 0x02,            //     Input (Data, Variable, Absolute)
    0x75, 0x05,            //     Report Size (5)
    0x95, 0x01,            //     Report Count (1)
    0x81, 0x01,            //     Input (Constant) - Padding
    0x05, 0x01,            //     Usage Page (Generic Desktop)
    0x09, 0x30,            //     Usage (X)
    0x09, 0x31,            //     Usage (Y)
    0x09, 0x38,            //     Usage (Wheel)
    0x15, 0x81,            //     Logical Minimum (-127)
    0x25, 0x7F,            //     Logical Maximum (127)
    0x75, 0x08,            //     Report Size (8)
    0x95, 0x03,            //     Report Count (3)
    0x81, 0x06,            //     Input (Data, Variable, Relative)
    0xC0,                  //   End Collection
    0xC0,                  // End Collection
};

/**
 * @brief Response of report map characteristic
 */
int report_map_cb(uint16_t conn_handle, uint16_t attr_handle,
                  struct ble_gatt_access_ctxt *ctxt, void *arg) {
    // Report map should be a read only characteristic
    assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR);
    uint16_t uuid16 = ble_uuid_u16(ctxt->chr->uuid);
    ESP_LOGI(HID_TAG, "UUID 0x%04X attr 0x%04X arg %d op %d", uuid16,
             attr_handle, (int)arg, ctxt->op);
    ESP_LOGI(HID_TAG, "Report map read");

    int rc = os_mbuf_append(ctxt->om, &hidReportMap, sizeof hidReportMap);
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

int report_cb(uint16_t conn_handle, uint16_t attr_handle,
              struct ble_gatt_access_ctxt *ctxt, void *arg) {
    // This is also used for boot mouse report.
    // Somehow in the HIDS spec, it has to support WRITE property.
    // But doing same as read seems to work.
    uint16_t uuid16 = ble_uuid_u16(ctxt->chr->uuid);
    ESP_LOGD(HID_TAG, "UUID 0x%04X attr 0x%04X arg %d op %d", uuid16,
             attr_handle, (int)arg, ctxt->op);
    for (int i = 0; i < 4; i++) {
        ESP_LOGD(HID_TAG, "M: 0x%02x", mouse_report_buffer[i]);
    }
    int rc = os_mbuf_append(ctxt->om, &mouse_report_buffer,
                            sizeof mouse_report_buffer);
    ESP_LOGD(HID_TAG, "Report event done with result code: %d", rc);

    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static const uint8_t reportDescriptor[] = {
    MOUSE_REPORT_ID, 0x01 // report type Input
};

int report_descriptor_cb(uint16_t conn_handle, uint16_t attr_handle,
                         struct ble_gatt_access_ctxt *ctxt, void *arg) {
    ESP_LOGI(HID_TAG, "Report descriptor read");
    int rc =
        os_mbuf_append(ctxt->om, &reportDescriptor, sizeof reportDescriptor);

    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

/**
 * @brief HID Information Charasteristic from HIDS Spec
 *
 */
struct hid_information_t {
    uint16_t bcdHID;
    uint8_t bCountryCode;
    uint8_t flags;
};

int hid_information_cb(uint16_t conn_handle, uint16_t attr_handle,
                       struct ble_gatt_access_ctxt *ctxt, void *arg) {
    assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR);
    struct hid_information_t hid_information;
    hid_information.bcdHID = 0x1101;     // HIDS v1.11, little endian
    hid_information.bCountryCode = 0x00; // not localized
    hid_information.flags = 0x00;
    int rc = os_mbuf_append(ctxt->om, &hid_information, sizeof hid_information);
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    return 0;
}

int battery_level_cb(uint16_t conn_handle, uint16_t attr_handle,
                     struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        uint8_t level = 77;
        int rc = os_mbuf_append(ctxt->om, &level, sizeof level);
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
}

struct pnp_id_t {
    uint16_t product_version;
    uint16_t product_id;
    uint16_t vendor_id;
    uint8_t vendor_id_source;
};

int pnp_id_cb(uint16_t conn_handle, uint16_t attr_handle,
              struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        struct pnp_id_t pnp_id;
        pnp_id.vendor_id_source = 0x02;
        pnp_id.vendor_id = 0x55f0;       // https://f055.io/ litle endian
        pnp_id.product_id = 0x0a00;      // https://f055.io/ little endian
        pnp_id.product_version = 0x0100; // Stands for v0.0.1
        int rc = os_mbuf_append(ctxt->om, &pnp_id, sizeof pnp_id);
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
}

void init_hid_control_internal() {}

int send_mouse_event_internal(hid_control_t *hid_control, uint8_t mouse_button,
                              int8_t mickeys_x, int8_t mickeys_y,
                              int8_t wheel) {
    ESP_LOGD(HID_TAG, "Notify event");
    // mouse_report.button = mouse_button;
    // mouse_report.x = mickeys_x;
    // mouse_report.y = mickeys_y;
    // mouse_report.wheel = wheel;

    mouse_report_buffer[0] = mouse_button; // Buttons
    mouse_report_buffer[1] = mickeys_x;    // X
    mouse_report_buffer[2] = mickeys_y;    // Y
    mouse_report_buffer[3] = wheel;        // Wheel

    int rc = 0;
    // Indicate is prefered because host can make response.
    if (hid_control->is_indicatable) {
        rc = ble_gattc_indicate(hid_control->conn, report_handle);
    } else if (hid_control->is_notifiable) {
        rc = ble_gattc_notify(hid_control->conn, report_handle);
    }

    if (rc) {
        ESP_LOGE(HID_TAG, "Notify Error. Function: %s", __FUNCTION__);
    }
    return rc;
}
