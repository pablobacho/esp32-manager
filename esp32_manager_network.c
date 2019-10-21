/**
 * esp32_manager_network.c
 *
 * (C) 2019 - Pablo Bacho <pablo@pablobacho.com>
 * This code is licensed under the MIT License.
 */

 #include "esp32_manager_network.h"

static const char * TAG = "esp32_manager_network";

char esp32_manager_network_hostname[] = ESP32_MANAGER_NETWORK_HOSTNAME_DEFAULT;
char esp32_manager_network_ssid[] = ESP32_MANAGER_NETWORK_SSID_DEFAULT;
char esp32_manager_network_password[] = ESP32_MANAGER_NETWORK_PASSWORD_DEFAULT;

uint8_t esp32_manager_network_status = 0;

esp32_manager_entry_t * esp32_manager_network_entries[];

esp32_manager_namespace_t esp32_manager_network_namespace = {
    .key = ESP32_MANAGER_NETWORK_NAMESPACE_KEY,
    .friendly = ESP32_MANAGER_NETWORK_NAMESPACE_FRIENDLY,
    .entries = esp32_manager_network_entries,
    .size = ESP32_MANAGER_NETWORK_ENTRIES_SIZE
};

esp32_manager_entry_t esp32_manager_network_entry_hostname = {
    .key = ESP32_MANAGER_NETWORK_HOSTNAME_KEY,
    .friendly = ESP32_MANAGER_NETWORK_HOSTNAME_FRIENDLY,
    .type = text,
    .value = (void *) esp32_manager_network_hostname,
    .default_value = (void *) ESP32_MANAGER_NETWORK_HOSTNAME_DEFAULT,
    .attributes = ESP32_MANAGER_ATTR_READWRITE,
    .from_string = &esp32_manager_network_entry_hostname_from_string
};

esp32_manager_entry_t esp32_manager_network_entry_ssid = {
    .key = ESP32_MANAGER_NETWORK_SSID_KEY,
    .friendly = ESP32_MANAGER_NETWORK_SSID_FRIENDLY,
    .type = text,
    .value = (void *) esp32_manager_network_ssid,
    .default_value = (void *) ESP32_MANAGER_NETWORK_SSID_DEFAULT,
    .attributes = ESP32_MANAGER_ATTR_READWRITE,
    .from_string = &esp32_manager_network_entry_ssid_from_string,
    .html_form_widget = &esp32_manager_network_entry_ssid_html_form_widget
};

esp32_manager_entry_t esp32_manager_network_entry_password = {
    .key = ESP32_MANAGER_NETWORK_PASSWORD_KEY,
    .friendly = ESP32_MANAGER_NETWORK_PASSWORD_FRIENDLY,
    .type = password,
    .value = (void *) esp32_manager_network_password,
    .default_value = (void *) ESP32_MANAGER_NETWORK_PASSWORD_DEFAULT,
    .attributes = ESP32_MANAGER_ATTR_WRITE,
    .from_string = &esp32_manager_network_entry_password_from_string
};

ESP_EVENT_DEFINE_BASE(ESP32_MANAGER_NETWORK_EVENT_BASE);

esp_err_t esp32_manager_network_init()
{
    esp_err_t e;

    // Creating WiFi event loop
    e = esp_event_loop_create_default();
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "WiFi event loop created successfully");
    } else if(e == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "Default event loop was already started");
    } else {
        ESP_LOGE(TAG, "Error creating WiFi event loop: %s", esp_err_to_name(e));
        return e;
    }

    e = esp_event_loop_init(esp32_manager_network_event_handler, NULL);
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

    // Register namespace
    e = esp32_manager_register_namespace(&esp32_manager_network_namespace);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "%s namespace registered", ESP32_MANAGER_NETWORK_NAMESPACE_KEY);
    } else {
        ESP_LOGE(TAG, "Error registering namespace: %s", esp_err_to_name(e));
        return ESP_FAIL;
    }

    // Register entries with esp32_manager
    e = esp32_manager_register_entry(&esp32_manager_network_namespace, &esp32_manager_network_entry_hostname);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "%s entry registered", ESP32_MANAGER_NETWORK_HOSTNAME_KEY);
    } else {
        ESP_LOGE(TAG, "Error registering %s entry", ESP32_MANAGER_NETWORK_HOSTNAME_KEY);
        return ESP_FAIL;
    }

    e = esp32_manager_register_entry(&esp32_manager_network_namespace, &esp32_manager_network_entry_ssid);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "%s entry registered", ESP32_MANAGER_NETWORK_SSID_KEY);
    } else {
        ESP_LOGE(TAG, "Error registering %s entry", ESP32_MANAGER_NETWORK_SSID_KEY);
        return ESP_FAIL;
    }

    e = esp32_manager_register_entry(&esp32_manager_network_namespace, &esp32_manager_network_entry_password);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "%s entry registered", ESP32_MANAGER_NETWORK_PASSWORD_KEY);
    } else {
        ESP_LOGE(TAG, "Error registering %s entry", ESP32_MANAGER_NETWORK_PASSWORD_KEY);
        return ESP_FAIL;
    }

    // Read settings from NVS if any exist
    e = esp32_manager_read_from_nvs(&esp32_manager_network_namespace);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Network settings loaded. Hostname: %s, SSID: %s, Password: %s", esp32_manager_network_hostname, esp32_manager_network_ssid, esp32_manager_network_password);
    } else {
        ESP_LOGE(TAG, "Error loading network settings: %s", esp_err_to_name(e));
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t esp32_manager_network_wifi_start(esp32_manager_network_wifi_mode_t mode)
{
    switch(mode) {
        case AUTO:
            if(strlen(esp32_manager_network_ssid) > 0) {
                ESP_LOGD(TAG, "WiFi AUTO mode. Connecting to AP.");
                return esp32_manager_network_wifi_start_station_mode();
            } else {
                ESP_LOGD(TAG, "WiFi AUTO mode. Creating AP.");
                return esp32_manager_network_wifi_start_ap_mode();
            }
        break;
        case STA:
            ESP_LOGD(TAG, "WiFi STATION mode. Connecting to AP.");
            return esp32_manager_network_wifi_start_station_mode();
        break;
        case AP:
            ESP_LOGD(TAG, "WiFi AP mode. Creating AP.");
            return esp32_manager_network_wifi_start_ap_mode();
        break;
    }
    return ESP_FAIL;
}

esp_err_t esp32_manager_network_wifi_start_station_mode()
{
    esp_err_t e;

    ESP_LOGD(TAG, "Connecting to WiFi");

    if(strlen(esp32_manager_network_ssid) == 0) {
        ESP_LOGE(TAG, "Error connecting to AP: No SSID Found.");
        return ESP_FAIL;
    }

    wifi_config_t wifi_config = {};

    strcpy((char *) wifi_config.sta.ssid, esp32_manager_network_ssid);
    strcpy((char *) wifi_config.sta.password, esp32_manager_network_password);
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

    // e = tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, esp32_manager_network_hostname);
    // if(e == ESP_OK) {
    //     ESP_LOGD(TAG, "Hostname set to %s", esp32_manager_network_hostname);
    // } else {
    //     ESP_LOGE(TAG, "Error setting hostname: %s", esp_err_to_name(e));
    // }

    return ESP_OK;
}

esp_err_t esp32_manager_network_wifi_start_ap_mode()
{
    esp_err_t e;

    ESP_LOGD(TAG, "Enter AP mode");

    wifi_config_t wifi_config = {};

    wifi_config.ap.ssid_len = strlen(ESP32_MANAGER_NETWORK_AP_SSID);
    wifi_config.ap.max_connection = 1;
    if(strlen(ESP32_MANAGER_NETWORK_AP_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    } else {
        wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }
    strcpy((char *) wifi_config.ap.ssid, ESP32_MANAGER_NETWORK_AP_SSID);
    strcpy((char *) wifi_config.ap.password, ESP32_MANAGER_NETWORK_AP_PASSWORD);

    e = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "WiFi set to AP+STA mode");
    } else {
        ESP_LOGE(TAG, "Error setting WiFi to AP+STA mode: %s", esp_err_to_name(e));
        return ESP_FAIL;
    }

    e = esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "WiFi AP configured");
    } else {
        ESP_LOGE(TAG, "Error configuring WiFi AP: %s", esp_err_to_name(e));
        return ESP_FAIL;
    }

    tcpip_adapter_ip_info_t ip_info;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info);
    ESP_LOGD(TAG, "AP IP: %s", ip4addr_ntoa(&ip_info.ip));

    e = esp_wifi_start();
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Wifi started in AP+STA mode");
    } else {
        ESP_LOGE(TAG, "Error starting Wifi in AP+STA mode: %s", esp_err_to_name(e));
    }

    return ESP_OK;
}

esp_err_t esp32_manager_network_wifi_stop()
{
    return esp_wifi_stop();
}

bool esp32_manager_network_connected()
{
    return (esp32_manager_network_status & ESP32_MANAGER_NETWORK_STATUS_CONNECTED) ? true : false;
}

bool esp32_manager_network_got_ip()
{
    return (esp32_manager_network_status & ESP32_MANAGER_NETWORK_STATUS_GOT_IP) ? true : false;
}

bool esp32_manager_network_is_ap()
{
    return (esp32_manager_network_status & ESP32_MANAGER_NETWORK_STATUS_AP_STARTED) ? true : false;
}

esp_err_t esp32_manager_network_event_handler(void * context, system_event_t * event)
{
    ESP_LOGD(TAG, "Event: %d", event->event_id);

    switch(event->event_id) {
        case SYSTEM_EVENT_WIFI_READY:
            ESP_LOGD(TAG, "Event: WIFI_READY");
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_WIFI_READY, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_SCAN_DONE:
            ESP_LOGD(TAG, "Event: SCAN_DONE");
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_SCAN_DONE, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            ESP_LOGD(TAG, "Event: STA_START");
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_STA_START, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_STOP:
            ESP_LOGD(TAG, "Event: STA_STOP");
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_STA_STOP, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_CONNECTED:
            ESP_LOGD(TAG, "Connected to WiFi: %s", (char *) &event->event_info.connected.ssid);
            ESP_LOGD(TAG, "Event: STA_CONNECTED");
            esp32_manager_network_status |= ESP32_MANAGER_NETWORK_STATUS_CONNECTED;
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_STA_CONNECTED, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGD(TAG, "Event: STA_DISCONNECTED");
            esp32_manager_network_status &= ~ESP32_MANAGER_NETWORK_STATUS_CONNECTED;
            // esp32_manager_network_wifi_stop();
            // esp32_manager_network_wifi_start_station_mode();
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_STA_DISCONNECTED, NULL, 0, portMAX_DELAY);
            esp_wifi_connect();
        break;
        case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
            ESP_LOGD(TAG, "Event: STA_AUTHMODE_CHANGE");
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_STA_AUTHMODE_CHANGE, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGD(TAG, "Got IP: %s", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            ESP_LOGD(TAG, "Event: STA_GOT_IP");
            esp32_manager_network_status |= ESP32_MANAGER_NETWORK_STATUS_GOT_IP;
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_STA_GOT_IP, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_LOST_IP:
            ESP_LOGD(TAG, "Event: STA_LOST_IP");
            esp32_manager_network_status &= ~ESP32_MANAGER_NETWORK_STATUS_GOT_IP;
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_STA_LOST_IP, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
            ESP_LOGD(TAG, "Event: STA_WPS_ER_SUCCESS");
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_STA_WPS_ER_SUCCESS, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_WPS_ER_FAILED:
            ESP_LOGD(TAG, "Event: STA_WPS_ER_FAILED");
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_STA_WPS_ER_FAILED, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
            ESP_LOGD(TAG, "Event: STA_WPS_ER_TIMEOUT");
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_STA_WPS_ER_TIMEOUT, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_STA_WPS_ER_PIN:
            ESP_LOGD(TAG, "Event: STA_WPS_ER_PIN");
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_STA_WPS_ER_PIN, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_AP_START:
            ESP_LOGD(TAG, "Event: AP_START");
            esp32_manager_network_status |= ESP32_MANAGER_NETWORK_STATUS_AP_STARTED;
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_AP_START, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_AP_STOP:
            ESP_LOGD(TAG, "Event: AP_STOP");
            esp32_manager_network_status &= ~ESP32_MANAGER_NETWORK_STATUS_AP_STARTED;
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_AP_STOP, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGD(TAG, "Event: AP_STACONNECTED");
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_AP_STACONNECTED, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGD(TAG, "Event: AP_STADISCONNECTED");
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_AP_STADISCONNECTED, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_AP_STAIPASSIGNED:
            ESP_LOGD(TAG, "Event: AP_STAIPASIGNED");
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_AP_STAIPASSIGNED, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_AP_PROBEREQRECVED:
            ESP_LOGD(TAG, "Event: AP_PROBEREQRECVED");
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_AP_PROBEREQRECVED, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_GOT_IP6:
            ESP_LOGD(TAG, "Event: GOT_IP6");
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_GOT_IP6, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_ETH_START:
            ESP_LOGD(TAG, "Event: ETH_START");
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_ETH_START, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_ETH_STOP:
            ESP_LOGD(TAG, "Event: ETH_STOP");
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_ETH_STOP, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_ETH_CONNECTED:
            ESP_LOGD(TAG, "Event: ETH_CONNECTED");
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_ETH_CONNECTED, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_ETH_DISCONNECTED:
            ESP_LOGD(TAG, "Event: ETH_DISCONNECTED");
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_ETH_DISCONNECTED, NULL, 0, portMAX_DELAY);
        break;
        case SYSTEM_EVENT_ETH_GOT_IP:
            ESP_LOGD(TAG, "Event: ETH_GOT_IP");
            esp_event_post(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_ETH_GOT_IP, NULL, 0, portMAX_DELAY);
        break;
        default:
            ESP_LOGE(TAG, "Event: unknown");
        break;
    }

    return ESP_OK;
}

esp_err_t esp32_manager_network_entry_hostname_from_string(esp32_manager_entry_t * entry, char * source)
{
    esp_err_t e;

    if(esp32_manager_validate_entry(entry) != ESP_OK) {
        ESP_LOGE(TAG, "Error esp32_manager_network_entry_hostname_from_string: invalid entry");
        return ESP_ERR_INVALID_ARG;
    }

    if(source == NULL) {
        ESP_LOGE(TAG, "Error esp32_manager_network_entry_hostname_from_string: null string");
        return ESP_ERR_INVALID_ARG;
    }

    char hostname[ESP32_MANAGER_NETWORK_HOSTNAME_MAX_LENGTH +1]; // Temporary strin to process the new hostname on
    uint8_t hostname_len = strlen(source);
    if((hostname_len == 0) || (hostname_len > ESP32_MANAGER_NETWORK_HOSTNAME_MAX_LENGTH)) {
        ESP_LOGE(TAG, "Hostname esp32_manager_network_entry_hostname_from_string: length %u", hostname_len);
        return ESP_FAIL;
    }
    strlcpy(hostname, source, hostname_len +1);

    if(!isalnum((unsigned char) hostname[0])) {
        ESP_LOGE(TAG, "Error: hostname must start with an alphanumeric character");
        return ESP_FAIL;
    } else {
        hostname[0] = tolower((unsigned char) hostname[0]);
    }
    for(uint8_t i=1; i < hostname_len; ++i) {
        if(!isalnum((unsigned char) hostname[i]) && (hostname[i] != '-')) {
            ESP_LOGE(TAG, "Error: invalid hostname (found '%c')", hostname[i]);
            return ESP_FAIL;
        }
        hostname[i] = tolower((unsigned char) hostname[i]);
    }

    e = tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, hostname);
    if(e !=ESP_OK) {
        ESP_LOGE(TAG, "Error updating hostname on TCP/IP stack: %s", esp_err_to_name(e));
        return ESP_FAIL;
    }

    strlcpy((char *) entry->value, hostname, hostname_len +1);

    ESP_LOGD(TAG, "Hostname successfully updated to %s", hostname);

    return ESP_OK;
}

esp_err_t esp32_manager_network_entry_ssid_from_string(esp32_manager_entry_t * entry, char * source)
{
    if(esp32_manager_validate_entry(entry) != ESP_OK) {
        ESP_LOGE(TAG, "Error esp32_manager_network_entry_ssid_from_string: invalid entry");
        return ESP_ERR_INVALID_ARG;
    }

    if(source == NULL) {
        ESP_LOGE(TAG, "Error esp32_manager_network_entry_ssid_from_string: null string");
        return ESP_ERR_INVALID_ARG;
    }

    char ssid[ESP32_MANAGER_NETWORK_SSID_MAX_LENGTH +1];
    uint8_t ssid_len = strlen(source);
    if((ssid_len == 0) || (ssid_len > ESP32_MANAGER_NETWORK_SSID_MAX_LENGTH)) {
        ESP_LOGE(TAG, "Error esp32_manager_network_entry_ssid_from_string: length %u", ssid_len);
        return ESP_FAIL;
    }

    strlcpy(ssid, source, ssid_len +1);

    while(ssid[ssid_len -1] == ' ') {
        ssid[ssid_len -1] = 0;
        --ssid_len;
        ESP_LOGW(TAG, "Warning: Removed trailing space from SSID");
    }

    strlcpy((char *) entry->value, ssid, ssid_len +1);

    ESP_LOGD(TAG, "SSID successfully updated to %s", ssid);

    return ESP_OK;
}

esp_err_t esp32_manager_network_entry_ssid_html_form_widget(char * buffer, esp32_manager_entry_t * entry, size_t buffer_size)
{
    esp_err_t e;

    if(buffer == NULL || entry == NULL || buffer_size == 0) {
        ESP_LOGE(TAG, "Error esp32_manager_network_entry_ssid_html_form_widget: invalid args");
        return ESP_ERR_INVALID_ARG;
    }

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false
    };

    strlcpy(buffer, "<div>", buffer_size);

    e = esp_wifi_scan_start(&scan_config, true);
    if(e != ESP_OK) {
        ESP_LOGE(TAG, "Error starting WiFi scan: %s", esp_err_to_name(e));
    } else { // Scan successful
        ESP_LOGD(TAG, "WiFi scan started");

        uint16_t ap_records_size = 10;
        wifi_ap_record_t ap_records[10];
        e = esp_wifi_scan_get_ap_records(&ap_records_size, ap_records);
        if(e != ESP_OK) {
            ESP_LOGE(TAG, "Error getting WiFi scan records: %s", esp_err_to_name(e));
        } else { // Got scan records
            ESP_LOGD(TAG, "Got WiFi scan records");
            // Print AP list
            // Table header

            strlcat(buffer, "<table><thead><tr><th>SSID</th><th>Signal</th></tr></thead><tbody>", buffer_size);
            for(uint8_t i=0; i < ap_records_size; ++i) {
                strlcat(buffer, "<tr><td><a href=\"#\" onclick=\"document.getElementById('", buffer_size);
                strlcat(buffer, (char *) entry->key, buffer_size);
                strlcat(buffer, "').value='", buffer_size);
                strlcat(buffer, (char *) ap_records[i].ssid, buffer_size);
                strlcat(buffer, "';\">", buffer_size);
                strlcat(buffer, (char *) ap_records[i].ssid, buffer_size);
                strlcat(buffer, "</a></td><td>", buffer_size);
                if(ap_records[i].rssi > -60) {
                    strlcat(buffer, "Excellent", buffer_size);
                } else if(ap_records[i].rssi > -70) {
                    strlcat(buffer, "Good", buffer_size);
                } else if(ap_records[i].rssi > -80) {
                    strlcat(buffer, "Poor", buffer_size);
                } else {
                    strlcat(buffer, "Bad", buffer_size);
                }
                strlcat(buffer, "</td></tr>", buffer_size); // close column, close row
            }
            strlcat(buffer, "</tbody></table>", buffer_size);
        }
    }

    strlcat(buffer, entry->friendly, buffer_size);
    strlcat(buffer, "<br/>", buffer_size);
    strlcat(buffer, "<input type=\"text\" id=\"", buffer_size);
    strlcat(buffer, entry->key, buffer_size);
    strlcat(buffer, "\" name=\"", buffer_size);
    strlcat(buffer, entry->key, buffer_size);
    strlcat(buffer, "\" value=\"", buffer_size);
    strlcat(buffer, (char *) entry->value, buffer_size);
    strlcat(buffer, "\" />", buffer_size);

    return ESP_OK;
}

esp_err_t esp32_manager_network_entry_password_from_string(esp32_manager_entry_t * entry, char * source)
{
    if(esp32_manager_validate_entry(entry) != ESP_OK) {
        ESP_LOGE(TAG, "Error esp32_manager_network_entry_password_from_string: invalid entry");
        return ESP_ERR_INVALID_ARG;
    }

    if(source == NULL) {
        ESP_LOGE(TAG, "Error esp32_manager_network_entry_password_from_string: null string");
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t password_len = strlen(source);
    if((password_len < ESP32_MANAGER_NETWORK_PASSWORD_MIN_LENGTH) || (password_len > ESP32_MANAGER_NETWORK_PASSWORD_MAX_LENGTH)) {
        ESP_LOGE(TAG, "Error esp32_manager_network_entry_password_from_string: length %u", password_len);
        return ESP_FAIL;
    }

    strlcpy((char *) entry->value, source, password_len +1);

    ESP_LOGD(TAG, "Password successfully updated to %s", source);

    return ESP_OK;
}
