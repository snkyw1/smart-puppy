#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ali_iot_light_cb_t)(int level);
typedef void (*ali_iot_conn_cb_t)(bool connected);

void ali_iot_init(void);

void ali_iot_deinit(void);

bool ali_iot_is_connected(void);

void ali_iot_set_light_cb(ali_iot_light_cb_t cb);

void ali_iot_set_conn_cb(ali_iot_conn_cb_t cb);

#ifdef __cplusplus
}
#endif
