/**
 * esp32_manager.c
 * 
 * (C) 2019 - Pablo Bacho <pablo@pablobacho.com>
 * This code is licensed under the MIT License.
 */

 #include "esp32_manager.h"

static const char * TAG = "esp32_manager";

esp_err_t esp32_manager_init()
{
    esp_err_t e;

    // Initialize storage
    e = esp32_manager_storage_init();
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Storage initialized");
    } else {
        ESP_LOGE(TAG, "Error initializing storage");
        return ESP_FAIL;
    }

    // Initialize networking
    e = esp32_manager_network_init();
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Network initialized");
    } else {
        ESP_LOGE(TAG, "Error initializing network");
        return ESP_FAIL;
    }

    // Initialize webconfig
    e = esp32_manager_webconfig_init();
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Webconfig initialized");
    } else {
        ESP_LOGE(TAG, "Error initializing webconfig");
        return ESP_FAIL;
    }

    // Initialize MQTT
    e = esp32_manager_mqtt_init();
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "MQTT initialized");
    } else {
        ESP_LOGE(TAG, "Error initializing MQTT");
        return ESP_FAIL;
    }

    return ESP_OK;
}