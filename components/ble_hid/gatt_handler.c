#include "gatt_handler.h"
#include "hid_service.h"

#include "esp_log.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#define GATT_HANDLER_TAG "GAT_HANDLER"

// Many hosts doesn't require this service even though it is mandatory in spec.
// Moreover, they don't like an unofficial vendor id and could possibly cut the
// connection. In such case comment out to undefine this service.
#define SUPPORT_DEVICE_INFO_SERVICE

// GATT HID service UUID
static const ble_uuid16_t gatt_svr_hid_uuid = BLE_UUID16_INIT(0x1812);
// GATT Battery service UUID
static const ble_uuid16_t gatt_svr_battery_uuid = BLE_UUID16_INIT(0x180f);

// GATT Characteristic type UUIDs
static const ble_uuid16_t gatt_characteristic_report_map =
    BLE_UUID16_INIT(0x2A4B);
static const ble_uuid16_t gatt_characteristic_report = BLE_UUID16_INIT(0x2A4D);
static const ble_uuid16_t gatt_characteristic_boot_mouse_report =
    BLE_UUID16_INIT(0x2A33);
static const ble_uuid16_t gatt_characteristic_report_descriptor =
    BLE_UUID16_INIT(0x2908);
static const ble_uuid16_t gatt_characteristic_hid_information =
    BLE_UUID16_INIT(0x2A4A);
static const ble_uuid16_t gatt_characteristic_hid_control_point =
    BLE_UUID16_INIT(0x2A4C);
static const ble_uuid16_t gatt_characteristic_battery_level =
    BLE_UUID16_INIT(0x2A19);

#ifdef SUPPORT_DEVICE_INFO_SERVICE
// GATT Device information service UUID
static const ble_uuid16_t gatt_svr_device_info_uuid = BLE_UUID16_INIT(0x180a);
static const ble_uuid16_t gatt_characteristic_pnp_id = BLE_UUID16_INIT(0x2A50);
#endif

int none_cb(uint16_t conn_handle, uint16_t attr_handle,
            struct ble_gatt_access_ctxt *ctxt, void *arg) {
    return 0;
}

/* Service types are defined in HOGP spec.
// 1+ HID service
// 1+ Battery service
// 1 Device information service
// (and optional services)
*/

// These must be in included service if refered from report map according to
// HOGP.
//
// static const struct ble_gatt_svc_def included_svcs[] = {
//     {/*** Service: Battery */
//      .type = BLE_GATT_SVC_TYPE_PRIMARY,
//      .uuid = &gatt_svr_battery_uuid.u,
//      .characteristics =
//          (struct ble_gatt_chr_def[]){
//              {
//                  /* Characteristic: Battery Level */
//                  .uuid = &gatt_characteristic_battery_level.u,
//                  .access_cb = battery_level_cb,
//                  .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC,
//              },
//              {0}}},
// #ifdef SUPPORT_DEVICE_INFO_SERVICE
//     {/*** Service: Device Information */
//      .type = BLE_GATT_SVC_TYPE_PRIMARY,
//      .uuid = &gatt_svr_device_info_uuid.u,
//      .characteristics =
//          (struct ble_gatt_chr_def[]){
//              {/* Characteristic: PnP ID */
//               /* Only this is mandatory in HOGP */
//               .uuid = &gatt_characteristic_pnp_id.u,
//               .access_cb = none_cb,
//               .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC},
//              {0}}},
// #endif
//     {0},
// };

// const struct ble_gatt_svc_def *included_svcs_list[] = {
//     &included_svcs[0],
//     NULL
// };

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /*** Service: HID */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_hid_uuid.u,
        // .includes = included_svcs_list,
        // HIDS defines following characteristics
        // Report map
        // Report (or boot mouse/keyboard specific equivalent for reports)
        // HID Information
        // HID Control point
        // (Optional, Protocol mode)
        .characteristics =
            (struct ble_gatt_chr_def[]){
                {
                    /* Characteristic: Report map */
                    .uuid = &gatt_characteristic_report_map.u,
                    .access_cb = report_map_cb,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC,
                },
                {/* Characteristic: Report */
                 .uuid = &gatt_characteristic_report.u,
                 .access_cb = report_cb,
                 .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC |
                          BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_INDICATE,
                 .val_handle = &report_handle,
                 .descriptors =
                     (struct ble_gatt_dsc_def[]){
                         {
                             .uuid = &gatt_characteristic_report_descriptor.u,
                             .att_flags = BLE_ATT_F_READ,
                             .access_cb = report_descriptor_cb,
                             .min_key_size = 0,
                         },
                         {
                             0 /* No more descriptors */
                         }}
                },
                {
                    /* Characteristic: Boot Mouse Report */
                    .uuid = &gatt_characteristic_boot_mouse_report.u,
                    .access_cb = report_cb,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC |
                             BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC,
                },
                {
                    /* Characteristic: HID Information */
                    .uuid = &gatt_characteristic_hid_information.u,
                    .access_cb = hid_information_cb,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC,
                },
                {/* Characteristic: HID Control point */
                 .uuid = &gatt_characteristic_hid_control_point.u,
                 // Ignores host suspend events.
                 .access_cb = none_cb,
                 .flags =
                     BLE_GATT_CHR_F_WRITE_NO_RSP | BLE_GATT_CHR_F_WRITE_ENC},
                {
                    0, /* No more characteristics in this service. */
                }},
    },
    {/*** Service: Battery */
     .type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = &gatt_svr_battery_uuid.u,
     .characteristics =
         (struct ble_gatt_chr_def[]){
             {
                 /* Characteristic: Battery Level */
                 .uuid = &gatt_characteristic_battery_level.u,
                 .access_cb = battery_level_cb,
                 .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC,
             },
             {0}}},
#ifdef SUPPORT_DEVICE_INFO_SERVICE
    {/*** Service: Device Information */
     .type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = &gatt_svr_device_info_uuid.u,
     .characteristics =
         (struct ble_gatt_chr_def[]){
             {
                 /* Characteristic: PnP ID */
                 /* Only this is mandatory in HOGP */
                 .uuid = &gatt_characteristic_pnp_id.u,
                 .access_cb = none_cb,
                 .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC,
             },
             {0}}},
#endif
    {
        0, /* No more services. */
    },
};

/**
 * @brief Copied from example. Just logs the gatt registration.
 *
 */
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    char buf[BLE_UUID_STR_LEN];
    hid_control_t *hid_control = (hid_control_t *)arg;

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        MODLOG_DFLT(DEBUG, "registered service %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                    ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        if (ble_uuid_cmp(ctxt->chr.chr_def->uuid,
                         &gatt_characteristic_report.u) == 0) {
            ESP_LOGD(GATT_HANDLER_TAG, "Registered report characteristic");
        }
        MODLOG_DFLT(DEBUG,
                    "registering characteristic %s with "
                    "def_handle=%d val_handle=%d\n",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        MODLOG_DFLT(DEBUG, "registering descriptor %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

/**
 * @brief start GAP/GATT server
 *
 * @return esp_err_t
 */
esp_err_t init_gatts_server(void) {
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}