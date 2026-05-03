cd ~/esp/projects/auto_web
source ~/.bashrc && get_esp32 && idf.py build && idf.py -p /dev/ttyXXXX flash monitor

(/dev/ttyXXXX: reemplazar XXXX por el puerto del ESP32)

el CMakeLists.txt tiene que verse asi:
idf_component_register(
    SRCS "auto_web.c"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        esp_wifi
        esp_netif
        esp_http_server
    PRIV_REQUIRES
        esp_driver_gpio
)