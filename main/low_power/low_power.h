#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void low_power_init(void);
bool low_power_is_sleeping(void);
void low_power_enter(void);
void low_power_exit(void);
void low_power_wake(void);
void low_power_set_wake_cb(void (*cb)(void));

#ifdef __cplusplus
}
#endif
