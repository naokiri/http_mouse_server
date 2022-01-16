#ifndef GATT_HANDLER_H
#define GATT_HANDLER_H

#include "esp_err.h"
#include "host/ble_gatt.h"

void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
esp_err_t init_gatts_server(void);

uint16_t report_handle;

#endif // GATT_HANDLER_H