/**
 * webconfig_manager.c - ESP-IDF component to work with web configuration
 * 
 * Implementation.
 * 
 * (C) 2019 - Pablo Bacho <pablo@pablobacho.com>
 * This code is licensed under the MIT License.
 */

 #include "webconfig_manager.h"

static const char * TAG = "webconfig_manager";

httpd_uri_t webconfig_manager_uris[];

httpd_handle_t webconfig_manager_webserver;

char webconfig_manager_content[];
char webconfig_manager_buffer[] = "";


esp_err_t webconfig_manager_init()
{
    esp_err_t e;

    // Create uris
    webconfig_manager_uris[WEBCONFIG_MANAGER_URI_ROOT_INDEX].uri        = WEBCONFIG_MANAGER_URI_ROOT_URL;
    webconfig_manager_uris[WEBCONFIG_MANAGER_URI_ROOT_INDEX].method     = HTTP_GET;
    webconfig_manager_uris[WEBCONFIG_MANAGER_URI_ROOT_INDEX].handler    = webconfig_manager_uri_handler_root;
    webconfig_manager_uris[WEBCONFIG_MANAGER_URI_ROOT_INDEX].user_ctx   = NULL;

    webconfig_manager_uris[WEBCONFIG_MANAGER_URI_SETUP_INDEX].uri       = WEBCONFIG_MANAGER_URI_SETUP_URL;
    webconfig_manager_uris[WEBCONFIG_MANAGER_URI_SETUP_INDEX].method    = HTTP_GET;
    webconfig_manager_uris[WEBCONFIG_MANAGER_URI_SETUP_INDEX].handler   = webconfig_manager_uri_handler_setup;
    webconfig_manager_uris[WEBCONFIG_MANAGER_URI_SETUP_INDEX].user_ctx  = NULL;

    webconfig_manager_uris[WEBCONFIG_MANAGER_URI_GET_INDEX].uri         = WEBCONFIG_MANAGER_URI_GET_URL;
    webconfig_manager_uris[WEBCONFIG_MANAGER_URI_GET_INDEX].method      = HTTP_GET;
    webconfig_manager_uris[WEBCONFIG_MANAGER_URI_GET_INDEX].handler     = webconfig_manager_uri_handler_get;
    webconfig_manager_uris[WEBCONFIG_MANAGER_URI_GET_INDEX].user_ctx    = NULL;

    // Register events relevant to the webserver
    e = esp_event_handler_register(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_STA_GOT_IP, webconfig_manager_event_handler, NULL);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Registered for event STA_GOT_IP");
    } else {
        ESP_LOGE(TAG, "Error registering for event STA_GOT_IP");
        return ESP_FAIL;
    }

    esp_event_handler_register(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_STA_LOST_IP, webconfig_manager_event_handler, NULL);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Registered for event STA_LOST_IP");
    } else {
        ESP_LOGE(TAG, "Error registering for event STA_LOST_IP");
        return ESP_FAIL;
    }

    esp_event_handler_register(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_AP_START, webconfig_manager_event_handler, NULL);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Registered for event AP_START");
    } else {
        ESP_LOGE(TAG, "Error registering for event AP_START");
        return ESP_FAIL;
    }

    esp_event_handler_register(NETWORK_MANAGER_EVENT_BASE, NETWORK_MANAGER_EVENT_AP_STOP, webconfig_manager_event_handler, NULL);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Registered for event AP_STOP");
    } else {
        ESP_LOGE(TAG, "Error registering for event AP_STOP");
        return ESP_FAIL;
    }

    return ESP_OK;
}

httpd_handle_t webconfig_manager_webserver_start()
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
            e = httpd_register_uri_handler(server, &webconfig_manager_uris[i]);
            if(e == ESP_OK) {
                ESP_LOGD(TAG, "Registered uri %s", webconfig_manager_uris[i].uri);
            } else {
                ESP_LOGE(TAG, "Error registering uri %s: %s", webconfig_manager_uris[i].uri, esp_err_to_name(e));
                webconfig_manager_webserver_stop(&server);
                return NULL;
            }
        }
        ESP_LOGD(TAG, "Webserver started");
    } else {
        ESP_LOGE(TAG, "Error starting webserver");
    }

    return server;
}

void webconfig_manager_webserver_stop(httpd_handle_t * server)
{
    if(*server != NULL) {
        httpd_stop(*server);
        *server = NULL;
        ESP_LOGD(TAG, "Webserver stopped");
    } else {
        ESP_LOGD(TAG, "Webserver was already stopped");
    }
}

void webconfig_manager_event_handler(void * handler_arg, esp_event_base_t base, int32_t id, void * event_data)
{
    if(base != NETWORK_MANAGER_EVENT_BASE) {
        return;
    }

    switch(id) {
        case NETWORK_MANAGER_EVENT_STA_GOT_IP:
        case NETWORK_MANAGER_EVENT_AP_START:
            webconfig_manager_webserver = webconfig_manager_webserver_start();
        break;
        case NETWORK_MANAGER_EVENT_STA_LOST_IP:
        case NETWORK_MANAGER_EVENT_AP_STOP:
            webconfig_manager_webserver_stop(&webconfig_manager_webserver);
        break;
        default: break;
    }
}

esp_err_t webconfig_manager_uri_handler_root(httpd_req_t *req)
{
    esp_err_t e;

    // Calculate size of request query
    size_t recv_size = MIN(httpd_req_get_url_query_len(req)+1, sizeof(webconfig_manager_content)-1);
    ESP_LOGD(TAG, "Request header size: %d", recv_size);

    // Get request query
    e = httpd_req_get_url_query_str(req, webconfig_manager_content, recv_size);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Query string: %s", webconfig_manager_content);
        // Search for reboot key
        e = httpd_query_key_value(webconfig_manager_content, WEBCONFIG_MANAGER_URI_PARAM_REBOOT, webconfig_manager_buffer, sizeof(webconfig_manager_buffer));
        if(e == ESP_OK) { // Reboot parameter found in query
            e = webconfig_manager_page_reboot(webconfig_manager_buffer, req); // Generate HTML
            if(e == ESP_OK) {
                ESP_LOGD(TAG, "Reboot page generated");
            } else {
                ESP_LOGE(TAG, "Error generating reboot page");
                httpd_resp_set_status(req, HTTPD_500);
            }
            e = httpd_resp_send(req, webconfig_manager_buffer, strlen(webconfig_manager_buffer)); // Send it to client
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

    e = webconfig_manager_page_root(webconfig_manager_buffer, req);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Root document generated");
    } else {
        ESP_LOGE(TAG, "Error generating root document");
        httpd_resp_set_status(req, HTTPD_500);
    }

    e = httpd_resp_send(req, webconfig_manager_buffer, strlen(webconfig_manager_buffer));
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Response sent");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Error sending response");
        return ESP_FAIL;
    }
}

esp_err_t webconfig_manager_uri_handler_setup(httpd_req_t * req)
{
    esp_err_t e;

    // Calculate size of request query
    size_t recv_size = MIN(httpd_req_get_url_query_len(req)+1, sizeof(webconfig_manager_content)-1);
    ESP_LOGD(TAG, "Request header size: %d", recv_size);

    // Get request query
    e = httpd_req_get_url_query_str(req, webconfig_manager_content, recv_size);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Query string: %s", webconfig_manager_content);
        // Search for namespace key
        e = httpd_query_key_value(webconfig_manager_content, "namespace", webconfig_manager_buffer, sizeof(webconfig_manager_buffer));
        if(e == ESP_OK) { // Requesting namespace page
            // Get namespace handle
            settings_manager_namespace_t * namespace = NULL;
            for(uint8_t i=0; i < settings_manager_namespaces_size; ++i) {
                if(!strcmp(webconfig_manager_buffer, settings_manager_namespaces[i].namespace)) {
                    namespace = &settings_manager_namespaces[i];
                    break;
                }
            }
            // Check if namespace requested exists
            if(namespace != NULL) {
                // Check if there are settings to update
                uint16_t entry_updated = 0; // Flag to mark if any settings were changed
                for(uint16_t i=0; i < settings_manager_entries_size; ++i) {
                    settings_manager_entry_t * entry = &settings_manager_entries[i];
                    if(settings_manager_entries[i].namespace_id == namespace->id) {
                        char encoded[100];
                        e = httpd_query_key_value(webconfig_manager_content, entry->key, encoded, sizeof(encoded));
                        if(e == ESP_OK) { // There is a setting to update
                            ESP_LOGD(TAG, "Value before decoding: %s", encoded);
                            webconfig_manager_urldecode(webconfig_manager_buffer, encoded); // Decode value from URL
                            ESP_LOGD(TAG, "Value after decoding: %s", webconfig_manager_buffer);
                            webconfig_manager_update_namespace_entry(entry, webconfig_manager_buffer);
                            ++entry_updated;
                            settings_manager_entry_to_string(encoded, entry);
                            ESP_LOGD(TAG, "Updated entry %s.%s with value %s", namespace->namespace, entry->key, encoded);
                        } // Nothing to do if setting is not found in query string
                    }
                }
                if(entry_updated > 0) {
                    e = settings_manager_commit_to_nvs(namespace); // Commit changes to namespace
                    if(e == ESP_OK) {
                        ESP_LOGD(TAG, "Entries updated and commited to NVS");
                    } else {
                        ESP_LOGE(TAG, "Error commiting changes to NVS: %s", esp_err_to_name(e));
                    }
                }
                // Generate response
                e = webconfig_manager_page_setup_namespace(webconfig_manager_buffer, req, namespace);
                if(e == ESP_OK) {
                    ESP_LOGD(TAG, "Setup namespace page generated");
                } else {
                    ESP_LOGE(TAG, "Error generating namespace page: %s", esp_err_to_name(e));
                }
            } else { // namespace does not exist
                ESP_LOGW(TAG, "Requested namespace does not exist");
                e = webconfig_manager_page_setup(webconfig_manager_buffer, req);
                if(e == ESP_OK) {
                    ESP_LOGD(TAG, "Setup page generated");
                } else {
                    ESP_LOGE(TAG, "Error generating setup page");
                    httpd_resp_set_status(req, HTTPD_500); // Set response to error 500
                }
            }
        } else if(e == ESP_ERR_NOT_FOUND) { // no namespace requested
            e = webconfig_manager_page_setup(webconfig_manager_buffer, req); // Return setup page
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
        e = webconfig_manager_page_setup(webconfig_manager_buffer, req); // Return setup page
        if(e == ESP_OK) {
            ESP_LOGD(TAG, "Homepage generated");
        } else {
            ESP_LOGE(TAG, "Error generating homepage: %s", esp_err_to_name(e));
            httpd_resp_set_status(req, HTTPD_500); // Set response to error 500
        }
    } else {
        ESP_LOGE(TAG, "Error processing query string: %s", esp_err_to_name(e));
        httpd_resp_set_status(req, HTTPD_500); // Set response to error 500
    }
    
    e = httpd_resp_send(req, webconfig_manager_buffer, strlen(webconfig_manager_buffer));
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Response sent");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Error sending response");
        return ESP_FAIL;
    }
}

esp_err_t webconfig_manager_uri_handler_get(httpd_req_t * req)
{
    esp_err_t e;

    size_t recv_size = MIN(httpd_req_get_url_query_len(req)+1, sizeof(webconfig_manager_content)-1);
    ESP_LOGD(TAG, "Request header size: %d", recv_size);

    // Get request query
    e = httpd_req_get_url_query_str(req, webconfig_manager_content, recv_size);
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Query string: %s", webconfig_manager_content);
        // Search for namespace key
        e = httpd_query_key_value(webconfig_manager_content, WEBCONFIG_MANAGER_URI_PARAM_GET_NAMESPACE, webconfig_manager_buffer, sizeof(webconfig_manager_buffer));
        if(e == ESP_OK) { // Requesting namespace page
            // Get namespace handle
            settings_manager_namespace_t * namespace = NULL;
            for(uint8_t i=0; i < settings_manager_namespaces_size; ++i) {
                if(!strcmp(webconfig_manager_buffer, settings_manager_namespaces[i].namespace)) {
                    namespace = &settings_manager_namespaces[i];
                    break;
                }
            }
            // Check if requested namespace exists
            if(namespace != NULL) {
                e = httpd_query_key_value(webconfig_manager_content, WEBCONFIG_MANAGER_URI_PARAM_GET_ENTRY, webconfig_manager_buffer, sizeof(webconfig_manager_buffer));
                settings_manager_entry_t * entry = NULL;
                for(uint8_t i=0; i < settings_manager_entries_size; ++i) {
                    if(settings_manager_entries[i].namespace_id == namespace->id) {
                        if(!strcmp(webconfig_manager_buffer, settings_manager_entries[i].key)) {
                            entry = &settings_manager_entries[i];
                            break;
                        }
                    }
                }
                // Check if requested setting exists
                if(entry != NULL) {
                    // Print raw value on response buffer
                    settings_manager_entry_to_string(webconfig_manager_buffer, entry);
                    ESP_LOGD(TAG, "Response content: %s", webconfig_manager_buffer);                 
                } else {
                    ESP_LOGE(TAG, "Requested namespace does not exist");
                    strcpy(webconfig_manager_buffer, "ERROR: Requested setting does not exist");
                    httpd_resp_set_status(req, HTTPD_404);
                }
            } else { // namespace does not exist
                ESP_LOGE(TAG, "Requested namespace does not exist");
                strcpy(webconfig_manager_buffer, "ERROR: Requested namespace does not exist");
                httpd_resp_set_status(req, HTTPD_404);
            }
        } else if(e == ESP_ERR_NOT_FOUND) { // no namespace requested
            ESP_LOGE(TAG, "No namespace found");
            strcpy(webconfig_manager_buffer, "ERROR: No namespace found");
            httpd_resp_set_status(req, HTTPD_404);
        } else {
            ESP_LOGE(TAG, "Error: query does not fit buffer: %s", esp_err_to_name(e));
            strcpy(webconfig_manager_buffer, "ERROR: Query does not fit buffer");
            httpd_resp_set_status(req, HTTPD_500);
        }
    } else {
        ESP_LOGE(TAG, "Error: no query string");
        httpd_resp_set_status(req, HTTPD_404);
    }
    
    e = httpd_resp_send(req, webconfig_manager_buffer, strlen(webconfig_manager_buffer));
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "Response sent");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Error sending response");
        return ESP_FAIL;
    }
}

esp_err_t webconfig_manager_page_root(char * buffer, httpd_req_t * req)
{
    if(req == NULL || buffer == NULL) {
        ESP_LOGE(TAG, "req and buffer cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // FIXME Buffer overrun protection

    // TODO Show featured values on home page
    // For now it is just a link to setup

    strcpy(buffer, "<html><head></head><body>");
    strcat(buffer, WEBCONFIG_MANAGER_WEB_TITLE);
    strcat(buffer, "</head><body><p><a href=\"/setup\">Setup</a></a></body>");

    return ESP_OK;
}

esp_err_t webconfig_manager_page_reboot(char * buffer, httpd_req_t * req)
{
    if(req == NULL || buffer == NULL) {
        ESP_LOGE(TAG, "req and buffer cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // FIXME Buffer overrun protection

    strcpy(buffer, "<html><head></head><body>");
    strcat(buffer, WEBCONFIG_MANAGER_WEB_TITLE);
    strcat(buffer, "</head><body>Rebooting device. Wait 10 seconds before clicking <a href=\"/setup\">back</a> (Link might not work if network settings were changed)</body>");
    
    return ESP_OK;
}

esp_err_t webconfig_manager_page_setup(char * buffer, httpd_req_t * req)
{
    if(req == NULL || buffer == NULL) {
        ESP_LOGE(TAG, "req and buffer cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // FIXME Buffer overrun protection

    strcpy(buffer, "<html><head></head><body>");
    strcat(buffer, WEBCONFIG_MANAGER_WEB_TITLE);
    strcat(buffer, "</head><body><ul>");
    for(uint8_t i=0; i<settings_manager_namespaces_size; ++i) {
        strcat(buffer, "<p><a href=\"/setup?namespace=");
        strcat(buffer, settings_manager_namespaces[i].namespace);
        strcat(buffer, "\">");
        strcat(buffer, settings_manager_namespaces[i].namespace);
        strcat(buffer, "</a></p>");
    }
    strcat(buffer, "</ul><a href=\"/?reboot=1\">Reboot</a></body></html>");

    return ESP_OK;
}

esp_err_t webconfig_manager_page_setup_namespace(char * buffer, httpd_req_t * req, settings_manager_namespace_t * namespace)
{
    if(req == NULL || buffer == NULL || namespace == NULL) {
        ESP_LOGE(TAG, "Invalid arguments");
        return ESP_ERR_INVALID_ARG;
    }

    // FIXME Buffer overrun protection

    strcpy(buffer, "<html><head></head><body>");
    strcat(buffer, WEBCONFIG_MANAGER_WEB_TITLE);
    strcat(buffer, "</head><body><form method=\"get\" action=\"/setup\"><br/><input name=\"namespace\" type=\"hidden\" value=\"");
    strcat(buffer, namespace->namespace);
    strcat(buffer, "\"><br/>");
    
    for(uint16_t i=0; i < settings_manager_entries_size; ++i) {
        if(settings_manager_entries[i].namespace_id == namespace->id) {
            settings_manager_entry_t * entry = &settings_manager_entries[i];
            char value[50]; // FIXME Remove this magic number
            strcat(buffer, entry->key);
            strcat(buffer, "<br/><input type=\"");

            // TBD Should this be made 2 switches so the common statements to different types don't get duplicated multiple times? (all the "strcat")
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
                    settings_manager_entry_to_string(value, entry);
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
                    ESP_LOGE(TAG, "Entry %s.%s is of an unknown type", namespace->namespace, entry->key);
                break;
            }
            strcat(buffer, "\" name=\"");
            strcat(buffer, entry->key);
            strcat(buffer, "\"/><br/>");
        }
    }

    strcat(buffer, "<input type=\"submit\" value=\"submit\"></form><a href=\"/setup\">Back</a></body></html>");

    return ESP_OK;
}

esp_err_t webconfig_manager_update_namespace_entry(settings_manager_entry_t * entry, const char * value_str)
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

int32_t webconfig_manager_urldecode(char *__restrict__ dest, const char *__restrict__ src)
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