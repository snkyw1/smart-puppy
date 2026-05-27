set(SOURCES "main.c"
            "wifi_manager/wifi_manager.c"
            "servo/servo.c"
            "servo/servo_move.c"
            "bat_monitor/bat_monitor.c"
            "accel/accel.c"
            "accel/sc7a20h/sc7a20h.c"
            "low_power/low_power.c"
            "lcd/lcd.c"
            "lcd/ui/ui.c"
            "lcd/font/font_awesome_16.c"
            "lcd/font/font_awesome_18.c"
            "lcd/font/font_awesome_24.c"
            "lcd/font/font_awesome_64.c"
            "lcd/font/han_sans_cn_medium_16_header.c"
            "lcd/style/my_style.c"
            "lcd/key/key.c"
            "lcd/apps/chat.c"
            "lcd/apps/iot.c"
            "lcd/apps/servo_ctrl.c"
            "lcd/apps/music.c"
            "lcd/apps/recorder.c"
            "lcd/apps/settings.c"
            "lcd/apps/wifi.c"
            "espnow/espnow_mgr.c"
            "ali_iot/ali_iot.c"
            )

if(CONFIG_SERVO_DEBUG)
    list(APPEND SOURCES "servo/servo_debug.c")
endif()

set(INCLUDE_DIRS "." "lcd" "bat_monitor" "accel" "servo" "wifi_manager" "low_power" "espnow" "ali_iot")

set(MY_REQUIRES esp_driver_gpio 
                esp_driver_i2c
                esp_driver_ledc
                esp_wifi
                nvs_flash
                esp_partition
                xiaozhi_esp32
                mqtt
                json
                )

set(ALI_IOT_CA_FILE "ali_iot/ali_iot_ca.crt")

idf_component_register(
    SRCS ${SOURCES}
    EMBED_FILES ${ALI_IOT_CA_FILE}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    PRIV_REQUIRES ${MY_REQUIRES}
)

spiffs_create_partition_assets(
    image
    "lcd/gif"
    FLASH_IN_PROJECT
    MMAP_FILE_SUPPORT_FORMAT ".gif"
)

set(FONT_BIN "${CMAKE_CURRENT_SOURCE_DIR}/lcd/font/han_sans_cn_medium_16.bin")
esptool_py_flash_to_partition(flash "font" ${FONT_BIN})
