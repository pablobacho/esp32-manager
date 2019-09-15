/**
 * settings_manager.h - ESP-IDF component to work with settings
 * 
 * Include this header file to use the component.
 * 
 * (C) 2019 - Pablo Bacho <pablo@pablobacho.com>
 * This code is licensed under the MIT License.
 */

#ifndef _SETTINGS_MANAGER_H_
#define _SETTINGS_MANAGER_H_

#include <string.h>

#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SETTINGS_MANAGER_NAMESPACE_MAX_SIZE  10     /*!< Maximum number of namespaces that can be registered */
#define SETTINGS_MANAGER_ENTRIES_MAX_SIZE    100    /*!< Maximum number of settings that can be registered */
  
#define SETTINGS_MANAGER_ATTR_READ         0b00000001  /*!< READ flag */
#define SETTINGS_MANAGER_ATTR_WRITE        0b00000010  /*!< WRITE flag */
#define SETTINGS_MANAGER_ATTR_READWRITE    0b00000011  /*!< READ & WRITE attributes. Meant for readability of the code because of both being commonly used together. */

/**
 * Settings type.
 */
typedef enum {i8=0, u8=1, i16=2, u16=3, i32=4, u32=5, i64=6, u64=7,
        multiple_choice=20, single_choice=21,
        text=40, password=41,
        blob=60, image=61} settings_manager_type_t;

/**
 * Namespace handle.
 */
typedef struct {
    uint8_t id;             /*!< Namespace id. Should be the same as position in the namespace array. */
    const char * namespace; /*!< Namespace name */
    nvs_handle nvs_handle;  /*!< NVS handle for this namespace */
} settings_manager_namespace_t;

/**
 * Definition of the settings_manager handle
 */
typedef settings_manager_namespace_t * settings_manager_handle_t;

/**
 * Settings entry
 */
typedef struct {
    uint8_t namespace_id;           /*!< id of the namespace this entry belongs to */
    const char * key;               /*!< unique key to identify the entry */
    settings_manager_type_t type;   /*!< type */
    void * value;                   /*!< pointer to the variable where the value of the setting is stored */
    uint32_t attributes;                 /*!< attributes */
} settings_manager_entry_t;

/**
 * Array to store namespace objects
 */
extern settings_manager_namespace_t settings_manager_namespaces[SETTINGS_MANAGER_NAMESPACE_MAX_SIZE];
extern uint8_t settings_manager_namespaces_size; /*!< Number of namespaces registered */

/**
 * Array to store entry objects
 */
extern settings_manager_entry_t settings_manager_entries[SETTINGS_MANAGER_ENTRIES_MAX_SIZE];
extern uint16_t settings_manager_entries_size; /*!< Number of entries registered */

/**
 * @brief   Initialize settings_manager
 * 
 * This function should be called only once, before initializing settings_manager and network_manager.
 * 
 * @return  ESP_OK: success
 *          ESP_FAIL: error opening NVS
 *          ESP_ERR_NVS_PART_NOT_FOUND: NVS partition not found
 */
esp_err_t settings_manager_init();

/**
 * @brief   Register namespace with settings_manager
 * 
 * @param   namespace name of the namespace
 * @return  handle to the namespace registered or null for error
 */
settings_manager_handle_t settings_manager_register_namespace(const char * namespace);

/**
 * @brief   Register settings entry
 * 
 * @param   handle namespace handle the entry belongs to
 * @param   key unique name of the setting
 * @param   type type of the value to be stored
 * @param   value pointer to the variable where the value is
 * @param   attributes attributes for this entry
 * @return  ESP_OK success
 *          ESP_ERR_NO_MEM no empty slots available for this entry
 *          ESP_ERR_INVALID_ARG namespace handle is not valid
 */
esp_err_t settings_manager_register_setting(settings_manager_handle_t handle, const char * key, settings_manager_type_t type, void * value, uint32_t attributes);

/**
 * @brief   Commits all settings under a namespace to NVS for permanent storage
 * 
 * @param   handle namespace handle
 * @return  ESP_OK success
 *          ESP_FAIL error
 *          ESP_ERR_INVALID_ARG invalid handle
 */
esp_err_t settings_manager_commit_to_nvs(settings_manager_handle_t handle);

/**
 * @brief   Read all settings under a namespace from NVS
 * 
 * @param   handle namespace handle
 * @return  ESP_OK success
 *          ESP_FAIL error
 *          ESP_ERR_INVALID_ARG invalid handle
 */
esp_err_t settings_manager_read_from_nvs(settings_manager_handle_t handle);

/**
 * @brief   Convert a setting value to string
 * 
 * @param   dest Destination buffer
 * @param   entry pointer to the settings entry
 * @return  ESP_OK success
 *          ESP_FAIL error
 *          ESP_ERR_INVALID_ARG for invalid arguments
 */
esp_err_t settings_manager_entry_to_string(char * dest, settings_manager_entry_t * entry);

#ifdef __cplusplus
}
#endif

#endif // _SETTINGS_MANAGER_H_