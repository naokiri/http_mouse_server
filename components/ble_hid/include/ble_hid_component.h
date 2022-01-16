#include "nimble/ble.h"

#ifndef BLE_HID_COMPONENT_H
#define BLE_HID_COMPONENT_H

typedef struct {
    bool is_notifiable;
    bool is_indicatable;
    // gap connection handle
    uint16_t conn;   
} hid_control_t;

void init_ble_hid(hid_control_t *control);

void init_hid_control();

int send_mouse_event(hid_control_t *hid_control, uint8_t mouse_button, int8_t mickeys_x, int8_t mickeys_y, int8_t wheel);

#endif // BLE_HID_COMPONENT_H