/**
 * network_manager.c - ESP-IDF component to work with settings
 * 
 * Implementation.
 * 
 * (C) 2019 - Pablo Bacho <pablo@pablobacho.com>
 * This code is licensed under the MIT License.
 */

 #include "network_manager.h"

static const char * TAG = "network_manager";


const char * NETWORK_MANAGER_NAMESPACE = NETWORK_MANAGER_NAMESPACE_CONFIG;

char network_manager_ssid[];
char network_manager_password[];

settings_manager_handle_t network_manager_settings_handle;

ESP_EVENT_DEFINE_BASE(NETWORK_MANAGER_EVENT_BASE);

esp_err_t network_manager_init()
{
    esp_err_t e;

    // Creating WiFi event loop
    e = esp_event_loop_create_default();
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "WiFi event loop created successfully");
    } else {
        ESP_LOGE(TAG, "Error creating WiFi event loop: %s", esp_err_to_name(e));
        return e;
    }

    e = esp_event_loop_init(network_manager_event_handler, NULL);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "WiFi legacy event loop initialized successfully");
    } else {
        ESP_LOGE(TAG, "Error initializing legacy WiFi event loop: %s", esp_err_to_name(e));
        return e;
    }

    // Start TCP/IP stack
    tcpip_adapter_init();

    // Init WiFi
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT(); // WiFi stack init and config
    e = esp_wifi_init(&wifi_init_config);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Wifi initialized successfully");
    } else {
        ESP_LOGE(TAG, "Error initializing WiFi: %s", esp_err_to_name(e));
        return e;
    }

    // Set storage to RAM
    e = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "WiFi storage set to RAM");
    } else {
        ESP_LOGE(TAG, "Error setting WiFi storage to RAM: %s", esp_err_to_name(e));
        return ESP_FAIL;
    }

    // Register namespace with settings_manager
    network_manager_settings_handle = settings_manager_register_namespace(NETWORK_MANAGER_NAMESPACE_CONFIG);
    if(network_manager_settings_handle != NULL) {
        ESP_LOGD(TAG, "Namespace registered");
    } else {
        ESP_LOGE(TAG, "Error registering namespace: %s", esp_err_to_name(e));
        return ESP_FAIL;
    }

    // Register settings with settings_manager
    e = settings_manager_register_setting(network_manager_settings_handle, NETWORK_MANAGER_SSID_KEY, text, network_manager_ssid, SETTINGS_MANAGER_ATTR_READWRITE);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "network_manager_ssid registered with settings_manager");
    } else {
        ESP_LOGE(TAG, "Error registering network_manager_ssid with settings_manager: %s", esp_err_to_name(e));
        return ESP_FAIL;
    }

    e = settings_manager_register_setting(network_manager_settings_handle, NETWORK_MANAGER_PASSWORD_KEY, password, network_manager_password, SETTINGS_MANAGER_ATTR_WRITE);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "network_manager_password registered with settings_manager");
    } else {
        ESP_LOGE(TAG, "Error registering network_manager_ssid with settings_manager: %s", esp_err_to_name(e));
        return ESP_FAIL;
    }

    // Read settings from NVS if any exist
    e = settings_manager_read_from_nvs(network_manager_settings_handle);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Network settings loaded. SSID: %s, Password: %s", network_manager_ssid, network_manager_password);
    } else {
        ESP_LOGE(TAG, "Error loading network settings: %s", esp_err_to_name(e));
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t network_manager_wifi_start(network_manager_wifi_mode_t mode)
{
    switch(mode) {
        case AUTO:
            if(strlen(network_manager_ssid) > 0) {
                ESP_LOGD(TAG, "WiFi AUTO mode. Connecting to AP.");
                return network_manager_wifi_start_station_mode();
            } else {
                ESP_LOGD(TAG, "WiFi AUTO mode. Creating AP.");
                return network_manager_wifi_start_ap_mode();
            }
        break;
        case STA:
            ESP_LOGD(TAG, "WiFi STATION mode. Connecting to AP.");
            return network_manager_wifi_start_station_mode();
        break;
        case AP:
            ESP_LOGD(TAG, "WiFi AP mode. Creating AP.");
            return network_manager_wifi_start_ap_mode();
        break;
    }
    return ESP_FAIL;
}

esp_err_t network_manager_wifi_start_station_mode()
{
    esp_err_t e;

    ESP_LOGD(TAG, "Connecting to WiFi");

    if(strlen(network_manager_ssid) == 0) {
        ESP_LOGE(TAG, "Error connecting to AP: No SSID Found.");
        return ESP_FAIL;
    }

    wifi_config_t wifi_config = {};

    strcpy((char *) wifi_config.sta.ssid, network_manager_ssid);
    strcpy((char *) wifi_config.sta.password, network_manager_password);
    ESP_LOGD(TAG, "Setting WiFi configuration SSID %s", wifi_config.sta.ssid);

    e = esp_wifi_set_mode(WIFI_MODE_STA);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "WiFi set to station mode");
    } else {
        ESP_LOGE(TAG, "Error setting WiFi to station mode: %s", esp_err_to_name(e));
        return ESP_FAIL;
    }

    e = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "WiFi configured");
    } else {
        ESP_LOGE(TAG, "Error configuring WiFi: %s", esp_err_to_name(e));
        return ESP_FAIL;
    }

    e = esp_wifi_start();
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "WiFi started in station mode successfully");
    } else {
        ESP_LOGE(TAG, "Error starting WiFi in station mode: %s", esp_err_to_name(e));
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t network_manager_wifi_start_ap_mode()
{
    esp_err_t e;

    ESP_LOGD(TAG, "Enter AP mode");

    wifi_config_t wifi_config = {};

    wifi_config.ap.ssid_len = strlen(NETWORK_MANAGER_AP_SSID);
    wifi_config.ap.max_connection = 1;
    if(strlen(NETWORK_MANAGER_AP_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    } else {
        wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }
    strcpy((char *) wifi_config.ap.ssid, NETWORK_MANAGER_AP_SSID);
    strcpy((char *) wifi_config.ap.password, NETWORK_MANAGER_AP_PASSWORD);

    e = esp_wifi_set_mode(WIFI_MODE_AP);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "WiFi set to AP mode");
    } else {
        ESP_LOGE(TAG, "Error setting WiFi to AP mode: %s", esp_err_to_name(e));
        return ESP_FAIL;
    }

    e = esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "WiFi configured");
    } else {
        ESP_LOGE(TAG, "Error configuring WiFi: %s", esp_err_to_name(e));
        return ESP_FAIL;
    }

    tcpip_adapter_ip_info_t ip_info;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info);
    ESP_LOGD(TAG, "AP IP: %s", ip4addr_ntoa(&ip_info.ip));

    e = esp_wifi_start();
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Wifi started in AP mode");
    } else {
        ESP_LOGE(TAG, "Error starting Wifi in AP mode: %s", esp_err_to_name(e));
    }

    return ESP_OK;
}

esp_err_t network_manager_wifi_stop()
{
    return esp_wifi_stop();
}

esp_err_t network_manager_event_handler(void * context, system_event_t * event)
{
    ESP_LOGD(TAG, "Event rised: %d", event->event_id);

    switch(event->event_id) {
        case SYSTEM_EVENT_WIFI_READY:
            ESP_LOGD(TAG, "Event: WIFI_READY");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_WIFI_READY, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_SCAN_DONE:
            ESP_LOGD(TAG, "Event: SCAN_DONE");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_SCAN_DONE, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            ESP_LOGD(TAG, "Event: STA_START");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_STA_START, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_STOP:
            ESP_LOGD(TAG, "Event: STA_STOP");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_STA_STOP, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_CONNECTED:
            ESP_LOGD(TAG, "Connected to WiFi: %s", (char *) &event->event_info.connected.ssid);
            ESP_LOGD(TAG, "Event: STA_CONNECTED");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_STA_CONNECTED, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            ESP_LOGD(TAG, "Event: STA_DISCONNECTED");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_STA_DISCONNECTED, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
            ESP_LOGD(TAG, "Event: STA_AUTHMODE_CHANGE");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_STA_AUTHMODE_CHANGE, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGD(TAG, "Got IP: %s", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            ESP_LOGD(TAG, "Event: STA_GOT_IP");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_STA_GOT_IP, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_LOST_IP:
            ESP_LOGD(TAG, "Event: STA_LOST_IP");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_STA_LOST_IP, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
            ESP_LOGD(TAG, "Event: STA_WPS_ER_SUCCESS");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_STA_WPS_ER_SUCCESS, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_WPS_ER_FAILED:
            ESP_LOGD(TAG, "Event: STA_WPS_ER_FAILED");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_STA_WPS_ER_FAILED, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
            ESP_LOGD(TAG, "Event: STA_WPS_ER_TIMEOUT");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_STA_WPS_ER_TIMEOUT, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_WPS_ER_PIN:
            ESP_LOGD(TAG, "Event: STA_WPS_ER_PIN");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_STA_WPS_ER_PIN, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_AP_START:
            ESP_LOGD(TAG, "Event: AP_START");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_AP_START, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_AP_STOP:
            ESP_LOGD(TAG, "Event: AP_STOP");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_AP_STOP, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGD(TAG, "Event: AP_STACONNECTED");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_AP_STACONNECTED, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGD(TAG, "Event: AP_STADISCONNECTED");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_AP_STADISCONNECTED, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_AP_STAIPASSIGNED:
            ESP_LOGD(TAG, "Event: AP_STAIPASIGNED");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_AP_STAIPASSIGNED, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_AP_PROBEREQRECVED:
            ESP_LOGD(TAG, "Event: AP_PROBEREQRECVED");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_AP_PROBEREQRECVED, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_GOT_IP6:
            ESP_LOGD(TAG, "Event: GOT_IP6");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_GOT_IP6, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_ETH_START:
            ESP_LOGD(TAG, "Event: ETH_START");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_ETH_START, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_ETH_STOP:
            ESP_LOGD(TAG, "Event: ETH_STOP");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_ETH_STOP, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_ETH_CONNECTED:
            ESP_LOGD(TAG, "Event: ETH_CONNECTED");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_ETH_CONNECTED, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_ETH_DISCONNECTED:
            ESP_LOGD(TAG, "Event: ETH_DISCONNECTED");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_ETH_DISCONNECTED, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_ETH_GOT_IP:
            ESP_LOGD(TAG, "Event: ETH_GOT_IP");
            esp_event_post(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_ETH_GOT_IP, NULL, 0, portMAX_DELAY);
        break;
        default:
            ESP_LOGE(TAG, "Event: unknown");
        break;
    }

    return ESP_OK;
}