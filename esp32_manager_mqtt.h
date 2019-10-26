/**
 * esp32_manager_mqtt.h
 *
 * (C) 2019 - Pablo Bacho <pablo@pablobacho.com>
 * This code is licensed under the MIT License.
 */

#ifndef _ESP32_MANAGER_MQTT_H_
#define _ESP32_MANAGER_MQTT_H_

#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include "esp32_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief  Parameters for the namespace */
#define ESP32_MANAGER_MQTT_NAMESPACE_KEY        "mqtt"
#define ESP32_MANAGER_MQTT_NAMESPACE_FRIENDLY   "MQTT"
extern esp32_manager_namespace_t esp32_manager_mqtt_namespace;  /*!< Struct holding namespace information */

/** @brief  Parameters for the Broker URL entry */
#define ESP32_MANAGER_MQTT_BROKER_URL_KEY           "broker_url"
#define ESP32_MANAGER_MQTT_BROKER_URL_FRIENDLY      "Broker URL"
#define ESP32_MANAGER_MQTT_BROKER_URL_MAX_LENGTH    64
#define ESP32_MANAGER_MQTT_BROKER_URL_DEFAULT       "mqtt://test.mosquitto.org"
extern char esp32_manager_mqtt_broker_url[ESP32_MANAGER_MQTT_BROKER_URL_MAX_LENGTH]; /*!< Variable to store the broker url */
extern esp32_manager_entry_t esp32_manager_mqtt_entry_broker_url; /*!< Struct holding broker url entry information */

/** @brief  Array grouping all entries */
#define ESP32_MANAGER_MQTT_NAMESPACE_SIZE           1
extern esp32_manager_entry_t * esp32_manager_mqtt_entries[ESP32_MANAGER_MQTT_NAMESPACE_SIZE];

/** @brief  MQTT handlers and parameters */
#define ESP32_MANAGER_MQTT_TOPIC_MAX_LENGTH     255 // FIXME Base this number on slashes, hostname, namespace and entries max length
extern esp_mqtt_client_handle_t esp32_manager_mqtt_client;

/**
 * @brief   Initialize esp32_manager_mqtt
 *
 * This function should be called only once.
 *
 * @return  ESP_OK: success
 *          ESP_FAIL: error
 */
esp_err_t esp32_manager_mqtt_init();

/**
 * @brief   Publish entry value
 *
 *          Publishes entry value in raw format under topic /[hostname]/[namespace.key]/[entry.key]
 *
 * @return  ESP_OK: success
 *          ESP_FAIL: error
 */
esp_err_t esp32_manager_mqtt_publish_entry(esp32_manager_namespace_t * namespace, esp32_manager_entry_t * entry);

/**
 * @brief   Event handler for mqtt legacy event loop
 *
 *          Not to be called directly.
 *
 * @param   event
 * @return  ESP_OK: success
 *          ESP_FAIL: error
 */
esp_err_t esp32_manager_mqtt_event_handler(esp_mqtt_event_handle_t event);

/**
 * @brief   Event handler for system event loop for network events
 *
 *          Not to be called directly
 *
 * @param   handler_arg
 * @param   base EVENT_BASE as defined on esp_event component
 * @param   id EVENT_ID as defined on esp_event component
 * @param   event_data
 */
void esp32_manager_mqtt_system_event_handler(void * handler_arg, esp_event_base_t base, int32_t id, void * event_data);

/**
 * @brief   esp32_manager callback to update MQTT broker url from string
 *
 * @param   entry pointer to entry
 * @param   source string encoded new value
 * @return  ESP_OK success
 *          ESP_FAIL error
 */
esp_err_t esp32_manager_mqtt_entry_broker_url_from_string(esp32_manager_entry_t * entry, char * source);

#ifdef __cplusplus
}
#endif

#endif // _ESP32_MANAGER_MQTT_H_