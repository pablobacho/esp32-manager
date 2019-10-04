/**
 * esp32_manager_storage.c
 * 
 * (C) 2019 - Pablo Bacho <pablo@pablobacho.com>
 * This code is licensed under the MIT License.
 */

#include "esp32_manager_storage.h"

static const char * TAG = "esp32_manager_storage";

esp32_manager_namespace_t * esp32_manager_namespaces[ESP32_MANAGER_NAMESPACES_SIZE];

esp_err_t esp32_manager_storage_init()
{
    esp_err_t e;

    // Initialize NVS storage
    ESP_LOGD(TAG, "Initializing NVS storage");
    e = nvs_flash_init();
    if(e == ESP_OK) {
        ESP_LOGD(TAG, "NVS initialized successfully");
    } else if(e == ESP_ERR_NVS_NO_FREE_PAGES) { // If it can't initialize NVS
        ESP_LOGW(TAG, "NVS partition was resized or changed. Formatting...");
        const esp_partition_t* nvs_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);      
        if(!nvs_partition) {
            ESP_LOGE(TAG, "No NVS partition found");
            return ESP_ERR_NVS_PART_NOT_FOUND;
        }
        e = esp_partition_erase_range(nvs_partition, 0, nvs_partition->size);
        if(e == ESP_OK) {
            ESP_LOGD(TAG, "Partition formatted succesfully");
        } else {
            ESP_LOGE(TAG, "Unable to erase the partition");
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}

esp_err_t esp32_manager_register_namespace(esp32_manager_namespace_t * namespace)
{
    esp_err_t e;

    if(namespace->key == NULL || namespace->friendly == NULL || namespace->entries == NULL) {
        ESP_LOGE(TAG, "Error registering namespace: Argument NULL");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGD(TAG, "Registering namespace: %s", namespace->key);

    uint8_t i;

    // Check if namespace is already registered
    for(i = 0; i < ESP32_MANAGER_NAMESPACES_SIZE; ++i) {
        if(esp32_manager_namespaces[i] == namespace) {
            ESP_LOGE(TAG, "Namespace %s already registered", namespace->key);
            return ESP_ERR_INVALID_STATE;
        }
    }

    // Register namespace
    for(i=0; i<ESP32_MANAGER_NAMESPACES_SIZE; ++i) {
        if(esp32_manager_namespaces[i] == NULL) {
            esp32_manager_namespaces[i] = namespace;
            ESP_LOGD(TAG, "Opening NVS for R/W");
            e = nvs_open(namespace->key, NVS_READWRITE, &namespace->nvs_handle);
            if(e == ESP_OK) {
                ESP_LOGD(TAG, "Namespace %s registered. NVS open for R/W.", namespace->key);
                return ESP_OK;
            } else {
                ESP_LOGE(TAG, "Cannot open namespace \"%s\" with read/write access: %s", namespace->key, esp_err_to_name(e));
                return ESP_FAIL;
            }
        }
    }
    if(i >= ESP32_MANAGER_NAMESPACES_SIZE) {
        ESP_LOGE(TAG, "Not enough memory to register namespace %s", namespace->key);
        return ESP_ERR_NO_MEM;
    }

    return ESP_FAIL;
}

esp_err_t esp32_manager_register_entry(esp32_manager_namespace_t * namespace, esp32_manager_entry_t * entry)
{
    // Check valid arguments
    if(namespace == NULL || entry == NULL) {
        ESP_LOGE(TAG, "Error registering setting: Invalid arguments");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGD(TAG, "Registering entry: %s.%s", namespace->key, entry->key);

    uint8_t i;

    // Check if entry is already registered
    for(i = 0; i < namespace->size; ++i) {
        if(namespace->entries[i] == entry) {
            ESP_LOGE(TAG, "Entry %s already registered", entry->key);
            return ESP_ERR_INVALID_STATE;
        }
    }

    // Register entry
    for(i=0; i < namespace->size; ++i) {
        if(namespace->entries[i] == NULL) {
            // Assign default from_string and to_string methods
            if(entry->from_string == NULL) {
                ESP_LOGW(TAG, "'from_string' method not found for entry %s.%s. Assigning default.", namespace->key, entry->key);
                entry->from_string = &esp32_manager_entry_from_string_default;
            }
            if(entry->to_string == NULL) {
                ESP_LOGW(TAG, "'to_string' method not found for entry %s.%s. Assigning default.", namespace->key, entry->key);
                entry->to_string = &esp32_manager_entry_to_string_default;
            }
            // Add entry to namespace
            namespace->entries[i] = entry;
            ESP_LOGD(TAG, "Entry %s.%s registered", namespace->key, entry->key);
            return ESP_OK;
        }
    }
    if(i >= ESP32_MANAGER_NAMESPACES_SIZE) { // The loop ended without registering entry
        ESP_LOGE(TAG, "Not enough memory to register entry %s.%s", namespace->key, entry->key);
        return ESP_ERR_NO_MEM;
    }

    return ESP_FAIL;
}

esp_err_t esp32_manager_entry_to_string_default(esp32_manager_entry_t * entry, char * dest)
{
    if(entry == NULL || dest == NULL) {
        ESP_LOGE(TAG, "entry and source cannot be NULL" );
        return ESP_ERR_INVALID_ARG;
    }

    switch(entry->type) {
        case i8:
            sprintf(dest, "%d", (signed int) *((int8_t *) entry->value));
        break;
        case u8:
            sprintf(dest, "%u", (unsigned int) *((uint8_t *) entry->value));
        break;
        case i16:
            sprintf(dest, "%d", (signed int) *((int16_t *) entry->value));
        break;
        case u16:
            sprintf(dest, "%u", (unsigned int) *((uint16_t *) entry->value));
        break;
        case i32:
            sprintf(dest, "%d", (signed int) *((int32_t *) entry->value));
        break;
        case u32:
            sprintf(dest, "%u", (unsigned int) *((uint32_t *) entry->value));
        break;
        case i64:
            sprintf(dest, "%ld", (signed long int) *((int64_t *) entry->value));
        break;
        case u64:
            sprintf(dest, "%lu", (unsigned long int) *((uint64_t *) entry->value));
        break;
        case flt:
            sprintf(dest, "%f", (float) *((float *) entry->value));
        break;
        case dbl:
            sprintf(dest, "%lf", (double) *((double *) entry->value));
        break;
        case text:
        case password:
            strcpy(dest, (char *) entry->value);
        break;
        case single_choice:
        case multiple_choice:
        case blob:
        case image:
            ESP_LOGE(TAG, "Not implemented yet");
            return ESP_FAIL;
        break;
        default:
            ESP_LOGE(TAG, "Entry %s is of an unknown type", entry->key);
            return ESP_FAIL;
        break;
    }

    return ESP_OK;
}

esp_err_t esp32_manager_entry_from_string_default(esp32_manager_entry_t * entry, char * source)
{
    if(entry == NULL || source == NULL) {
        ESP_LOGE(TAG, "entry and source cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    uint64_t unumber;
    int64_t inumber;

    switch(entry->type) {
        case i8:
            inumber = atoi(source);
            *((int8_t *) entry->value) = (int8_t) inumber;
            break;
        case i16:
            inumber = atoi(source);
            *((int16_t *) entry->value) = (int16_t) inumber;
            break;
        case i32:
            inumber = atoi(source);
            *((int32_t *) entry->value) = (int32_t) inumber;
            break;
        case i64:
            inumber = atoi(source);
            *((int64_t *) entry->value) = (int64_t) inumber;
            break;
        case u8:
            unumber = atoi(source);
            *((uint8_t *) entry->value) = (uint8_t) unumber;
            break;
        case u16:
            unumber = atoi(source);
            *((uint16_t *) entry->value) = (uint16_t) unumber;
            break;
        case u32:
            unumber = atoi(source);
            *((uint32_t *) entry->value) = (uint32_t) unumber;
            break;
        case u64:
            unumber = atoi(source);
            *((uint64_t *) entry->value) = (uint64_t) unumber;
            break;
        case text: // This type needs to be null-terminated
            strcpy((char *) entry->value, source);
            break;
        case password:
            strcpy((char *) entry->value, source);
            break;
        // TODO Implement these cases
        case flt:
        case dbl:
        case single_choice:
        case multiple_choice:
        case blob: // Data structures, binary and other non-null-terminated types go here
        case image:
            ESP_LOGE(TAG, "Not implemented");
            return ESP_FAIL;
            //nvs_set_blob(handle->nvs_handle, entry->key, entry->value, size);
        break;
        default:
            ESP_LOGE(TAG, "Entry %s is of an unknown type", entry->key);
            return ESP_FAIL;
        break;
    }

    return ESP_OK;
}

esp_err_t esp32_manager_commit_to_nvs(esp32_manager_namespace_t * namespace)
{
    esp_err_t e = ESP_OK;
    uint16_t entries_to_commit_counter = 0;

    if(namespace == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    for(uint16_t i=0; i < namespace->size; ++i) {
        esp32_manager_entry_t * entry = namespace->entries[i];
        switch(entry->type) {
            case i8:
                e = nvs_set_i8(namespace->nvs_handle, entry->key, *((int8_t *) entry->value));
            break;
            case u8:
            case single_choice: // Change this to a different integral type to allow for other than 256 options
                e = nvs_set_u8(namespace->nvs_handle, entry->key, *((uint8_t *) entry->value));
            break;
            case i16:
                e = nvs_set_i16(namespace->nvs_handle, entry->key, *((int16_t *) entry->value));
            break;
            case u16:
                e = nvs_set_u16(namespace->nvs_handle, entry->key, *((uint16_t *) entry->value));
            break;
            case i32:
                e = nvs_set_i32(namespace->nvs_handle, entry->key, *((int32_t *) entry->value));
            break;
            case u32:
            case multiple_choice: // Change this to a different integral type to allow for other than 32 options
                e = nvs_set_u32(namespace->nvs_handle, entry->key, *((uint32_t *) entry->value));
            break;
            case i64:
                e = nvs_set_i64(namespace->nvs_handle, entry->key, *((int64_t *) entry->value));
            break;
            case u64:
                e = nvs_set_u64(namespace->nvs_handle, entry->key, *((uint64_t *) entry->value));
            break;
            case flt:
                e = nvs_set_blob(namespace->nvs_handle, entry->key, (float *) entry->value, sizeof(float));
            break;
            case dbl:
                e = nvs_set_blob(namespace->nvs_handle, entry->key, (double *) entry->value, sizeof(double));
            break;
            case text: // This type needs to be null-terminated
            case password:
                e = nvs_set_str(namespace->nvs_handle, entry->key, (char *) entry->value);
            break;
            case blob: // Data structures, binary and other non-null-terminated types go here
            case image:
                ESP_LOGE(TAG, "Blob and image support not implemented");
                //nvs_set_blob(namespace->nvs_handle, entry->key, entry->value, size);
            break;
            default:
                ESP_LOGE(TAG, "Entry %s.%s is of an unknown type", namespace->key, entry->key);
            break;
        }
        // Check for errors
        if(e == ESP_OK) {
            ESP_LOGD(TAG, "Entry %s.%s set for NVS commit", namespace->key, entry->key);
            ++entries_to_commit_counter;
        } else {
            ESP_LOGE(TAG, "Entry %s.%s could not be set for NVS commit", namespace->key, entry->key);
        }
    
    }

    if(entries_to_commit_counter > 0) {
        e = nvs_commit(namespace->nvs_handle);
        if(e == ESP_OK) {
            ESP_LOGD(TAG, "Namespace %s commited to NVS", namespace->key);
            return ESP_OK;
        } else {
            ESP_LOGE(TAG, "Could not commit namespace %s to NVS", namespace->key);
            return ESP_FAIL;
        }
    } else {
        ESP_LOGD(TAG, "Nothing to commit in namespace %s", namespace->key);
        return ESP_OK;
    }
}

esp_err_t esp32_manager_read_from_nvs(esp32_manager_namespace_t * namespace)
{
    esp_err_t e = ESP_OK;
    uint8_t error_counter = 0; // When reading an entry from NVS throws error, it will retry to read it a number of times.

    size_t blob_length; // length parameter for blob values

    if(namespace == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    for(uint16_t i=0; i < namespace->size; ++i) {
        esp32_manager_entry_t * entry = namespace->entries[i];
        switch(entry->type) {
            case i8:
                e = nvs_get_i8(namespace->nvs_handle, entry->key, (int8_t *) entry->value);
            break;
            case u8:
            case single_choice:
                e = nvs_get_u8(namespace->nvs_handle, entry->key, (uint8_t *) entry->value);
            break;
            case i16:
                e = nvs_get_i16(namespace->nvs_handle, entry->key, (int16_t *) entry->value);
            break;
            case u16:
                e = nvs_get_u16(namespace->nvs_handle, entry->key, (uint16_t *) entry->value);
            break;
            case i32:
                e = nvs_get_i32(namespace->nvs_handle, entry->key, (int32_t *) entry->value);
            break;
            case u32:
            case multiple_choice:
                e = nvs_get_u32(namespace->nvs_handle, entry->key, (uint32_t *) entry->value);
            break;
            case i64:
                e = nvs_get_i64(namespace->nvs_handle, entry->key, (int64_t *) entry->value);
            break;
            case u64:
                e = nvs_get_u64(namespace->nvs_handle, entry->key, (uint64_t *) entry->value);
            break;
            case flt:
                blob_length = sizeof(float);
                e = nvs_get_blob(namespace->nvs_handle, entry->key, entry->value, &blob_length);
            break;
            case dbl:
                blob_length = sizeof(double);
                e = nvs_get_blob(namespace->nvs_handle, entry->key, entry->value, &blob_length);
            case text:
            case password: ; // ; is an empty statement because labels need to be followed by a statement, not a declaration.
                size_t len; // nvs_get_str needs a non-zero pointer to store the string length, even if we don't need it.
                e = nvs_get_str(namespace->nvs_handle, entry->key, (char *) entry->value, &len);
            break;
            case blob:
            case image:
                ESP_LOGE(TAG, "Blob and image support not implemented yet");
                e = ESP_FAIL;
                //nvs_set_blob(namespace->nvs_handle, entry->key, entry->value, size);
            break;
            default:
                ESP_LOGE(TAG, "Entry %s.%s is of an unknown type", namespace->key, entry->key);
                e = ESP_OK;
            break;
        }
        if(e == ESP_OK) { // Entry read successfully from NVS
            ESP_LOGD(TAG, "Entry %s.%s read from NVS", namespace->key, entry->key);
            error_counter = 0;
        } else if(e == ESP_ERR_NVS_NOT_FOUND) { // Entry not found in NVS. Not an error.
            error_counter = 0;
            ESP_LOGD(TAG, "Entry %s.%s not found in NVS", namespace->key, entry->key); // Todo: Should this be a warning, informational or just debug?
        } else {
            ESP_LOGW(TAG, "Entry %s.%s could not be read from NVS. It will be erased.", namespace->key, entry->key); // Something went wrong
            if(error_counter > 0) { // If we tried already
                ESP_LOGE(TAG, "Erasing entry %s.%s.", namespace->key, entry->key);
                e = nvs_erase_key(namespace->nvs_handle, entry->key); // Erase the entry
                if(e != ESP_OK) {
                    ESP_LOGE(TAG, "Entry %s.%s could not be erased from NVS: %s", namespace->key, entry->key, esp_err_to_name(e));
                    return ESP_FAIL;
                }
            } else { // If this is the first attempt to read the entry, try to read it again.
                ESP_LOGD(TAG, "Retrying to read entry %s.%s", namespace->key, entry->key);
                --i; // Decrement i so it the loop goes over the same entry again
                ++error_counter; // Count it as an error
            }
            
        }
    }

    return ESP_OK;
}

esp_err_t esp32_manager_validate_namespace(esp32_manager_namespace_t * namespace)
{
    if(namespace == NULL) {
        ESP_LOGE(TAG, "Error esp32_manager_check_namespace_integrity: null pointer");
        return ESP_FAIL;
    }

    if(namespace->key == NULL) {
        ESP_LOGE(TAG, "Error esp32_manager_check_namespace_integrity: null key");
        return ESP_FAIL;
    } else {
        if(strlen(namespace->key) > ESP32_MANAGER_NAMESPACE_KEY_MAX_LENGTH) {
            ESP_LOGE(TAG, "Error esp32_manager_check_namespace_integrity: key is too long");
            return ESP_FAIL;
        }
    }

    if(namespace->friendly == NULL) {
        ESP_LOGE(TAG, "Error esp32_manager_check_namespace_integrity: null friendly name");
        return ESP_FAIL;
    }

    if(namespace->entries == NULL) {
        ESP_LOGE(TAG, "Error esp32_manager_check_namespace_integrity: null entries");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t esp32_manager_validate_entry(esp32_manager_entry_t * entry)
{
    if(entry == NULL) {
        ESP_LOGE(TAG, "Error esp32_manager_check_entry_integrity: null pointer");
        return ESP_FAIL;
    }

    if(entry->key == NULL) {
        ESP_LOGE(TAG, "Error esp32_manager_check_entry_integrity: null key");
        return ESP_FAIL;
    } else {
        if(strlen(entry->key) > ESP32_MANAGER_ENTRY_KEY_MAX_LENGTH) {
            ESP_LOGE(TAG, "Error esp32_manager_check_entry_integrity: key is too long");
            return ESP_FAIL;
        }
    }

    if(entry->friendly == NULL) {
        ESP_LOGE(TAG, "Error esp32_manager_check_entry_integrity: null friendly name");
        return ESP_FAIL;
    }

    if(entry->value == NULL) {
        ESP_LOGE(TAG, "Error esp32_manager_check_entry_integrity: null value");
        return ESP_FAIL;
    }

    if(entry->default_value == NULL) {
        ESP_LOGE(TAG, "Error esp32_manager_check_entry_integrity: null default value");
        return ESP_FAIL;
    }

    return ESP_OK;
}