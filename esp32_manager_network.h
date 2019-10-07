/**
 * esp32_manager_network.h - ESP-IDF component to work with WiFi
 *
 * Include this header file to use the component.
 *
 * (C) 2019 - Pablo Bacho <pablo@pablobacho.com>
 * This code is licensed under the MIT License.
 */

#ifndef _ESP32_MANAGER_NETWORK_H_
#define _ESP32_MANAGER_NETWORK_H_

#include <string.h>
#include <ctype.h>

#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "tcpip_adapter.h"
#include "freertos/event_groups.h"

#include "esp32_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP32_MANAGER_NETWORK_NAMESPACE_KEY       "network"   /*!< Namespace to store settings of this module */
#define ESP32_MANAGER_NETWORK_NAMESPACE_FRIENDLY  "Network"   /*!< Network namespace friendly name */

#define ESP32_MANAGER_NETWORK_HOSTNAME_KEY        "hostname"
#define ESP32_MANAGER_NETWORK_HOSTNAME_FRIENDLY   "Hostname"
#define ESP32_MANAGER_NETWORK_HOSTNAME_MAX_LENGTH 32
#define ESP32_MANAGER_NETWORK_HOSTNAME_DEFAULT    "esp32-device"

#define ESP32_MANAGER_NETWORK_SSID_KEY            "ssid"      /*!< SSID entry key */
#define ESP32_MANAGER_NETWORK_SSID_FRIENDLY       "SSID"      /*!< SSID friendly name */
#define ESP32_MANAGER_NETWORK_SSID_MAX_LENGTH     32          /*!< Maximum length of an SSID */
#define ESP32_MANAGER_NETWORK_SSID_DEFAULT        ""          /*!< Default SSID to connect to */

#define ESP32_MANAGER_NETWORK_PASSWORD_KEY        "password"  /*!< password entry key */
#define ESP32_MANAGER_NETWORK_PASSWORD_FRIENDLY   "Password"  /*!< password friendly name */
#define ESP32_MANAGER_NETWORK_PASSWORD_MIN_LENGTH 8           /*!< Minimum password length */
#define ESP32_MANAGER_NETWORK_PASSWORD_MAX_LENGTH 63          /*!< Maximum password length */
#define ESP32_MANAGER_NETWORK_PASSWORD_DEFAULT    ""          /*!< Default password of the SSID to connect to */

#define ESP32_MANAGER_NETWORK_ENTRIES_SIZE          3

#define ESP32_MANAGER_NETWORK_AP_SSID         "wifi-manager"  /*!< SSID to use when creating an AP */
#define ESP32_MANAGER_NETWORK_AP_PASSWORD     "12345678"      /*!< Password of the AP created */

/**
 * Mode to start WiFi in
 */
typedef enum {
    AUTO = 0,   /*!< Auto will start in STATION mode if there is a valid SSID. Otherwise will create AP. */
    STA,        /*!< Start in STATION mode */
    AP          /*!< Create AP */
} esp32_manager_network_wifi_mode_t;

extern const char * ESP32_MANAGER_NETWORK_NAMESPACE;  /*!< String to store namespace. Pointers to namespace name point to this variable. */
extern char esp32_manager_network_hostname[ESP32_MANAGER_NETWORK_HOSTNAME_MAX_LENGTH +1];
extern char esp32_manager_network_ssid[ESP32_MANAGER_NETWORK_SSID_MAX_LENGTH +1]; /*!< String to store SSID to connect to in STATION mode */
extern char esp32_manager_network_password[ESP32_MANAGER_NETWORK_PASSWORD_MAX_LENGTH +1]; /*!< String to store password of the SSID to connect to in STATION mode */

extern esp32_manager_entry_t * esp32_manager_network_entries[ESP32_MANAGER_NETWORK_ENTRIES_SIZE];
extern esp32_manager_namespace_t esp32_manager_network_namespace;
extern esp32_manager_entry_t esp32_manager_network_entry_hostname;
extern esp32_manager_entry_t esp32_manager_network_entry_ssid;
extern esp32_manager_entry_t esp32_manager_network_entry_password;

#define ESP32_MANAGER_NETWORK_STATUS_CONNECTED    0b00000001
#define ESP32_MANAGER_NETWORK_STATUS_GOT_IP       0b00000010
#define ESP32_MANAGER_NETWORK_STATUS_AP_STARTED   0b00000100
extern uint8_t esp32_manager_network_status;

ESP_EVENT_DECLARE_BASE(ESP32_MANAGER_NETWORK_EVENT_BASE); /*!< BASE event of this module */

/**
 * List of possible events this module can trigger
 */
typedef enum {
    ESP32_MANAGER_NETWORK_EVENT_WIFI_READY = 0,           /*!< ESP32 WiFi ready */
    ESP32_MANAGER_NETWORK_EVENT_SCAN_DONE,                /*!< ESP32 finish scanning AP */
    ESP32_MANAGER_NETWORK_EVENT_STA_START,                /*!< ESP32 station start */
    ESP32_MANAGER_NETWORK_EVENT_STA_STOP,                 /*!< ESP32 station stop */
    ESP32_MANAGER_NETWORK_EVENT_STA_CONNECTED,            /*!< ESP32 station connected to AP */
    ESP32_MANAGER_NETWORK_EVENT_STA_DISCONNECTED,         /*!< ESP32 station disconnected from AP */
    ESP32_MANAGER_NETWORK_EVENT_STA_AUTHMODE_CHANGE,      /*!< the auth mode of AP connected by ESP32 station changed */
    ESP32_MANAGER_NETWORK_EVENT_STA_GOT_IP,               /*!< ESP32 station got IP from connected AP */
    ESP32_MANAGER_NETWORK_EVENT_STA_LOST_IP,              /*!< ESP32 station lost IP and the IP is reset to 0 */
    ESP32_MANAGER_NETWORK_EVENT_STA_WPS_ER_SUCCESS,       /*!< ESP32 station wps succeeds in enrollee mode */
    ESP32_MANAGER_NETWORK_EVENT_STA_WPS_ER_FAILED,        /*!< ESP32 station wps fails in enrollee mode */
    ESP32_MANAGER_NETWORK_EVENT_STA_WPS_ER_TIMEOUT,       /*!< ESP32 station wps timeout in enrollee mode */
    ESP32_MANAGER_NETWORK_EVENT_STA_WPS_ER_PIN,           /*!< ESP32 station wps pin code in enrollee mode */
    ESP32_MANAGER_NETWORK_EVENT_AP_START,                 /*!< ESP32 soft-AP start */
    ESP32_MANAGER_NETWORK_EVENT_AP_STOP,                  /*!< ESP32 soft-AP stop */
    ESP32_MANAGER_NETWORK_EVENT_AP_STACONNECTED,          /*!< a station connected to ESP32 soft-AP */
    ESP32_MANAGER_NETWORK_EVENT_AP_STADISCONNECTED,       /*!< a station disconnected from ESP32 soft-AP */
    ESP32_MANAGER_NETWORK_EVENT_AP_STAIPASSIGNED,         /*!< ESP32 soft-AP assign an IP to a connected station */
    ESP32_MANAGER_NETWORK_EVENT_AP_PROBEREQRECVED,        /*!< Receive probe request packet in soft-AP interface */
    ESP32_MANAGER_NETWORK_EVENT_GOT_IP6,                  /*!< ESP32 station or ap or ethernet interface v6IP addr is preferred */
    ESP32_MANAGER_NETWORK_EVENT_ETH_START,                /*!< ESP32 ethernet start */
    ESP32_MANAGER_NETWORK_EVENT_ETH_STOP,                 /*!< ESP32 ethernet stop */
    ESP32_MANAGER_NETWORK_EVENT_ETH_CONNECTED,            /*!< ESP32 ethernet phy link up */
    ESP32_MANAGER_NETWORK_EVENT_ETH_DISCONNECTED,         /*!< ESP32 ethernet phy link down */
    ESP32_MANAGER_NETWORK_EVENT_ETH_GOT_IP,               /*!< ESP32 ethernet got IP from connected AP */
    ESP32_MANAGER_NETWORK_EVENT_MAX
} esp32_manager_network_event_id_t;

/**
 * @brief   Initialize esp32_manager_network module.
 *
 *          This function needs to be called after initializing settings_manager
 *
 * @return  ESP_OK success
 *          ESP_FAIL error
 */
esp_err_t esp32_manager_network_init();

/**
 * @brief   Start WiFi and connect to network or create AP
 *
 * @param   mode Mode to start the WiFi on: AUTO, STA or AP.
 * @return  ESP_OK success
 *          ESP_FAIL error
 */
esp_err_t esp32_manager_network_wifi_start(esp32_manager_network_wifi_mode_t mode);

/**
 * @brief   Start WiFi in station mode
 *
 * @return  ESP_OK success
 *          ESP_FAIL error
 */
esp_err_t esp32_manager_network_wifi_start_station_mode();

/**
 * @brief   Start WiFi in AP mode
 *
 * @return  ESP_OK success
 *          ESP_FAIL error
 */
esp_err_t esp32_manager_network_wifi_start_ap_mode();

/**
 * @brief   Stop WiFi
 *
 * @return  ESP_OK success
 *          ESP_ERR_WIFI_NOT_INIT if wifi not initialized
 */
esp_err_t esp32_manager_network_wifi_stop();

/**
 * @brief   Check if device connected to WiFi network.
 *
 * @return  Connection status
 */
bool esp32_manager_network_connected();

/**
 * @brief   Check if device got IP address.
 *
 * @return  IP status
 */
bool esp32_manager_network_got_ip();

/**
 * @brief   Check if AP is started.
 *
 * @return  AP status
 */
bool esp32_manager_network_is_ap();

/**
 * @brief   Event handler for network events.
 *
 *          This function receives events from the legacy event system and posts them to the default event loop of the new event library.
 *
 * @return  ESP_OK always succeeds although it does not mean the event was successfully posted
 */
esp_err_t esp32_manager_network_event_handler(void * context, system_event_t * event);

/**
 * @brief   esp32_manager callback to update hostname from string
 *
 * @param   entry pointer to entry
 * @param   source string encoded new value
 * @return  ESP_OK success
 *          ESP_FAIL error
 */
esp_err_t esp32_manager_network_entry_hostname_from_string(esp32_manager_entry_t * entry, char * source);

/**
 * @brief   esp32_manager callback to update ssid from string
 *
 * @param   entry pointer to entry
 * @param   source string encoded new value
 * @return  ESP_OK success
 *          ESP_FAIL error
 */
esp_err_t esp32_manager_network_entry_ssid_from_string(esp32_manager_entry_t * entry, char * source);

esp_err_t esp32_manager_network_entry_ssid_html_form_widget(char * buffer, esp32_manager_entry_t * entry, size_t buffer_size);

/**
 * @brief   esp32_manager callback to update password from string
 *
 * @param   entry pointer to entry
 * @param   source string encoded new value
 * @return  ESP_OK success
 *          ESP_FAIL error
 */
esp_err_t esp32_manager_network_entry_password_from_string(esp32_manager_entry_t * entry, char * source);

#ifdef __cplusplus
}
#endif

#endif // _ESP32_MANAGER_NETWORK_H_