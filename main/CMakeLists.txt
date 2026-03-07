# Main component CMakeLists.txt
idf_component_register(
    SRCS 
        "main.c"
        "display.c"
        "watch_face.c"
        "input.c"
        "wifi.c"
        "bluetooth.c"
        # Emotional Comfort Features
        "comfort/breathing.c"
        "comfort/messages.c"
        "comfort/emergency.c"
    INCLUDE_DIRS 
        "../include"
        "comfort"
    REQUIRES
        driver
        esp_wifi
        nvs_flash
        lwip
        freertos
        esp_common
        esp_hw_support
        esp_netif
        esp_timer
)
