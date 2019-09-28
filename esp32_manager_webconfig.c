/**
 * esp32_manager_webconfig.c
 * 
 * (C) 2019 - Pablo Bacho <pablo@pablobacho.com>
 * This code is licensed under the MIT License.
 */

 #include "esp32_manager_webconfig.h"

static const char * TAG = "esp32_manager_webconfig";

httpd_uri_t esp32_manager_webconfig_uris[];

httpd_handle_t esp32_manager_webconfig_webserver;

char esp32_manager_webconfig_content[];
char esp32_manager_webconfig_buffer[] = "";


esp_err_t esp32_manager_webconfig_init()
{
    esp_err_t e;

    // Create uris
    esp32_manager_webconfig_uris[WEBCONFIG_MANAGER_URI_ROOT_INDEX].uri        = WEBCONFIG_MANAGER_URI_ROOT_URL;
    esp32_manager_webconfig_uris[WEBCONFIG_MANAGER_URI_ROOT_INDEX].method     = HTTP_GET;
    esp32_manager_webconfig_uris[WEBCONFIG_MANAGER_URI_ROOT_INDEX].handler    = esp32_manager_webconfig_uri_handler_root;
    esp32_manager_webconfig_uris[WEBCONFIG_MANAGER_URI_ROOT_INDEX].user_ctx   = NULL;

    esp32_manager_webconfig_uris[WEBCONFIG_MANAGER_URI_CSS_INDEX].uri         = WEBCONFIG_MANAGER_URI_CSS_URL;
    esp32_manager_webconfig_uris[WEBCONFIG_MANAGER_URI_CSS_INDEX].method      = HTTP_GET;
    esp32_manager_webconfig_uris[WEBCONFIG_MANAGER_URI_CSS_INDEX].handler     = esp32_manager_webconfig_uri_handler_style;
    esp32_manager_webconfig_uris[WEBCONFIG_MANAGER_URI_CSS_INDEX].user_ctx    = NULL;

    esp32_manager_webconfig_uris[WEBCONFIG_MANAGER_URI_SETUP_INDEX].uri       = WEBCONFIG_MANAGER_URI_SETUP_URL;
    esp32_manager_webconfig_uris[WEBCONFIG_MANAGER_URI_SETUP_INDEX].method    = HTTP_GET;
    esp32_manager_webconfig_uris[WEBCONFIG_MANAGER_URI_SETUP_INDEX].handler   = esp32_manager_webconfig_uri_handler_setup;
    esp32_manager_webconfig_uris[WEBCONFIG_MANAGER_URI_SETUP_INDEX].user_ctx  = NULL;

    esp32_manager_webconfig_uris[WEBCONFIG_MANAGER_URI_GET_INDEX].uri         = WEBCONFIG_MANAGER_URI_GET_URL;
    esp32_manager_webconfig_uris[WEBCONFIG_MANAGER_URI_GET_INDEX].method      = HTTP_GET;
    esp32_manager_webconfig_uris[WEBCONFIG_MANAGER_URI_GET_INDEX].handler     = esp32_manager_webconfig_uri_handler_get;
    esp32_manager_webconfig_uris[WEBCONFIG_MANAGER_URI_GET_INDEX].user_ctx    = NULL;

    // Register events relevant to the webserver
    e = esp_event_handler_register(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_STA_GOT_IP, esp32_manager_webconfig_event_handler, NULL);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Registered for event STA_GOT_IP");
    } else {
        ESP_LOGE(TAG, "Error registering for event STA_GOT_IP");
        return ESP_FAIL;
    }

    esp_event_handler_register(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_STA_LOST_IP, esp32_manager_webconfig_event_handler, NULL);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Registered for event STA_LOST_IP");
    } else {
        ESP_LOGE(TAG, "Error registering for event STA_LOST_IP");
        return ESP_FAIL;
    }

    esp_event_handler_register(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_AP_START, esp32_manager_webconfig_event_handler, NULL);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Registered for event AP_START");
    } else {
        ESP_LOGE(TAG, "Error registering for event AP_START");
        return ESP_FAIL;
    }

    esp_event_handler_register(ESP32_MANAGER_NETWORK_EVENT_BASE, ESP32_MANAGER_NETWORK_EVENT_AP_STOP, esp32_manager_webconfig_event_handler, NULL);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Registered for event AP_STOP");
    } else {
        ESP_LOGE(TAG, "Error registering for event AP_STOP");
        return ESP_FAIL;
    }

    return ESP_OK;
}

httpd_handle_t esp32_manager_webconfig_webserver_start()
{
    esp_err_t e;

    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = WEBCONFIG_MANAGER_URIS_SIZE;

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    if(httpd_start(&server, &config) == ESP_OK) {
        // Register uris
        for(uint16_t i=0; i < WEBCONFIG_MANAGER_URIS_SIZE; ++i) {
            e = httpd_register_uri_handler(server, &esp32_manager_webconfig_uris[i]);
            if(e == ESP_OK) {
                ESP_LOGD(TAG, "Registered uri %s", esp32_manager_webconfig_uris[i].uri);
            } else {
                ESP_LOGE(TAG, "Error registering uri %s: %s", esp32_manager_webconfig_uris[i].uri, esp_err_to_name(e));
                esp32_manager_webconfig_webserver_stop(&server);
                return NULL;
            }
        }
        ESP_LOGD(TAG, "Webserver started");
    } else {
        ESP_LOGE(TAG, "Error starting webserver");
    }

    return server;
}

void esp32_manager_webconfig_webserver_stop(httpd_handle_t * server)
{
    if(*server != NULL) {
        httpd_stop(*server);
        *server = NULL;
        ESP_LOGD(TAG, "Webserver stopped");
    } else {
        ESP_LOGD(TAG, "Webserver was already stopped");
    }
}

void esp32_manager_webconfig_event_handler(void * handler_arg, esp_event_base_t base, int32_t id, void * event_data)
{
    if(base != ESP32_MANAGER_NETWORK_EVENT_BASE) {
        return;
    }

    switch(id) {
        case ESP32_MANAGER_NETWORK_EVENT_STA_GOT_IP:
        case ESP32_MANAGER_NETWORK_EVENT_AP_START:
            esp32_manager_webconfig_webserver = esp32_manager_webconfig_webserver_start();
        break;
        case ESP32_MANAGER_NETWORK_EVENT_STA_LOST_IP:
        case ESP32_MANAGER_NETWORK_EVENT_AP_STOP:
            esp32_manager_webconfig_webserver_stop(&esp32_manager_webconfig_webserver);
        break;
        default: break;
    }
}

esp_err_t esp32_manager_webconfig_uri_handler_root(httpd_req_t *req)
{
    esp_err_t e;

    // Calculate size of request query
    size_t recv_size = MIN(httpd_req_get_url_query_len(req)+1, sizeof(esp32_manager_webconfig_content)-1);
    ESP_LOGD(TAG, "Request header size: %d", recv_size);

    // Get request query
    e = httpd_req_get_url_query_str(req, esp32_manager_webconfig_content, recv_size);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Query string: %s", esp32_manager_webconfig_content);
        // Search for reboot key
        e = httpd_query_key_value(esp32_manager_webconfig_content, WEBCONFIG_MANAGER_URI_PARAM_REBOOT, esp32_manager_webconfig_buffer, sizeof(esp32_manager_webconfig_buffer));
        if(e == ESP_OK) { // Reboot parameter found in query
            e = esp32_manager_webconfig_page_reboot(esp32_manager_webconfig_buffer, req); // Generate HTML
            if(e == ESP_OK) {
                ESP_LOGD(TAG, "Reboot page generated");
            } else {
                ESP_LOGE(TAG, "Error generating reboot page");
                httpd_resp_set_status(req, HTTPD_500);
            }
            e = httpd_resp_send(req, esp32_manager_webconfig_buffer, strlen(esp32_manager_webconfig_buffer)); // Send it to client
            if(e == ESP_OK) {
                ESP_LOGD(TAG, "Page reboot response sent to client");
            } else {
                ESP_LOGE(TAG, "Error sending page reboot response to client");
            }
            ESP_LOGD(TAG, "Restarting in %d seconds", WEBCONFIG_MANAGER_REBOOT_DELAY / 1000);
            vTaskDelay(WEBCONFIG_MANAGER_REBOOT_DELAY / portTICK_PERIOD_MS); // Wait before rebooting
            esp_restart();
        }
    }

    e = esp32_manager_webconfig_page_root(esp32_manager_webconfig_buffer, req);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Root document generated");
    } else {
        ESP_LOGE(TAG, "Error generating root document");
        httpd_resp_set_status(req, HTTPD_500);
    }

    e = httpd_resp_send(req, esp32_manager_webconfig_buffer, strlen(esp32_manager_webconfig_buffer));
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Response sent");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Error sending response");
        return ESP_FAIL;
    }
}

esp_err_t esp32_manager_webconfig_uri_handler_style(httpd_req_t *req)
{
    esp_err_t e;
    
    e = httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
    e += httpd_resp_set_hdr(req, "Pragma", "no-cache");
    e += httpd_resp_set_hdr(req, "Expires", "0");
    if(e != ESP_OK) {
        ESP_LOGE(TAG, "Error settings cache headers");
    }

    e = httpd_resp_send(req, (const char *) style_min_css_start, style_min_css_end-style_min_css_start);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Response sent");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Error sending response");
        return ESP_FAIL;
    }
}

esp_err_t esp32_manager_webconfig_uri_handler_setup(httpd_req_t * req)
{
    esp_err_t e;

    // Calculate size of request query
    size_t recv_size = MIN(httpd_req_get_url_query_len(req)+1, sizeof(esp32_manager_webconfig_content)-1);
    ESP_LOGD(TAG, "Request header size: %d", recv_size);

    // Get request query
    e = httpd_req_get_url_query_str(req, esp32_manager_webconfig_content, recv_size);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Query string: %s", esp32_manager_webconfig_content);
        // Search for namespace key
        e = httpd_query_key_value(esp32_manager_webconfig_content, WEBCONFIG_MANAGER_URI_PARAM_NAMESPACE, esp32_manager_webconfig_buffer, sizeof(esp32_manager_webconfig_buffer));
        if(e == ESP_OK) { // Requesting namespace page
            // Get namespace
            esp32_manager_namespace_t * namespace = NULL;
            for(uint8_t i=0; i < ESP32_MANAGER_NAMESPACES_SIZE; ++i) {
                if(esp32_manager_namespaces[i] != NULL) {
                    if(!strcmp(esp32_manager_webconfig_buffer, esp32_manager_namespaces[i]->key)) {
                        namespace = esp32_manager_namespaces[i];
                        break;
                    }
                }
            }
            // if namespace requested exists
            if(namespace != NULL) {
                ESP_LOGD(TAG, "Selected namespace %s", namespace->key);
                // Check if there are settings to update
                uint16_t entry_updated = 0; // Flag to mark if any settings were changed
                for(uint16_t i=0; i < namespace->entries_size; ++i) {
                    esp32_manager_entry_t * entry = namespace->entries[i];
                    char encoded[100]; // FIXME Magic number
                    ESP_LOGD(TAG, "Searching for entry %s.%s", namespace->key, entry->key);
                    e = httpd_query_key_value(esp32_manager_webconfig_content, entry->key, encoded, sizeof(encoded));
                    if(e == ESP_OK) { // There is a setting to update
                        ESP_LOGD(TAG, "Value before decoding: %s", encoded);
                        esp32_manager_webconfig_urldecode(esp32_manager_webconfig_buffer, encoded); // Decode value from URL
                        ESP_LOGD(TAG, "Value after decoding: %s", esp32_manager_webconfig_buffer);
                        esp32_manager_webconfig_update_namespace_entry(entry, esp32_manager_webconfig_buffer);
                        ++entry_updated;
                        esp32_manager_entry_to_string(encoded, entry);
                        ESP_LOGD(TAG, "Updated entry %s.%s with value %s", namespace->key, entry->key, encoded);
                    } // Nothing to do if setting is not found in query string
                    
                }
                if(entry_updated > 0) {
                    e = esp32_manager_commit_to_nvs(namespace); // Commit changes to namespace
                    if(e == ESP_OK) {
                        ESP_LOGD(TAG, "Entries updated and commited to NVS");
                    } else {
                        ESP_LOGE(TAG, "Error commiting changes to NVS: %s", esp_err_to_name(e));
                    }
                }
                // Generate response
                ESP_LOGD(TAG, "Generating response");
                e = esp32_manager_webconfig_page_setup_namespace(esp32_manager_webconfig_buffer, req, namespace);
                if(e == ESP_OK) {
                    ESP_LOGD(TAG, "Setup namespace page generated");
                } else {
                    ESP_LOGE(TAG, "Error generating namespace page: %s", esp_err_to_name(e));
                }
            } else { // namespace does not exist
                ESP_LOGW(TAG, "Requested namespace does not exist");
                e = esp32_manager_webconfig_page_setup(esp32_manager_webconfig_buffer, req);
                if(e == ESP_OK) {
                    ESP_LOGD(TAG, "Setup page generated");
                } else {
                    ESP_LOGE(TAG, "Error generating setup page");
                    httpd_resp_set_status(req, HTTPD_500); // Set response to error 500
                }
            }
        } else if(e == ESP_ERR_NOT_FOUND) { // no namespace requested
            e = esp32_manager_webconfig_page_setup(esp32_manager_webconfig_buffer, req); // Return setup page
            if(e == ESP_OK) {
                ESP_LOGD(TAG, "Setup page generated");
            } else {
                ESP_LOGE(TAG, "Error generating setup page: %s", esp_err_to_name(e));
                httpd_resp_set_status(req, HTTPD_500); // Set response to error 500
            }
        } else {
            ESP_LOGE(TAG, "Error: query does not fit buffer: %s", esp_err_to_name(e));
            httpd_resp_set_status(req, HTTPD_500); // Set response to error 500
        }
    } else if(e == ESP_ERR_NOT_FOUND) { // there is no query string
        ESP_LOGD(TAG, "No query string. Returning setup page.");
        e = esp32_manager_webconfig_page_setup(esp32_manager_webconfig_buffer, req); // Return setup page
        if(e == ESP_OK) {
            ESP_LOGD(TAG, "Setup page generated");
        } else {
            ESP_LOGE(TAG, "Error generating setup page: %s", esp_err_to_name(e));
            httpd_resp_set_status(req, HTTPD_500); // Set response to error 500
        }
    } else {
        ESP_LOGE(TAG, "Error processing query string: %s", esp_err_to_name(e));
        httpd_resp_set_status(req, HTTPD_500); // Set response to error 500
    }
    
    e = httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
    e += httpd_resp_set_hdr(req, "Pragma", "no-cache");
    e += httpd_resp_set_hdr(req, "Expires", "0");
    if(e != ESP_OK) {
        ESP_LOGE(TAG, "Error settings cache headers");
    }

    e = httpd_resp_send(req, esp32_manager_webconfig_buffer, strlen(esp32_manager_webconfig_buffer));
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Response sent");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Error sending response");
        return ESP_FAIL;
    }
}

esp_err_t esp32_manager_webconfig_uri_handler_get(httpd_req_t * req)
{
    esp_err_t e;

    size_t recv_size = MIN(httpd_req_get_url_query_len(req)+1, sizeof(esp32_manager_webconfig_content)-1);
    ESP_LOGD(TAG, "Request header size: %d", recv_size);

    // Get request query
    e = httpd_req_get_url_query_str(req, esp32_manager_webconfig_content, recv_size);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Query string: %s", esp32_manager_webconfig_content);
        // Search for namespace key
        e = httpd_query_key_value(esp32_manager_webconfig_content, WEBCONFIG_MANAGER_URI_PARAM_NAMESPACE, esp32_manager_webconfig_buffer, sizeof(esp32_manager_webconfig_buffer));
        if(e == ESP_OK) { // Requesting namespace page
            // Get namespace handle
            esp32_manager_namespace_t * namespace = NULL;
            for(uint8_t i=0; i < ESP32_MANAGER_NAMESPACES_SIZE; ++i) {
                if(esp32_manager_namespaces[i] != NULL) {
                    if(!strcmp(esp32_manager_webconfig_buffer, esp32_manager_namespaces[i]->key)) {
                        namespace = esp32_manager_namespaces[i];
                        break;
                    }
                }
            }
            // if requested namespace exists
            if(namespace != NULL) {
                e = httpd_query_key_value(esp32_manager_webconfig_content, WEBCONFIG_MANAGER_URI_PARAM_ENTRY, esp32_manager_webconfig_buffer, sizeof(esp32_manager_webconfig_buffer));
                esp32_manager_entry_t * entry = NULL;
                for(uint8_t i=0; i < namespace->entries_size; ++i) {
                    if(!strcmp(esp32_manager_webconfig_buffer, namespace->entries[i]->key)) {
                        entry = namespace->entries[i];
                        break;
                    }
                }
                // if requested entry exists
                if(entry != NULL) {
                    // Print raw value on response buffer
                    esp32_manager_entry_to_string(esp32_manager_webconfig_buffer, entry);
                    ESP_LOGD(TAG, "Response content: %s", esp32_manager_webconfig_buffer);                 
                } else {
                    ESP_LOGE(TAG, "Requested namespace does not exist");
                    strcpy(esp32_manager_webconfig_buffer, "ERROR: Requested setting does not exist");
                    httpd_resp_set_status(req, HTTPD_404);
                }
            } else { // namespace does not exist
                ESP_LOGE(TAG, "Requested namespace does not exist");
                strcpy(esp32_manager_webconfig_buffer, "ERROR: Requested namespace does not exist");
                httpd_resp_set_status(req, HTTPD_404);
            }
        } else if(e == ESP_ERR_NOT_FOUND) { // no namespace requested
            ESP_LOGE(TAG, "No namespace found");
            strcpy(esp32_manager_webconfig_buffer, "ERROR: No namespace found");
            httpd_resp_set_status(req, HTTPD_404);
        } else {
            ESP_LOGE(TAG, "Error: query does not fit buffer: %s", esp_err_to_name(e));
            strcpy(esp32_manager_webconfig_buffer, "ERROR: Query does not fit buffer");
            httpd_resp_set_status(req, HTTPD_500);
        }
    } else {
        ESP_LOGE(TAG, "Error: no query string");
        httpd_resp_set_status(req, HTTPD_404);
    }
    
    e = httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
    e += httpd_resp_set_hdr(req, "Pragma", "no-cache");
    e += httpd_resp_set_hdr(req, "Expires", "0");
    if(e != ESP_OK) {
        ESP_LOGE(TAG, "Error settings cache headers");
    }

    e = httpd_resp_send(req, esp32_manager_webconfig_buffer, strlen(esp32_manager_webconfig_buffer));
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Response sent");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Error sending response");
        return ESP_FAIL;
    }
}

esp_err_t esp32_manager_webconfig_page_root(char * buffer, httpd_req_t * req)
{
    if(req == NULL || buffer == NULL) {
        ESP_LOGE(TAG, "req and buffer cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // FIXME Buffer overrun protection

    // TODO Show featured values on home page
    // For now it is just a link to setup

    strcpy(buffer, "<html><head><link rel=\"stylesheet\" href=\"style.min.css\" /><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />");
    strcat(buffer, WEBCONFIG_MANAGER_WEB_TITLE);
    strcat(buffer, "</head><body><p><a class=\"button\" href=\"/setup\">Setup</a></a></body>");

    return ESP_OK;
}

esp_err_t esp32_manager_webconfig_page_reboot(char * buffer, httpd_req_t * req)
{
    if(req == NULL || buffer == NULL) {
        ESP_LOGE(TAG, "req and buffer cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // FIXME Buffer overrun protection

    strcpy(buffer, "<html><head><link rel=\"stylesheet\" href=\"style.min.css\" /><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />");
    strcat(buffer, WEBCONFIG_MANAGER_WEB_TITLE);
    strcat(buffer, "</head><body>Rebooting device. Wait 10 seconds before clicking <a href=\"/setup\">back</a> (Link might not work if network settings were changed)</body>");
    
    return ESP_OK;
}

esp_err_t esp32_manager_webconfig_page_setup(char * buffer, httpd_req_t * req)
{
    if(req == NULL || buffer == NULL) {
        ESP_LOGE(TAG, "req and buffer cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // FIXME Buffer overrun protection
    ESP_LOGD(TAG, "Generating setup page");

    strcpy(buffer, "<html><head><link rel=\"stylesheet\" href=\"style.min.css\" /><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />");
    strcat(buffer, WEBCONFIG_MANAGER_WEB_TITLE);
    strcat(buffer, "</head><body><ul>");
    for(uint8_t i=0; i < ESP32_MANAGER_NAMESPACES_SIZE; ++i) {
        if(esp32_manager_namespaces[i] != NULL) {
            strcat(buffer, "<li><a href=\"/setup?namespace=");
            strcat(buffer, esp32_manager_namespaces[i]->key);
            strcat(buffer, "\">");
            strcat(buffer, esp32_manager_namespaces[i]->friendly);
            strcat(buffer, "</a></li>");
        }
    }
    strcat(buffer, "</ul><a class=\"button\" href=\"/?reboot=1\">Reboot device</a></body></html>");

    return ESP_OK;
}

esp_err_t esp32_manager_webconfig_page_setup_namespace(char * buffer, httpd_req_t * req, esp32_manager_namespace_t * namespace)
{
    if(req == NULL || buffer == NULL || namespace == NULL) {
        ESP_LOGE(TAG, "Invalid arguments");
        return ESP_ERR_INVALID_ARG;
    }

    // FIXME Buffer overrun protection

    strcpy(buffer, "<html><head><link rel=\"stylesheet\" href=\"style.min.css\" media=\"screen\" /><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />");
    strcat(buffer, WEBCONFIG_MANAGER_WEB_TITLE);
    strcat(buffer, "</head><body><form method=\"get\" action=\"/setup\"><br/><input name=\"namespace\" type=\"hidden\" value=\"");
    strcat(buffer, namespace->key);
    strcat(buffer, "\"><br/>");
    
    for(uint16_t i=0; i < namespace->entries_size; ++i) {
        if(namespace->entries[i] == NULL) continue;

        esp32_manager_entry_t * entry = namespace->entries[i];
        char value[50]; // FIXME Remove this magic number
        strcat(buffer, entry->friendly);
        strcat(buffer, "<br/><input type=\"");

        switch(entry->type) {
            case i8:
            case i16:
            case i32:
            case i64:
            case u8:
            case u16:
            case u32:
            case u64:
            case flt:
            case dbl:
                strcat(buffer, "number\" value=\"");
                esp32_manager_entry_to_string(value, entry);
                strcat(buffer, value);
                break;
            case text: // This type needs to be null-terminated
                strcat(buffer, "text\" value=\"");
                strcat(buffer, (char *) entry->value);
                break;
            case password:
                strcat(buffer, "password\" value=\"");
                strcat(buffer, (char *) entry->value);
                break;
            // TODO Implement these cases
            case single_choice:
            case multiple_choice:
            case blob: // Data structures, binary and other non-null-terminated types go here
            case image:
                ESP_LOGE(TAG, "Not implemented");
                //nvs_set_blob(handle->nvs_handle, entry->key, entry->value, size);
            break;
            default:
                ESP_LOGE(TAG, "Entry %s.%s is of an unknown type", namespace->key, entry->key);
            break;
        }
        strcat(buffer, "\" name=\"");
        strcat(buffer, entry->key);
        strcat(buffer, "\"/><br/>");
    }

    strcat(buffer, "<input type=\"submit\" value=\"submit\"></form><a href=\"/setup\">Back</a></body></html>");

    return ESP_OK;
}

esp_err_t esp32_manager_webconfig_update_namespace_entry(esp32_manager_entry_t * entry, const char * value_str)
{
    if(entry == NULL || value_str == NULL) {
        ESP_LOGE(TAG, "entry and value_str cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    uint64_t unumber;
    int64_t inumber;

    switch(entry->type) {
        case i8:
            inumber = atoi(value_str);
            *((int8_t *) entry->value) = (int8_t) inumber;
            break;
        case i16:
            inumber = atoi(value_str);
            *((int16_t *) entry->value) = (int16_t) inumber;
            break;
        case i32:
            inumber = atoi(value_str);
            *((int32_t *) entry->value) = (int32_t) inumber;
            break;
        case i64:
            inumber = atoi(value_str);
            *((int64_t *) entry->value) = (int64_t) inumber;
            break;
        case u8:
            unumber = atoi(value_str);
            *((uint8_t *) entry->value) = (uint8_t) unumber;
            break;
        case u16:
            unumber = atoi(value_str);
            *((uint16_t *) entry->value) = (uint16_t) unumber;
            break;
        case u32:
            unumber = atoi(value_str);
            *((uint32_t *) entry->value) = (uint32_t) unumber;
            break;
        case u64:
            unumber = atoi(value_str);
            *((uint64_t *) entry->value) = (uint64_t) unumber;
            break;
        case text: // This type needs to be null-terminated
            strcpy((char *) entry->value, value_str);
            break;
        case password:
            strcpy((char *) entry->value, value_str);
            break;
        // TODO Implement these cases
        case flt:
        case dbl:
        case single_choice:
        case multiple_choice:
        case blob: // Data structures, binary and other non-null-terminated types go here
        case image:
            ESP_LOGE(TAG, "Not implemented");
            //nvs_set_blob(handle->nvs_handle, entry->key, entry->value, size);
        break;
        default:
            ESP_LOGE(TAG, "Entry %s is of an unknown type", entry->key);
            return ESP_FAIL;
        break;
    }

    return ESP_OK;
}

int32_t esp32_manager_webconfig_urldecode(char *__restrict__ dest, const char *__restrict__ src)
{
    char * index;
	const char * end = src + strlen(src);
	unsigned int c;
 
	for (index = dest; src <= end; index++) {
		c = *src++;
		if (c == '+') { // Replace '+' with spaces ' '
            c = ' ';
        } else if (c == '%' && (!WEBCONFIG_MANAGER_ISHEX(*src++) || !WEBCONFIG_MANAGER_ISHEX(*src++) || !sscanf(src - 2, "%2x", &c))) {
            // If found a %, next two characters need to be hexadecimal. Read both as a character value. If not they are not, throw an error.
			return -1;
        }
 		if (dest) {
            *index = c; // Copy decoded char
        }
	}
 
	return index - dest; // Return size of decoded string
}