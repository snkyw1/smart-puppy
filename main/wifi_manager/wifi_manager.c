#include "wifi_manager.h"
#include "esp_wifi.h"
#include <string.h>
#include "esp_log.h"
#include "esp_smartconfig.h"
#include "application.h"
#include "mmap_generate_audio.h"

#define MAX_SCAN_AP_NUM 20

#define TAG "component_wifi"

extern void wifi_connect_cb(bool is_connected, const char* wifi_ssid);
extern void wifi_scan_cb(wifi_network* nets, uint8_t count, const char* connected_ssid);
extern void wifi_smartconfig_cb(bool is_done);

static bool wifi_is_connected = false;
static bool wifi_is_started = false;


static void event_handler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == IP_EVENT)
    {
        if (event_id == IP_EVENT_STA_GOT_IP)
        {
            wifi_ap_record_t ap_info;
            esp_wifi_sta_get_ap_info(&ap_info);
            wifi_connect_cb(true, (char*)ap_info.ssid);
            wifi_is_connected = true;
            ESP_LOGI(TAG, "connected to %s", ap_info.ssid);
            SetMainEventBits(MAIN_EVENT_NETWORK_CONNECTED);
        }
    }
    else if (event_base == WIFI_EVENT)
    {
        if (event_id == WIFI_EVENT_STA_START) {
            wifi_is_started = true;
        }
        else if (event_id == WIFI_EVENT_STA_STOP) {
            wifi_is_started = false;
        }
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*)event_data;
            ESP_LOGI(TAG, "disconnect reason:%d\n", event->reason);
            wifi_connect_cb(false, (char*)event->ssid);
            wifi_is_connected = false;
            SetMainEventBits(MAIN_EVENT_NETWORK_DISCONNECTED);
        }
        else if (event_id == WIFI_EVENT_SCAN_DONE)
        {
            wifi_event_sta_scan_done_t* event = (wifi_event_sta_scan_done_t*)event_data;
            if (event->status == 1) return;

            uint16_t ap_count = event->number;
            // uint16_t ap_count;
            // ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
            if (ap_count > MAX_SCAN_AP_NUM) ap_count = MAX_SCAN_AP_NUM;

            wifi_ap_record_t* ap_info = calloc(ap_count, sizeof(wifi_ap_record_t));
            if (ap_info == NULL)
            {
                ESP_LOGE(TAG, "wifi_ap_record_t malloc error");
                return;
            }
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_info));

            bool* duplicates = calloc(ap_count, sizeof(bool));
            for (uint8_t i = 0; i < ap_count; i++)
            {
                if (duplicates[i]) continue;
                for (uint8_t j = i + 1; j < ap_count; j++)
                {
                    if (strcmp((char*)ap_info[j].ssid, (char*)ap_info[i].ssid) == 0)
                    {
                        duplicates[j] = true;
                    }
                }
            }

            uint8_t real_ap_count = 0;
            for (uint8_t i = 0; i < ap_count; i++)
            {
                if (!duplicates[i])
                {
                    ap_info[real_ap_count++] = ap_info[i]; // 只复制有效项
                }
            }
            free(duplicates);

            wifi_network* nets = calloc(real_ap_count, sizeof(wifi_network));

            for (uint16_t i = 0; i < real_ap_count; i++)
            {
                strncpy(nets[i].ssid, (char*)ap_info[i].ssid, sizeof(nets[i].ssid));
                nets[i].rssi = ap_info[i].rssi;
                nets[i].is_secure = ap_info[i].authmode != WIFI_AUTH_OPEN;
            }

            free(ap_info);

            if (wifi_is_connected)
            {
                wifi_ap_record_t ap_info;
                esp_wifi_sta_get_ap_info(&ap_info);
                wifi_scan_cb(nets, real_ap_count, (char*)ap_info.ssid);
                return;
            }

            wifi_scan_cb(nets, real_ap_count, NULL);
        }
    }
    else if (event_base == SC_EVENT)
    {
        if (event_id == SC_EVENT_GOT_SSID_PSWD)
        {
            smartconfig_event_got_ssid_pswd_t* evt = (smartconfig_event_got_ssid_pswd_t*)event_data;
            wifi_manager_connect((char*)evt->ssid, (char*)evt->password);
            wifi_smartconfig_cb(false);
        }
        else if (event_id == SC_EVENT_SEND_ACK_DONE)
        {
            ESP_ERROR_CHECK(esp_smartconfig_stop());
            wifi_smartconfig_cb(true);
        }
    }
}

bool wifi_manager_is_started()
{
    return wifi_is_started;
}

bool wifi_manager_is_connected()
{
    return wifi_is_connected;
}

void wifi_manager_smartconfig(bool enable)
{
    if (!wifi_is_started) return;
    if (enable)
    {
        ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_V2));
        smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
        SchedulePlaySound(MMAP_AUDIO_WIFICONFIG_OGG);
    }
    else
    {
        ESP_ERROR_CHECK(esp_smartconfig_stop());
    }
}

static void wifi_manager_sta_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &event_handler, NULL);
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_STOP, &event_handler, NULL);
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &event_handler, NULL);
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &event_handler, NULL);
    esp_event_handler_register(SC_EVENT, SC_EVENT_GOT_SSID_PSWD, &event_handler, NULL);
    esp_event_handler_register(SC_EVENT, SC_EVENT_SEND_ACK_DONE, &event_handler, NULL);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_manager_scan(void)
{
    if (!wifi_is_started) return;
    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, false));
}

void wifi_manager_connect(const char* ssid, const char* pwd)
{
    if (!wifi_is_started) return;
    wifi_config_t wifi_config = { 0 };
    memcpy(wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, pwd, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    if (wifi_is_connected)
    {
        ESP_ERROR_CHECK(esp_wifi_disconnect());
    }

    ESP_ERROR_CHECK(esp_wifi_connect());
}

void wifi_manager_get_config_password(const char* ssid, char* const password)
{
    wifi_config_t wifi_config = { 0 };
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));
    if (strncmp(ssid, (const char* )wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid)) == 0) {
        strcpy(password, (const char* )wifi_config.sta.password);
    }
}

void wifi_manager_init(void)
{
    wifi_manager_sta_init();

    wifi_config_t wifi_config = { 0 };
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));

    if (wifi_config.sta.ssid[0] != '\0')
    {
        ESP_ERROR_CHECK(esp_wifi_connect());
    }
    else
        ESP_LOGI(TAG, "NVS no wifi config");
}
