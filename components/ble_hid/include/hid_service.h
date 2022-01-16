#include "ble_hid_component.h"
#include "host/ble_gatt.h"

int report_map_cb(uint16_t conn_handle, uint16_t attr_handle,
                  struct ble_gatt_access_ctxt *ctxt, void *arg);

int report_cb(uint16_t conn_handle, uint16_t attr_handle,
              struct ble_gatt_access_ctxt *ctxt, void *arg);

int hid_information_cb(uint16_t conn_handle, uint16_t attr_handle,
                       struct ble_gatt_access_ctxt *ctxt, void *arg);

int battery_level_cb(uint16_t conn_handle, uint16_t attr_handle,
                     struct ble_gatt_access_ctxt *ctxt, void *arg);

int report_descriptor_cb(uint16_t conn_handle, uint16_t attr_handle,
                         struct ble_gatt_access_ctxt *ctxt, void *arg);

void init_hid_control_internal();

int send_mouse_event_internal(hid_control_t *hid_control, uint8_t mouse_button,
                               int8_t mickeys_x, int8_t mickeys_y,
                               int8_t wheel);