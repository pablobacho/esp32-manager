/**
 * esp32_manager_webconfig.h
 *
 * (C) 2019 - Pablo Bacho <pablo@pablobacho.com>
 * This code is licensed under the MIT License.
 */

#ifndef _ESP32_MANAGER_WEBCONFIG_MANAGER_H_
#define _ESP32_MANAGER_WEBCONFIG_MANAGER_H_

#include "esp_http_server.h"

#include "esp32_manager.h"
#include "esp32_manager_network.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WEBCONFIG_MANAGER_WEB_TITLE             CONFIG_ESP32_MANAGER_WEBCONFIG_TITLE   /*!< Title. Will be shown on generated pages. */

#define WEBCONFIG_MANAGER_NAMESPACE_KEY         "webconfig"
#define WEBCONFIG_MANAGER_NAMESPACE_FRIENDLY    "Web configuration"

#define WEBCONFIG_MANAGER_URI_ROOT_INDEX    0           /*!< Position of the root uri in the uris array*/
#define WEBCONFIG_MANAGER_URI_ROOT_URL      "/"         /*!< uri of the root document */
extern httpd_uri_t esp32_manager_webconfig_uri_root;
#define WEBCONFIG_MANAGER_URI_CSS_INDEX     1           /*!< Position of the CSS file uri in the uris array*/
#define WEBCONFIG_MANAGER_URI_CSS_URL       "/style.min.css" /*!< uri of the CSS file */
extern httpd_uri_t esp32_manager_webconfig_uri_css;
#define WEBCONFIG_MANAGER_URI_SETUP_INDEX   2           /*!< Position of the setup uri in the uris array */
#define WEBCONFIG_MANAGER_URI_SETUP_URL     "/setup"    /*!< Uri of the setup page */
extern httpd_uri_t esp32_manager_webconfig_uri_setup;
#define WEBCONFIG_MANAGER_URI_GET_INDEX     3           /*!< Position of the get uri in the uris array */
#define WEBCONFIG_MANAGER_URI_GET_URL       "/get"      /*!< uri of the get page */
extern httpd_uri_t esp32_manager_webconfig_uri_get;
#define WEBCONFIG_MANAGER_URI_FACTORY_INDEX 4           /*!< Position of the factory uri in the uris array */
#define WEBCONFIG_MANAGER_URI_FACTORY_URL   "/factory"  /*!< uri of the factory page */
#define WEBCONFIG_MANAGER_URIS_SIZE         5   /*!< Number of uris that will be registered */
extern httpd_uri_t * esp32_manager_webconfig_uris[WEBCONFIG_MANAGER_URIS_SIZE]; /*!< Array to store uris */

#define WEBCONFIG_MANAGER_URI_PARAM_NAMESPACE       "namespace" /*!< Query key to select namespace using the get uri */
#define WEBCONFIG_MANAGER_URI_PARAM_ENTRY           "entry"     /*!< Query key to request a setting using the get uri */
#define WEBCONFIG_MANAGER_URI_PARAM_REBOOT_DEVICE   "reboot"    /*!< Query key of the parameter for requesting a reboot */
#define WEBCONFIG_MANAGER_URI_PARAM_FACTORY_RESET   "factory_reset" /*!< Query key of the parameter for requesting a factory reset */
#define WEBCONFIG_MANAGER_URI_PARAM_CONFIRM         "confirm"   /*!< Query key for confirmation of factory requests (such as reboot or factory reset) */

#define WEBCONFIG_MANAGER_REBOOT_DELAY      3000            /*!< Delay between serving the reboot page and rebooting the device */

#define WEBCONFIG_MANAGER_RESPONSE_BUFFER_MAX_LENGTH    10240    /*!< Maximum lenght of an HTTP response */

extern char esp32_manager_webconfig_content[CONFIG_HTTPD_MAX_REQ_HDR_LEN +1]; /*!< Buffer to store requests' content */
extern char esp32_manager_webconfig_buffer[WEBCONFIG_MANAGER_RESPONSE_BUFFER_MAX_LENGTH +1]; /*!< Buffer to store responses */

/** @brief  Milligram CSS file */
extern const uint8_t style_min_css_start[] asm("_binary_style_min_css_start");
extern const uint8_t style_min_css_end[] asm("_binary_style_min_css_end");

/**
 * @brief   Initialize esp32_manager_webconfig
 *
 * This function should be called only once, after initializing esp32_manager and network_manager.
 *
 * @return  ESP_OK: success
 *          ESP_FAIL: error
 */
esp_err_t esp32_manager_webconfig_init();

/**
 * @brief   Start webserver and register URI handlers
 *
 * @return  Webserver handle or NULL for error
 */
httpd_handle_t esp32_manager_webconfig_webserver_start();

/**
 * @brief   Stop webserver
 *
 * @param   server: Pointer to the server handle.
 */
void esp32_manager_webconfig_webserver_stop(httpd_handle_t * server);

/**
 * @brief   Processing events for esp32_manager_webconfig
 *
 * @param   handler_arg
 * @param   base EVENT_BASE as defined on esp_event component
 * @param   id EVENT_ID as defined on esp_event component
 * @param   event_data
 */
void esp32_manager_webconfig_event_handler(void * handler_arg, esp_event_base_t base, int32_t id, void * event_data);

/**
 * @brief   Handler to call when root document is requested ("/")
 *
 * @param   req Pointer to the request handle
 * @return  ESP_OK: success
 *          ESP_FAIL: error
 */
esp_err_t esp32_manager_webconfig_uri_handler_root(httpd_req_t *req);

/**
 * @brief   Handler to call when CSS stylesheet is requested ("/res/style.min.css")
 *
 * @param   req Pointer to the request handle
 * @return  ESP_OK: success
 *          ESP_FAIL: error
 */
esp_err_t esp32_manager_webconfig_uri_handler_style(httpd_req_t *req);


/**
 * @brief   Handler to call when setup page is requested
 *
 * @param   req Pointer to the request handle
 * @return  ESP_OK: success
 *          ESP_FAIL: error
 */
esp_err_t esp32_manager_webconfig_uri_handler_setup(httpd_req_t * req);

/**
 * @brief   Handler to call when get page is requested
 *
 * @param   req Pointer to the request handle
 * @return  ESP_OK: success
 *          ESP_FAIL: error
 */
esp_err_t esp32_manager_webconfig_uri_handler_get(httpd_req_t * req);

/**
 * @brief   Handler to call when factory page is requested
 *
 * @param   req Pointer to the request handle
 * @return  ESP_OK: success
 *          ESP_FAIL: error
 */
esp_err_t esp32_manager_webconfig_uri_handler_factory(httpd_req_t * req);

/**
 * @brief   Generates HTML code for root document
 *
 * @param   buffer String buffer to store the HTML generated
 * @param   req Pointer to the request handle
 * @return  ESP_OK: success
 *          ESP_ERR_INVALID_ARG: buffer or req are not valid
 */
esp_err_t esp32_manager_webconfig_page_root(char * buffer, httpd_req_t * req, size_t buffer_size);

/**
 * @brief   Generates HTML code for reboot page
 *
 * @param   buffer String buffer to store the HTML generated
 * @param   req Pointer to the request handle
 * @return  ESP_OK: success
 *          ESP_ERR_INVALID_ARG: buffer or req are not valid
 */
esp_err_t esp32_manager_webconfig_page_reboot(char * buffer, httpd_req_t * req, size_t buffer_size);

/**
 * @brief   Generates HTML code for setup page
 *
 * @param   buffer String buffer to store the HTML generated
 * @param   req Pointer to the request handle
 * @return  ESP_OK: success
 *          ESP_ERR_INVALID_ARG: buffer or req are not valid
 */
esp_err_t esp32_manager_webconfig_page_setup(char * buffer, httpd_req_t * req, size_t buffer_size);

/**
 * @brief   Generates HTML code for setup namespace page
 *
 * @param   buffer String buffer to store the HTML generated
 * @param   req Pointer to the request handle
 * @param   namespace Namespace to be edited
 * @return  ESP_OK: success
 *          ESP_ERR_INVALID_ARG: buffer, req or namespace are not valid
 */
esp_err_t esp32_manager_webconfig_page_setup_namespace(char * buffer, httpd_req_t * req, esp32_manager_namespace_t * namespace, size_t buffer_size);

/**
 * @brief   Generates the right HTML form input code according to entry type
 *
 * @param   entry Pointer to entry
 * @param   dest Output buffer
 * @return  ESP_OK success
 *          ESP_FAIL error
 */
esp_err_t esp32_manager_webconfig_html_form_widget_default(char * dest, esp32_manager_entry_t * entry, size_t buffer_size);

/**
 * @brief   Starts the task that generates a delayed reboot so the webserver can finish up serving http requests
 *
 * @param   delay number of milliseconds to delay reboot
 * @return  ESP_OK success
 *          ESP_FAIL error
 */
esp_err_t esp32_manager_webconfig_deferred_reboot(uint32_t delay);

/**
 * @brief   Task that generates a delayed reboot so the webserver can finish up serving http requests
 *
 * @param   delay number of milliseconds to delay reboot as an uint32_t (value, not pointer)
 * @return  ESP_OK success
 *          ESP_FAIL error
 */
void esp32_manager_webconfig_deferred_reboot_task(void * pvParameter);

/**
 * @brief   Decode parameter value from an encoded URL query string
 *
 * @param   dest Buffer for the decoded string
 * @param   src Encoded value extracted from the query string
 * @return  ESP_OK decoded OK
 *          ESP_FAIL string format error
 */
esp_err_t esp32_manager_webconfig_urldecode(char *__restrict__ dest, const char *__restrict__ src);

#ifdef __cplusplus
}
#endif

#endif // _ESP32_MANAGER_WEBCONFIG_MANAGER_H_