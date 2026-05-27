#include "board.h"
#include "system_info.h"
#include "settings.h"
#include "assets/lang_config.h"
#include <esp_log.h>
#include <esp_chip_info.h>
#include <esp_random.h>
#include <esp_ota_ops.h>
#include "esp_app_desc.h"
#include "codecs/no_audio_codec.h"
#include "esp_network.h"
#include "cJSON.h"
#include "esp_wifi.h"

#define TAG "Board"

Board::Board() {
    Settings settings("board", true);
    uuid_ = settings.GetString("uuid");
    if (uuid_.empty()) {
        uuid_ = GenerateUuid();
        settings.SetString("uuid", uuid_);
    }
    ESP_LOGI(TAG, "UUID=%s SKU=%s", uuid_.c_str(), BOARD_NAME);
}

AudioCodec* Board::GetAudioCodec()
{
    static NoAudioCodecSimplex audio_codec(
        CONFIG_AUDIO_INPUT_SAMPLE_RATE, 
        CONFIG_AUDIO_OUTPUT_SAMPLE_RATE,
        (gpio_num_t)CONFIG_AUDIO_I2S_GPIO_SPK_BCLK,
        (gpio_num_t)CONFIG_AUDIO_I2S_GPIO_SPK_WS, 
        (gpio_num_t)CONFIG_AUDIO_I2S_GPIO_SPK_DIN,
        (gpio_num_t)CONFIG_AUDIO_I2S_GPIO_MIC_BCLK,
        (gpio_num_t)CONFIG_AUDIO_I2S_GPIO_MIC_WS, 
        (gpio_num_t)CONFIG_AUDIO_I2S_GPIO_MIC_DOUT,
        (gpio_num_t)CONFIG_AUDIO_GPIO_PA_CTRL
    );

        return &audio_codec;
}

std::string Board::GenerateUuid() {
    // UUID v4 需要 16 字节的随机数据
    uint8_t uuid[16];
    
    // 使用 ESP32 的硬件随机数生成器
    esp_fill_random(uuid, sizeof(uuid));
    
    // 设置版本 (版本 4) 和变体位
    uuid[6] = (uuid[6] & 0x0F) | 0x40;    // 版本 4
    uuid[8] = (uuid[8] & 0x3F) | 0x80;    // 变体 1
    
    // 将字节转换为标准的 UUID 字符串格式
    char uuid_str[37];
    snprintf(uuid_str, sizeof(uuid_str),
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        uuid[0], uuid[1], uuid[2], uuid[3],
        uuid[4], uuid[5], uuid[6], uuid[7],
        uuid[8], uuid[9], uuid[10], uuid[11],
        uuid[12], uuid[13], uuid[14], uuid[15]);
    
    return std::string(uuid_str);
}


std::string Board::GetSystemInfoJson() {
    /* 
        {
            "version": 2,
            "flash_size": 4194304,
            "psram_size": 0,
            "minimum_free_heap_size": 123456,
            "mac_address": "00:00:00:00:00:00",
            "uuid": "00000000-0000-0000-0000-000000000000",
            "chip_model_name": "esp32s3",
            "chip_info": {
                "model": 1,
                "cores": 2,
                "revision": 0,
                "features": 0
            },
            "application": {
                "name": "my-app",
                "version": "1.0.0",
                "compile_time": "2021-01-01T00:00:00Z"
                "idf_version": "4.2-dev"
                "elf_sha256": ""
            },
            "partition_table": [
                "app": {
                    "label": "app",
                    "type": 1,
                    "subtype": 2,
                    "address": 0x10000,
                    "size": 0x100000
                }
            ],
            "ota": {
                "label": "ota_0"
            },
            "board": {
                ...
            }
        }
    */
    std::string json = R"({"version":2,"language":")" + std::string(Lang::CODE) + R"(",)";
    json += R"("flash_size":)" + std::to_string(SystemInfo::GetFlashSize()) + R"(,)";
    json += R"("minimum_free_heap_size":")" + std::to_string(SystemInfo::GetMinimumFreeHeapSize()) + R"(",)";
    json += R"("mac_address":")" + SystemInfo::GetMacAddress() + R"(",)";
    json += R"("uuid":")" + uuid_ + R"(",)";
    json += R"("chip_model_name":")" + SystemInfo::GetChipModelName() + R"(",)";

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    json += R"("chip_info":{)";
    json += R"("model":)" + std::to_string(chip_info.model) + R"(,)";
    json += R"("cores":)" + std::to_string(chip_info.cores) + R"(,)";
    json += R"("revision":)" + std::to_string(chip_info.revision) + R"(,)";
    json += R"("features":)" + std::to_string(chip_info.features) + R"(},)";

    auto app_desc = esp_app_get_description();
    json += R"("application":{)";
    json += R"("name":")" + std::string(app_desc->project_name) + R"(",)";
    json += R"("version":")" + std::string(app_desc->version) + R"(",)";
    json += R"("compile_time":")" + std::string(app_desc->date) + R"(T)" + std::string(app_desc->time) + R"(Z",)";
    json += R"("idf_version":")" + std::string(app_desc->idf_ver) + R"(",)";
    char sha256_str[65];
    for (int i = 0; i < 32; i++) {
        snprintf(sha256_str + i * 2, sizeof(sha256_str) - i * 2, "%02x", app_desc->app_elf_sha256[i]);
    }
    json += R"("elf_sha256":")" + std::string(sha256_str) + R"(")";
    json += R"(},)";

    json += R"("partition_table": [)";
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
    while (it) {
        const esp_partition_t *partition = esp_partition_get(it);
        json += R"({)";
        json += R"("label":")" + std::string(partition->label) + R"(",)";
        json += R"("type":)" + std::to_string(partition->type) + R"(,)";
        json += R"("subtype":)" + std::to_string(partition->subtype) + R"(,)";
        json += R"("address":)" + std::to_string(partition->address) + R"(,)";
        json += R"("size":)" + std::to_string(partition->size) + R"(},)";;
        it = esp_partition_next(it);
    }
    json.pop_back(); // Remove the last comma
    json += R"(],)";

    json += R"("ota":{)";
    auto ota_partition = esp_ota_get_running_partition();
    json += R"("label":")" + std::string(ota_partition->label) + R"(")";
    json += R"(},)";

    // Append display info

    if (true) {
        json += R"("display":{)";
        json += R"("monochrome":)" + std::string("false") + R"(,)";
        json += R"("width":)" + std::to_string(CONFIG_LCD_WIDTH) + R"(,)";
        json += R"("height":)" + std::to_string(CONFIG_LCD_HEIGHT) + R"(,)";
        json.pop_back(); // Remove the last comma
    }
    json += R"(},)";

    json += R"("board":)" + GetBoardJson();

    // Close the JSON object
    json += R"(})";
    return json;
}

NetworkInterface* Board::GetNetwork() {
    static EspNetwork network;
    return &network;
}


std::string Board::GetBoardJson() {

    std::string json = R"({"type":")" + std::string(BOARD_TYPE) + R"(",)";
    json += R"("name":")" + std::string(BOARD_NAME) + R"(",)";
    wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);

    if (ret == ESP_OK) {
        json += R"("ssid":")" + std::string((char*)ap_info.ssid) + R"(",)";
        json += R"("rssi":)" + std::to_string(ap_info.rssi) + R"(,)";
        json += R"("channel":)" + std::to_string(ap_info.primary) + R"(,)";
        char ip_str[16] = "0.0.0.0";
        esp_netif_ip_info_t ip_info;
        esp_netif_t* netif = esp_netif_get_default_netif();
        if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK)
        {
            esp_ip4addr_ntoa(&ip_info.ip, ip_str, sizeof(ip_str));
        }
        json += R"("ip":")" + std::string(ip_str) + R"(",)";
    }

    json += R"("mac":")" + SystemInfo::GetMacAddress() + R"("})";
    return json;
}

void Board::SetPowerSaveLevel(PowerSaveLevel level) {
    
    wifi_ps_type_t type;
    switch (level) {
        case PowerSaveLevel::LOW_POWER:
            type = WIFI_PS_MAX_MODEM;
            break;
        case PowerSaveLevel::BALANCED:
            type = WIFI_PS_MIN_MODEM;
            break;
        case PowerSaveLevel::PERFORMANCE:
        default:
            type = WIFI_PS_NONE;
            break;
    }
    esp_wifi_set_ps(type);
}

std::string Board::GetDeviceStatusJson() {
    auto& board = Board::GetInstance();
    auto root = cJSON_CreateObject();

    // Audio speaker
    auto audio_speaker = cJSON_CreateObject();
    if (auto codec = board.GetAudioCodec()) {
        cJSON_AddNumberToObject(audio_speaker, "volume", codec->output_volume());
    }
    cJSON_AddItemToObject(root, "audio_speaker", audio_speaker);

    // Screen
    auto screen = cJSON_CreateObject();

    cJSON_AddNumberToObject(screen, "brightness", lcd_get_brightness());
    
    cJSON_AddStringToObject(screen, "theme", "light");

    cJSON_AddItemToObject(root, "screen", screen);


    // Network
    auto network = cJSON_CreateObject();
    cJSON_AddStringToObject(network, "type", "wifi");

    wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    if (ret == ESP_OK) {
        cJSON_AddStringToObject(network, "ssid", (char*)ap_info.ssid);
        int rssi = ap_info.rssi;
        const char* signal = rssi >= -60 ? "strong" : (rssi >= -70 ? "medium" : "weak");
        cJSON_AddStringToObject(network, "signal", signal);
    }
    cJSON_AddItemToObject(root, "network", network);
    

    auto str = cJSON_PrintUnformatted(root);
    std::string result(str);
    cJSON_free(str);
    cJSON_Delete(root);
    return result;
}
