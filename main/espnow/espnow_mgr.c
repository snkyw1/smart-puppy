#include "espnow_mgr.h"

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include <string.h>
#include <stdint.h>

#define TAG "espnow_mgr"

static espnow_recv_cb_t s_recv_cb = NULL;
static bool s_initialized = false;

static uint8_t s_peer_mac[ESP_NOW_ETH_ALEN];

static int hex_to_byte(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static esp_err_t parse_mac_string(const char* mac_str, uint8_t* mac_out)
{
    int byte_idx = 0;
    int nibble = 0;
    int val = 0;

    for (size_t i = 0; mac_str[i] != '\0'; i++) {
        if (mac_str[i] == ':' || mac_str[i] == '-') {
            if (nibble != 0) return ESP_ERR_INVALID_ARG;
            continue;
        }
        int h = hex_to_byte(mac_str[i]);
        if (h < 0) return ESP_ERR_INVALID_ARG;
        if (nibble == 0) {
            val = h << 4;
            nibble = 1;
        } else {
            val |= h;
            if (byte_idx >= ESP_NOW_ETH_ALEN) return ESP_ERR_INVALID_ARG;
            mac_out[byte_idx++] = (uint8_t)val;
            nibble = 0;
        }
    }
    if (byte_idx != ESP_NOW_ETH_ALEN || nibble != 0) {
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

static void espnow_send_cb(const esp_now_send_info_t* send_info, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGI(TAG, "发送成功 -> " MACSTR, MAC2STR(send_info->des_addr));
    } else {
        ESP_LOGW(TAG, "发送失败 -> " MACSTR, MAC2STR(send_info->des_addr));
    }
}

static void espnow_recv_cb(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len)
{
    if (len == sizeof(espnow_led_cmd_t)) {
        espnow_led_cmd_t* cmd = (espnow_led_cmd_t*)data;
        ESP_LOGI(TAG, "收到来自 " MACSTR " 的指令: cmd=%s, value=%d",
                 MAC2STR(recv_info->src_addr), cmd->cmd, cmd->value);
        if (s_recv_cb) {
            s_recv_cb(cmd);
        }
    } else {
        ESP_LOGW(TAG, "收到无效数据长度: %d", len);
    }
}

esp_err_t espnow_mgr_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "ESP-NOW 已初始化");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "初始化 ESP-NOW");

    wifi_mode_t mode;
    esp_err_t ret = esp_wifi_get_mode(&mode);
    if (ret == ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGE(TAG, "WiFi 未初始化，请先开启 WiFi");
        return ret;
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "获取 WiFi 模式失败: %s", esp_err_to_name(ret));
        return ret;
    }
    if (mode != WIFI_MODE_STA && mode != WIFI_MODE_APSTA) {
        ESP_LOGE(TAG, "WiFi 未处于 STA 模式(当前 mode=%d)", mode);
        return ESP_ERR_INVALID_STATE;
    }

    const char* mac_str = CONFIG_IOT_ESPNOW_PEER_MAC;
    ret = parse_mac_string(mac_str, s_peer_mac);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MAC 地址解析失败: %s", mac_str);
        return ret;
    }
    ESP_LOGI(TAG, "对端 MAC: " MACSTR, MAC2STR(s_peer_mac));

    uint8_t channel;
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        channel = ap_info.primary;
        ESP_LOGI(TAG, "已连接 AP, 使用当前信道: %d", channel);
    } else {
        channel = CONFIG_IOT_ESPNOW_CHANNEL;
        ESP_LOGI(TAG, "未连接 AP, 使用配置信道: %d", channel);
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    }

    ret = esp_now_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_now_init 失败: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_now_peer_info_t peer_info = {
        .channel = channel,
        .ifidx = WIFI_IF_STA,
        .encrypt = false,
    };
    memcpy(peer_info.peer_addr, s_peer_mac, ESP_NOW_ETH_ALEN);

    bool peer_exists = esp_now_is_peer_exist(s_peer_mac);
    if (!peer_exists) {
        ret = esp_now_add_peer(&peer_info);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "添加对端失败: %s", esp_err_to_name(ret));
            esp_now_deinit();
            return ret;
        }
    }

    esp_now_register_send_cb(espnow_send_cb);
    esp_now_register_recv_cb(espnow_recv_cb);

    s_initialized = true;
    ESP_LOGI(TAG, "ESP-NOW 初始化完成");

    return ESP_OK;
}

esp_err_t espnow_mgr_send_led(uint8_t value)
{
    if (!s_initialized) {
        ESP_LOGE(TAG, "ESP-NOW 未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    espnow_led_cmd_t cmd = {
        .cmd = "led",
        .value = value,
    };

    esp_err_t ret = esp_now_send(s_peer_mac, (uint8_t*)&cmd, sizeof(cmd));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "发送 LED(%d) 指令失败: %s", value, esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "发送 LED(%d) 指令", value);
    }

    return ret;
}

void espnow_mgr_set_recv_cb(espnow_recv_cb_t cb)
{
    s_recv_cb = cb;
}

void espnow_mgr_deinit(void)
{
    if (!s_initialized) return;

    esp_now_unregister_send_cb();
    esp_now_unregister_recv_cb();
    esp_now_del_peer(s_peer_mac);
    esp_now_deinit();
    s_initialized = false;
    s_recv_cb = NULL;
    ESP_LOGI(TAG, "ESP-NOW 已反初始化");
}
