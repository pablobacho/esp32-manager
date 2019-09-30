/**
 * esp32_manager_mqtt.c
 * 
 * (C) 2019 - Pablo Bacho <pablo@pablobacho.com>
 * This code is licensed under the MIT License.
 */

#include "esp32_manager_mqtt.h"

static const char * TAG = "esp32_manager_mqtt";

char esp32_manager_mqtt_broker_url[] = ESP32_MANAGER_MQTT_BROKER_URL_DEFAULT;

esp_mqtt_client_handle_t esp32_manager_mqtt_client = NULL;

esp32_manager_entry_t * esp32_manager_mqtt_entries[];

esp32_manager_namespace_t esp32_manager_mqtt_namespace = {
    .key = ESP32_MANAGER_MQTT_NAMESPACE_KEY,
    .friendly = ESP32_MANAGER_MQTT_NAMESPACE_FRIENDLY,
    .entries = esp32_manager_mqtt_entries,
    .size = ESP32_MANAGER_MQTT_NAMESPACE_SIZE
};

esp32_manager_entry_t esp32_manager_mqtt_entry_broker_url = {
    .key = ESP32_MANAGER_MQTT_BROKER_URL_KEY,
    .friendly = ESP32_MANAGER_MQTT_BROKER_URL_FRIENDLY,
    .type = text,
    .value = (void *) esp32_manager_mqtt_broker_url,
    .default_value = (void *) ESP32_MANAGER_MQTT_BROKER_URL_DEFAULT,
    .attributes = ESP32_MANAGER_ATTR_READWRITE
};

esp_err_t esp32_manager_mqtt_init() {
    esp_err_t e;
    
    // Register namespace
    e = esp32_manager_register_namespace(&esp32_manager_mqtt_namespace);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "%s namespace registered", esp32_manager_mqtt_namespace.key);
    } else {
        ESP_LOGE(TAG, "Error registering namespace%s: %s", esp32_manager_mqtt_namespace.key, esp_err_to_name(e));
        return ESP_FAIL;
    }

    // Register entries with esp32_manager
    e = esp32_manager_register_entry(&esp32_manager_mqtt_namespace, &esp32_manager_mqtt_entry_broker_url);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "%s setting registered", esp32_manager_mqtt_entry_broker_url.key);
    } else {
        ESP_LOGE(TAG, "Error registering %s", esp32_manager_mqtt_entry_broker_url.key);
        return ESP_FAIL;
    }

    // Register events relevant to the mqtt client
    e = esp_event_handler_register(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_STA_GOT_IP, esp32_manager_mqtt_system_event_handler, NULL);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Registered for event STA_GOT_IP");
    } else {
        ESP_LOGE(TAG, "Error registering for event STA_GOT_IP");
        return ESP_FAIL;
    }

    esp_event_handler_register(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_STA_LOST_IP, esp32_manager_mqtt_system_event_handler, NULL);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Registered for event STA_LOST_IP");
    } else {
        ESP_LOGE(TAG, "Error registering for event STA_LOST_IP");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t esp32_manager_mqtt_publish_entry(esp32_manager_namespace_t * namespace, esp32_manager_entry_t * entry)
{
    if(esp32_manager_mqtt_client == NULL) {
        return ESP_FAIL;
    }
    
    char topic[ESP32_MANAGER_MQTT_TOPIC_MAX_LENGTH] = "/";
    strcat(topic, esp32_manager_network_hostname);
    strcat(topic, "/");
    strcat(topic, namespace->key);
    strcat(topic, "/");
    strcat(topic, entry->key);
    
    char value_str[10]; // FIXME Magic number
    entry->to_string(entry, value_str);

    int msg_id = esp_mqtt_client_publish(esp32_manager_mqtt_client, topic, value_str, strlen(value_str), 0, false);
    ESP_LOGD(TAG, "Publish msg %d with topic %s and content %s", msg_id, topic, value_str);

    return ESP_OK;
}

esp_err_t esp32_manager_mqtt_event_handler(esp_mqtt_event_handle_t event)
{    
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

void esp32_manager_mqtt_system_event_handler(void * handler_arg, esp_event_base_t base, int32_t id, void * event_data)
{
    if(base != ESP32_MANAGER_NETWORK_EVENT_BASE) {
        return;
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = esp32_manager_mqtt_broker_url,
        .event_handle = esp32_manager_mqtt_event_handler,
    };

    switch(id) {
        case ESP32_MANAGER_NETWORK_EVENT_STA_GOT_IP:
            esp32_manager_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
            esp_mqtt_client_start(esp32_manager_mqtt_client);
            break;
        case ESP32_MANAGER_NETWORK_EVENT_STA_LOST_IP:
            esp_mqtt_client_stop(esp32_manager_mqtt_client);
            esp32_manager_mqtt_client = NULL;
            break;
        default: break;
    }
}