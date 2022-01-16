#include "host/ble_hs.h"
#include "ble_hid_component.h"

// Defined in https://btprodspecificationrefs.blob.core.windows.net/assigned-values/16-bit%20UUID%20Numbers%20Document.pdf
#define GATT_SERVICE_HID 0x1812

static uint8_t own_addr_type;

void begin_advertise(hid_control_t *hid_control);
int gap_handler(struct ble_gap_event *event, void *arg);