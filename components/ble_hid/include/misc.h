#include "host/ble_gap.h"

void print_bytes(const uint8_t *bytes, int len);
void print_addr(const void *addr);
void bleprph_print_conn_desc(struct ble_gap_conn_desc *desc);