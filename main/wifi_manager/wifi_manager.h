#ifndef SMART_PUPPY_WIFI_MANAGER_H
#define SMART_PUPPY_WIFI_MANAGER_H
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    char ssid[32];
    int8_t rssi; // 信号强度 -100~0 dBm
    bool is_secure; // 是否需要密码
} wifi_network;

void wifi_manager_init(void);
void wifi_manager_scan(void);
void wifi_manager_connect(const char* ssid, const char* pwd);
void wifi_manager_smartconfig(bool enable);
void wifi_manager_get_config_password(const char* ssid, char* const password);
bool wifi_manager_is_started();
bool wifi_manager_is_connected();

#endif //SMART_PUPPY_WIFI_MANAGER_H
