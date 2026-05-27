#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESPNOW_LED_CMD_LEN 8

typedef struct {
    char cmd[ESPNOW_LED_CMD_LEN];
    uint8_t value;
} __attribute__((packed)) espnow_led_cmd_t;

typedef void (*espnow_recv_cb_t)(const espnow_led_cmd_t* cmd);

esp_err_t espnow_mgr_init(void);

esp_err_t espnow_mgr_send_led(uint8_t value);

void espnow_mgr_set_recv_cb(espnow_recv_cb_t cb);

void espnow_mgr_deinit(void);

#ifdef __cplusplus
}
#endif
