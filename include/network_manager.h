/**
 * network_manager.h - ESP-IDF component to work with WiFi
 * 
 * Include this header file to use the component.
 * 
 * (C) 2019 - Pablo Bacho <pablo@pablobacho.com>
 * This code is licensed under the MIT License.
 */

#ifndef _NETWORK_MANAGER_H_
#define _NETWORK_MANAGER_H_

#include <string.h>

#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"

#include "freertos/event_groups.h"

#include "settings_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NETWORK_MANAGER_NAMESPACE_CONFIG   "network"        /*!< Namespace to store settings of this module */

#define NETWORK_MANAGER_SSID_KEY           "ssid"           /*!< SSID entry key */
#define NETWORK_MANAGER_SSID_MAX_LENGTH    32               /*!< Maximum length of an SSID */
#define NETWORK_MANAGER_SSID_DEFAULT       ""               /*!< Default SSID to connect to */

#define NETWORK_MANAGER_PASSWORD_KEY           "password"   /*!< password entry key */
#define NETWORK_MANAGER_PASSWORD_MAX_LENGTH    63           /*!< Maximum password length */
#define NETWORK_MANAGER_PASSWORD_DEFAULT       ""           /*!< Default password of the SSID to connect to */

#define NETWORK_MANAGER_AP_SSID        "wifi-manager"       /*!< SSID to use when creating an AP */
#define NETWORK_MANAGER_AP_PASSWORD    "12345678"           /*!< Password of the AP created */

/**
 * Mode to start WiFi in
 */
typedef enum {
    AUTO = 0,   /*!< Auto will start in STATION mode if there is a valid SSID. Otherwise will create AP. */
    STA,        /*!< Start in STATION mode */
    AP          /*!< Create AP */
} network_manager_wifi_mode_t;

extern const char * NETWORK_MANAGER_NAMESPACE;  /*!< String to store namespace. Pointers to namespace name point to this variable. */
extern char network_manager_ssid[NETWORK_MANAGER_SSID_MAX_LENGTH +1]; /*!< String to store SSID to connect to in STATION mode */
extern char network_manager_password[NETWORK_MANAGER_PASSWORD_MAX_LENGTH +1]; /*!< String to store password of the SSID to connect to in STATION mode */

#define NETWORK_MANAGER_STATUS_CONNECTED    0b00000001
#define NETWORK_MANAGER_STATUS_GOT_IP       0b00000010
#define NETWORK_MANAGER_STATUS_AP_STARTED   0b00000100
extern uint8_t network_manager_status;

extern settings_manager_handle_t network_manager_settings_handle; /*!< settings_manager handle for this namespace */

ESP_EVENT_DECLARE_BASE(NETWORK_MANAGER_EVENT_BASE); /*!< BASE event of this module */

/**
 * List of possible events this module can trigger
 */
typedef enum {
    NETWORK_MANAGER_EVENT_WIFI_READY = 0,           /*!< ESP32 WiFi ready */
    NETWORK_MANAGER_EVENT_SCAN_DONE,                /*!< ESP32 finish scanning AP */
    NETWORK_MANAGER_EVENT_STA_START,                /*!< ESP32 station start */
    NETWORK_MANAGER_EVENT_STA_STOP,                 /*!< ESP32 station stop */
    NETWORK_MANAGER_EVENT_STA_CONNECTED,            /*!< ESP32 station connected to AP */
    NETWORK_MANAGER_EVENT_STA_DISCONNECTED,         /*!< ESP32 station disconnected from AP */
    NETWORK_MANAGER_EVENT_STA_AUTHMODE_CHANGE,      /*!< the auth mode of AP connected by ESP32 station changed */
    NETWORK_MANAGER_EVENT_STA_GOT_IP,               /*!< ESP32 station got IP from connected AP */
    NETWORK_MANAGER_EVENT_STA_LOST_IP,              /*!< ESP32 station lost IP and the IP is reset to 0 */
    NETWORK_MANAGER_EVENT_STA_WPS_ER_SUCCESS,       /*!< ESP32 station wps succeeds in enrollee mode */
    NETWORK_MANAGER_EVENT_STA_WPS_ER_FAILED,        /*!< ESP32 station wps fails in enrollee mode */
    NETWORK_MANAGER_EVENT_STA_WPS_ER_TIMEOUT,       /*!< ESP32 station wps timeout in enrollee mode */
    NETWORK_MANAGER_EVENT_STA_WPS_ER_PIN,           /*!< ESP32 station wps pin code in enrollee mode */
    NETWORK_MANAGER_EVENT_AP_START,                 /*!< ESP32 soft-AP start */
    NETWORK_MANAGER_EVENT_AP_STOP,                  /*!< ESP32 soft-AP stop */
    NETWORK_MANAGER_EVENT_AP_STACONNECTED,          /*!< a station connected to ESP32 soft-AP */
    NETWORK_MANAGER_EVENT_AP_STADISCONNECTED,       /*!< a station disconnected from ESP32 soft-AP */
    NETWORK_MANAGER_EVENT_AP_STAIPASSIGNED,         /*!< ESP32 soft-AP assign an IP to a connected station */
    NETWORK_MANAGER_EVENT_AP_PROBEREQRECVED,        /*!< Receive probe request packet in soft-AP interface */
    NETWORK_MANAGER_EVENT_GOT_IP6,                  /*!< ESP32 station or ap or ethernet interface v6IP addr is preferred */
    NETWORK_MANAGER_EVENT_ETH_START,                /*!< ESP32 ethernet start */
    NETWORK_MANAGER_EVENT_ETH_STOP,                 /*!< ESP32 ethernet stop */
    NETWORK_MANAGER_EVENT_ETH_CONNECTED,            /*!< ESP32 ethernet phy link up */
    NETWORK_MANAGER_EVENT_ETH_DISCONNECTED,         /*!< ESP32 ethernet phy link down */
    NETWORK_MANAGER_EVENT_ETH_GOT_IP,               /*!< ESP32 ethernet got IP from connected AP */
    NETWORK_MANAGER_EVENT_MAX
} network_manager_event_id_t;

/**
 * @brief   Initialize network_manager module.
 * 
 *          This function needs to be called after initializing settings_manager
 * 
 * @return  ESP_OK success
 *          ESP_FAIL error
 */
esp_err_t network_manager_init();

/**
 * @brief   Start WiFi and connect to network or create AP
 * 
 * @param   mode Mode to start the WiFi on: AUTO, STA or AP.
 * @return  ESP_OK success
 *          ESP_FAIL error
 */
esp_err_t network_manager_wifi_start(network_manager_wifi_mode_t mode);

/**
 * @brief   Start WiFi in station mode
 * 
 * @return  ESP_OK success
 *          ESP_FAIL error
 */
esp_err_t network_manager_wifi_start_station_mode();

/**
 * @brief   Start WiFi in AP mode
 * 
 * @return  ESP_OK success
 *          ESP_FAIL error
 */
esp_err_t network_manager_wifi_start_ap_mode();

/**
 * @brief   Stop WiFi
 * 
 * @return  ESP_OK success
 *          ESP_ERR_WIFI_NOT_INIT if wifi not initialized
 */
esp_err_t network_manager_wifi_stop();

/**
 * @brief   Check if device connected to WiFi network.
 * 
 * @return  Connection status
 */
bool network_manager_connected();

/**
 * @brief   Check if device got IP address.
 * 
 * @return  IP status
 */
bool network_manager_got_ip();

/**
 * @brief   Check if AP is started.
 * 
 * @return  AP status
 */
bool network_manager_is_ap();

/**
 * @brief   Event handler for network events.
 * 
 *          This function receives events from the legacy event system and posts them to the default event loop of the new event library.
 * 
 * @return  ESP_OK always succeeds although it does not mean the event was successfully posted
 */
esp_err_t network_manager_event_handler(void * context, system_event_t * event);

#ifdef __cplusplus
}
#endif

#endif // _SETTINGS_MANAGER_H_