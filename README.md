# mouse_server
Make esp32 work as bluetooth mouse, and controllable via http GET

Tested on esp-idf v4.3

# Disclaimer
This is not a product level code that ignores some Bluetooth security features intentionally.

For PnP ID, this component uses a famous private prototyping value as the USB vendor/product ID. You may have to undefine the service (yes, it works for most cases) or register your own ID before reusing this code.

# License 
Licensed under Apache License 2.0

# Build
Setup esp-idf

Configure sdkconfig, enable Bluetooth, enable NimBLE stack.

Run "idf.py build"


# References
mouse 

naokiri/esp32_mouse_ble (https://github.com/naokiri/esp32_mouse_ble)