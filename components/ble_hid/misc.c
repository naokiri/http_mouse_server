#include "host/ble_gap.h"
#include "modlog/modlog.h"
#include "nimble/ble.h"
#include <stdint.h>

/**
 * Utility function to log an array of bytes.
 */
void print_bytes(const uint8_t *bytes, int len) {
    int i;

    for (i = 0; i < len; i++) {
        MODLOG_DFLT(INFO, "%s0x%02x", i != 0 ? ":" : "", bytes[i]);
    }
}

void print_addr(const void *addr) {
    const uint8_t *u8p;

    u8p = addr;
    MODLOG_DFLT(INFO, "%02x:%02x:%02x:%02x:%02x:%02x", u8p[5], u8p[4], u8p[3],
                u8p[2], u8p[1], u8p[0]);
}

/**
 * Logs information about a connection to the console.
 */
void bleprph_print_conn_desc(struct ble_gap_conn_desc *desc) {
    MODLOG_DFLT(INFO, "handle=%d our_ota_addr_type=%d our_ota_addr=",
                desc->conn_handle, desc->our_ota_addr.type);
    print_addr(desc->our_ota_addr.val);
    MODLOG_DFLT(INFO,
                " our_id_addr_type=%d our_id_addr=", desc->our_id_addr.type);
    print_addr(desc->our_id_addr.val);
    MODLOG_DFLT(INFO, " peer_ota_addr_type=%d peer_ota_addr=",
                desc->peer_ota_addr.type);
    print_addr(desc->peer_ota_addr.val);
    MODLOG_DFLT(INFO,
                " peer_id_addr_type=%d peer_id_addr=", desc->peer_id_addr.type);
    print_addr(desc->peer_id_addr.val);
    MODLOG_DFLT(INFO,
                " conn_itvl=%d conn_latency=%d supervision_timeout=%d "
                "encrypted=%d authenticated=%d bonded=%d\n",
                desc->conn_itvl, desc->conn_latency, desc->supervision_timeout,
                desc->sec_state.encrypted, desc->sec_state.authenticated,
                desc->sec_state.bonded);
}
