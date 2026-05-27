# Define source files
set(SOURCES "audio/audio_codec.cc"
            "audio/audio_service.cc"
            "audio/demuxer/ogg_demuxer.cc"
            "audio/codecs/no_audio_codec.cc"
            "audio/processors/audio_debugger.cc"
            "protocols/protocol.cc"
            "protocols/mqtt_protocol.cc"
            "protocols/websocket_protocol.cc"
            "mcp_server.cc"
            "system_info.cc"
            "application.cc"
            "ota.cc"
            "settings.cc"
            "device_state_machine.cc"
            "board.cc"
            )

set(INCLUDE_DIRS "." "audio" "audio/demuxer" "protocols")


if(CONFIG_BOARD_TYPE_SMART_PUPPY)
    set(BOARD_TYPE "smart-puppy")
endif()

# Select audio processor according to Kconfig
if(CONFIG_USE_AUDIO_PROCESSOR)
    list(APPEND SOURCES "audio/processors/afe_audio_processor.cc")
else()
    list(APPEND SOURCES "audio/processors/no_audio_processor.cc")
endif()
if(CONFIG_IDF_TARGET_ESP32S3 OR CONFIG_IDF_TARGET_ESP32P4)
    list(APPEND SOURCES "audio/wake_words/afe_wake_word.cc")
    list(APPEND SOURCES "audio/wake_words/custom_wake_word.cc")
else()
    list(APPEND SOURCES "audio/wake_words/esp_wake_word.cc")
endif()

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS ${INCLUDE_DIRS}
                    PRIV_REQUIRES
                        esp_psram
                        esp_driver_gpio
                        esp_driver_i2s
                        esp_app_format
                        spi_flash
                        efuse
                        nvs_flash
                        esp_wifi
                        app_update
                    )

# Use target_compile_definitions to define BOARD_TYPE, BOARD_NAME
# If BOARD_NAME is empty, use BOARD_TYPE
if(NOT BOARD_NAME)
    set(BOARD_NAME ${BOARD_TYPE})
endif()
target_compile_definitions(${COMPONENT_LIB}
                    PRIVATE BOARD_TYPE=\"${BOARD_TYPE}\" BOARD_NAME=\"${BOARD_NAME}\"
                    )


spiffs_create_partition_assets(
    audio
    "assets/audio"
    FLASH_IN_PROJECT
    MMAP_FILE_SUPPORT_FORMAT ".ogg" ".lrc"
)
