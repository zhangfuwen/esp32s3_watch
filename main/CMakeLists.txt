# Main component CMakeLists.txt
idf_component_register(
    SRCS 
        "main.c"
        "display.c"
        "display_test.c"
        "simple_test.c"
        "watch_face.c"
        "input.c"
        "wifi.c"
        "bluetooth.c"
        # UI Test screens
        "ui/test_menu.c"
        "ui/test_display.c"
        "ui/test_imu.c"
        "ui/test_battery.c"
        "ui/watch_face_ui.c"
        "ui/watch_face.c"
        "ui/watch_face_chinese.c"
        "ui/voice_recorder.c"
        "ui/hw_tests.c"
        "drivers/i2s_audio.c"
        # Fonts
        "fonts/chinese_16.c"
        # LVGL Port
        "lvgl_port.c"
        "lvgl_test.c"
        # Core modules
        "core/event_bus.c"
        "core/power_manager.c"
        # Drivers
        "drivers/display_driver.c"
        "drivers/imu_driver.c"
        "drivers/touch_driver.c"
        "drivers/battery_driver.c"
        "drivers/cst816_touch.c"
        # Services
        "services/time_service.c"
        "services/motion_detect.c"
        "services/ble_notify.c"
        "services/time_update.c"
        "services/imu_task.c"
    INCLUDE_DIRS 
        "../include"
        "ui"
        "core"
        "drivers"
        "services"
        "fonts"
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
        esp_lcd
)
