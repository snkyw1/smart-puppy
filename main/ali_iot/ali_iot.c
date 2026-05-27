#include "ali_iot.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "mqtt_client.h"
#include "mbedtls/md.h"
#include "cJSON.h"
#include "sdkconfig.h"

#define TAG "ALI_IOT"

extern const uint8_t ali_iot_ca_crt_start[]   asm("_binary_ali_iot_ca_crt_start");
extern const uint8_t ali_iot_ca_crt_end[]   asm("_binary_ali_iot_ca_crt_end");

static esp_mqtt_client_handle_t mqtt_client = NULL;
static ali_iot_light_cb_t light_cb = NULL;
static ali_iot_conn_cb_t conn_cb = NULL;
static bool connected = false;

static void parse_property_set(const char* data, int data_len)
{
    char* buf = (char*)malloc(data_len + 1);
    if (!buf) return;
    memcpy(buf, data, data_len);
    buf[data_len] = '\0';

    cJSON* root = cJSON_Parse(buf);
    free(buf);
    if (!root) return;

    cJSON* items = cJSON_GetObjectItem(root, "items");
    if (!items) {
        cJSON_Delete(root);
        return;
    }

    cJSON* light_level = cJSON_GetObjectItem(items, "light_level");
    if (!light_level) {
        cJSON_Delete(root);
        return;
    }

    cJSON* value = cJSON_GetObjectItem(light_level, "value");
    if (!value || !cJSON_IsNumber(value)) {
        cJSON_Delete(root);
        return;
    }

    int level = value->valueint;
    ESP_LOGI(TAG, "收到光照等级: %d", level);

    if (light_cb) {
        light_cb(level);
    }

    cJSON_Delete(root);
}

static void mqtt_event_handler(void* handler_args, esp_event_base_t base,
                               int32_t event_id, void* event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED: {
        ESP_LOGI(TAG, "MQTT已连接");
        connected = true;

        if (conn_cb) conn_cb(true);

        char topic[128];
        snprintf(topic, sizeof(topic),
                 "/%s/%s/user/get",
                 CONFIG_ALI_IOT_PRODUCT_KEY, CONFIG_ALI_IOT_DEVICE_NAME);
        int ret = esp_mqtt_client_subscribe(mqtt_client, topic, 0);
        if (ret < 0) {
            ESP_LOGE(TAG, "订阅失败: %s", topic);
        } else {
            ESP_LOGI(TAG, "已订阅: %s", topic);
        }
        break;
    }

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT已断开");
        connected = false;
        if (conn_cb) conn_cb(false);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "收到MQTT消息");
        if (event->data_len > 0 && event->data) {
            parse_property_set(event->data, event->data_len);
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT错误");
        connected = false;
        break;

    default:
        break;
    }
}
static void ali_iot_gen_password(char* password, size_t len)
{
    const char* cid = CONFIG_ALI_IOT_DEVICE_NAME;

    char content[256];
    snprintf(content, sizeof(content),
        "clientId%sdeviceName%sproductKey%s",
        cid, CONFIG_ALI_IOT_DEVICE_NAME, CONFIG_ALI_IOT_PRODUCT_KEY);

    unsigned char hmac[20];
    mbedtls_md_hmac(
        mbedtls_md_info_from_type(MBEDTLS_MD_SHA1),
        (const unsigned char*)CONFIG_ALI_IOT_DEVICE_SECRET,
        strlen(CONFIG_ALI_IOT_DEVICE_SECRET),
        (const unsigned char*)content,
        strlen(content),
        hmac);

    for (int i = 0; i < 20; i++)
    {
        snprintf(password + i * 2, 3, "%02x", hmac[i]);
    }
    if (len > 40)
        password[40] = '\0';
}


void ali_iot_init(void)
{
    if (mqtt_client != NULL) return;

    const char* pk = CONFIG_ALI_IOT_PRODUCT_KEY;
    const char* dn = CONFIG_ALI_IOT_DEVICE_NAME;
    const char* ds = CONFIG_ALI_IOT_DEVICE_SECRET;

    if (!pk || !pk[0] || !dn || !dn[0] || !ds || !ds[0]) {
        ESP_LOGE(TAG, "请先配置阿里云IoT三元组");
        return;
    }

    static char client_id[128];
    snprintf(client_id, sizeof(client_id), "%s|securemode=2,signmethod=hmacsha1|", dn);

    static char username[128];
    snprintf(username, sizeof(username), "%s&%s", dn, pk);

    static char password[64];
    ali_iot_gen_password(password, sizeof(password));

    ESP_LOGI(TAG, "Broker: %s", CONFIG_ALI_IOT_MQTT_URL);
    ESP_LOGI(TAG, "ClientID: %s", client_id);
    ESP_LOGI(TAG, "Username: %s", username);
    ESP_LOGI(TAG, "Password: %s", password);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = CONFIG_ALI_IOT_MQTT_URL,
            .verification.certificate = (const char *)ali_iot_ca_crt_start,
        },
        .credentials = {
            .client_id = client_id,
            .username = username,
            .authentication.password = password,
        }
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT客户端初始化失败");
        return;
    }

    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                   mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);

    ESP_LOGI(TAG, "阿里云IoT MQTT客户端已启动");
}

void ali_iot_deinit(void)
{
    if (mqtt_client == NULL) return;

    esp_mqtt_client_stop(mqtt_client);
    esp_mqtt_client_destroy(mqtt_client);
    mqtt_client = NULL;
    connected = false;
    light_cb = NULL;
    conn_cb = NULL;
    ESP_LOGI(TAG, "阿里云IoT MQTT客户端已释放");
}

bool ali_iot_is_connected(void)
{
    return connected;
}

void ali_iot_set_light_cb(ali_iot_light_cb_t cb)
{
    light_cb = cb;
}

void ali_iot_set_conn_cb(ali_iot_conn_cb_t cb)
{
    conn_cb = cb;
}
