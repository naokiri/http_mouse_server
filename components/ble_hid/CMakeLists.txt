idf_component_register(SRCS "ble_hid_component.c" "gap_handler.c" "gatt_handler.c" "misc.c" "hid_service.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "bt")
