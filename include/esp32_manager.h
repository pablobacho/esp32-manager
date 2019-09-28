/**
 * esp32_manager.h
 * 
 * (C) 2019 - Pablo Bacho <pablo@pablobacho.com>
 * This code is licensed under the MIT License.
 */

#ifndef _ESP32_MANAGER_H_
#define _ESP32_MANAGER_H_

#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"

#include "esp32_manager_storage.h"
#include "esp32_manager_network.h"
#include "esp32_manager_webconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Initialize esp32_manager
 * 
 * This function should be called only once.
 * 
 * @return  ESP_OK: success
 *          ESP_FAIL: error opening NVS
 *          ESP_ERR_NVS_PART_NOT_FOUND: NVS partition not found
 */
esp_err_t esp32_manager_init();

#ifdef __cplusplus
}
#endif

#endif // _ESP32_MANAGER_H_