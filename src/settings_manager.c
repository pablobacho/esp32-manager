/**
 * settings_manager.c - ESP-IDF component to work with settings
 * 
 * Implementation.
 * 
 * (C) 2019 - Pablo Bacho <pablo@pablobacho.com>
 * This code is licensed under the MIT License.
 */

 #include "settings_manager.h"

static const char * TAG = "settings_manager";

settings_manager_namespace_t settings_manager_namespaces[SETTINGS_MANAGER_NAMESPACE_MAX_SIZE];
uint8_t settings_manager_namespaces_size = 0;

settings_manager_entry_t settings_manager_entries[SETTINGS_MANAGER_ENTRIES_MAX_SIZE];
uint16_t settings_manager_entries_size = 0;

esp_err_t settings_manager_init()
{
    esp_err_t e;

    ESP_LOGD(TAG, "Initializing storage for settings");
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

settings_manager_handle_t settings_manager_register_namespace(const char * namespace)
{
    esp_err_t e;

    ESP_LOGD(TAG, "Registering namespace: %s", namespace);

    // Check if there is space
    if(settings_manager_namespaces_size >= SETTINGS_MANAGER_NAMESPACE_MAX_SIZE) {
        ESP_LOGE(TAG, "Not enough memory to register namespace %s", namespace);
        return NULL;
    }

    ESP_LOGD(TAG, "There is enough memory to register namespace. Checking if it already exists...");
    // Check if namespace is already loaded
    for(uint8_t i = 0; i < settings_manager_namespaces_size; ++i) {
        if(!strcmp(namespace, settings_manager_namespaces[i].namespace)) {
            ESP_LOGW(TAG, "Namespace %s already loaded", namespace);
            return (settings_manager_handle_t) &settings_manager_namespaces[i];
        }
    }

    ESP_LOGD(TAG, "Registering namespace..");

    // Register namespace & increment size pointer
    settings_manager_namespace_t * current_namespace = &settings_manager_namespaces[settings_manager_namespaces_size];
    current_namespace->namespace = namespace;
    ESP_LOGD(TAG, "Opening NVS for R/W");
    e = nvs_open(current_namespace->namespace, NVS_READWRITE, &current_namespace->nvs_handle);
    if(e == ESP_OK) {
        current_namespace->id = settings_manager_namespaces_size++;
        current_namespace->namespace = namespace;
        ESP_LOGD(TAG, "Namespace %s registered with id %d. NVS open for R/W.", current_namespace->namespace, current_namespace->id);
        return current_namespace;
    } else {
        ESP_LOGE(TAG, "Cannot open namespace \"settings\" with read/write access");
        return NULL;
    }
}

esp_err_t settings_manager_register_setting(settings_manager_handle_t handle, const char * key, settings_manager_type_t type, void * value, uint32_t attributes)
{
    // Check valid handler
    if(handle == NULL) {
        ESP_LOGE(TAG, "Can't register setting: Invalid handle");
        return ESP_ERR_INVALID_ARG;
    }

    // Check for space in memory
    if(settings_manager_entries_size >= SETTINGS_MANAGER_ENTRIES_MAX_SIZE) {
        ESP_LOGE(TAG, "Not enough memory to register setting %s.%s", handle->namespace, key);
        return ESP_ERR_NO_MEM;
    }

    // Check if setting exists
    for(uint16_t i=0; i<settings_manager_entries_size; ++i) {
        if(settings_manager_entries[i].namespace_id == handle->id) {
            if(!strcmp(settings_manager_entries[i].key, key)) { // If it exists, overwrite
                ESP_LOGW(TAG, "Setting already exists. Overwriting");
                settings_manager_entries[i].type = type;
                settings_manager_entries[i].value = value;
                settings_manager_entries[i].attributes = attributes;
                return ESP_OK;
            }
        }
    }

    // Register setting
    settings_manager_entry_t * entry = &settings_manager_entries[settings_manager_entries_size++];
    entry->namespace_id = handle->id;
    entry->key = key;
    entry->type = type;
    entry->value = value;
    entry->attributes = attributes;

    return ESP_OK;
}

esp_err_t settings_manager_commit_to_nvs(settings_manager_handle_t handle)
{
    esp_err_t e = ESP_OK;
    uint16_t entries_to_commit_counter = 0;

    if(handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    for(uint16_t i=0; i<settings_manager_entries_size; ++i) {
        settings_manager_entry_t * entry = &settings_manager_entries[i];
        if(handle->id == entry->namespace_id) {
            switch(entry->type) {
                case i8:
                    e = nvs_set_i8(handle->nvs_handle, entry->key, *((int8_t *) entry->value));
                break;
                case u8:
                case single_choice: // Change this to a different integral type to allow for other than 256 options
                    e = nvs_set_u8(handle->nvs_handle, entry->key, *((uint8_t *) entry->value));
                break;
                case i16:
                    e = nvs_set_i16(handle->nvs_handle, entry->key, *((int16_t *) entry->value));
                break;
                case u16:
                    e = nvs_set_u16(handle->nvs_handle, entry->key, *((uint16_t *) entry->value));
                break;
                case i32:
                    e = nvs_set_i32(handle->nvs_handle, entry->key, *((int32_t *) entry->value));
                break;
                case u32:
                case multiple_choice: // Change this to a different integral type to allow for other than 32 options
                    e = nvs_set_u32(handle->nvs_handle, entry->key, *((uint32_t *) entry->value));
                break;
                case i64:
                    e = nvs_set_i64(handle->nvs_handle, entry->key, *((int64_t *) entry->value));
                break;
                case u64:
                    e = nvs_set_u64(handle->nvs_handle, entry->key, *((uint64_t *) entry->value));
                break;
                case flt:
                    e = nvs_set_blob(handle->nvs_handle, entry->key, (float *) entry->value, sizeof(float));
                break;
                case dbl:
                    e = nvs_set_blob(handle->nvs_handle, entry->key, (double *) entry->value, sizeof(double));
                break;
                case text: // This type needs to be null-terminated
                case password:
                    e = nvs_set_str(handle->nvs_handle, entry->key, (char *) entry->value);
                break;
                case blob: // Data structures, binary and other non-null-terminated types go here
                case image:
                    ESP_LOGE(TAG, "Blob and image support not implemented");
                    //nvs_set_blob(handle->nvs_handle, entry->key, entry->value, size);
                break;
                default:
                    ESP_LOGE(TAG, "Entry %s.%s is of an unknown type", handle->namespace, entry->key);
                break;
            }
            // Check for errors
            if(e == ESP_OK) {
                ESP_LOGD(TAG, "Entry %s.%s set for NVS commit", handle->namespace, entry->key);
                ++entries_to_commit_counter;
            } else {
                ESP_LOGE(TAG, "Entry %s.%s could not be set for NVS commit", handle->namespace, entry->key);
            }
        }
    }

    if(entries_to_commit_counter > 0) {
        e = nvs_commit(handle->nvs_handle);
        if(e == ESP_OK) {
            ESP_LOGD(TAG, "Namespace %s commited to NVS", handle->namespace);
            return ESP_OK;
        } else {
            ESP_LOGE(TAG, "Could not commit namespace %s to NVS", handle->namespace);
            return ESP_FAIL;
        }
    } else {
        ESP_LOGD(TAG, "Nothing to commit in namespace %s", handle->namespace);
        return ESP_OK;
    }
}

esp_err_t settings_manager_read_from_nvs(settings_manager_handle_t handle)
{
    esp_err_t e = ESP_OK;
    uint8_t error_counter = 0; // When reading an entry from NVS throws error, it will retry to read it a number of times.

    size_t blob_length; // length parameter for blob values

    if(handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    for(uint16_t i=0; i<settings_manager_entries_size; ++i) {
        settings_manager_entry_t * entry = &settings_manager_entries[i];
        if(handle->id == entry->namespace_id) {
            switch(entry->type) {
                case i8:
                    e = nvs_get_i8(handle->nvs_handle, entry->key, (int8_t *) entry->value);
                break;
                case u8:
                case single_choice:
                    e = nvs_get_u8(handle->nvs_handle, entry->key, (uint8_t *) entry->value);
                break;
                case i16:
                    e = nvs_get_i16(handle->nvs_handle, entry->key, (int16_t *) entry->value);
                break;
                case u16:
                    e = nvs_get_u16(handle->nvs_handle, entry->key, (uint16_t *) entry->value);
                break;
                case i32:
                    e = nvs_get_i32(handle->nvs_handle, entry->key, (int32_t *) entry->value);
                break;
                case u32:
                case multiple_choice:
                    e = nvs_get_u32(handle->nvs_handle, entry->key, (uint32_t *) entry->value);
                break;
                case i64:
                    e = nvs_get_i64(handle->nvs_handle, entry->key, (int64_t *) entry->value);
                break;
                case u64:
                    e = nvs_get_u64(handle->nvs_handle, entry->key, (uint64_t *) entry->value);
                break;
                case flt:
                    blob_length = sizeof(float);
                    e = nvs_get_blob(handle->nvs_handle, entry->key, entry->value, &blob_length);
                break;
                case dbl:
                    blob_length = sizeof(double);
                    e = nvs_get_blob(handle->nvs_handle, entry->key, entry->value, &blob_length);
                case text:
                case password: ; // ; is an empty statement because labels need to be followed by a statement, not a declaration.
                    size_t len; // nvs_get_str needs a non-zero pointer to store the string length, even if we don't need it.
                    e = nvs_get_str(handle->nvs_handle, entry->key, (char *) entry->value, &len);
                break;
                case blob:
                case image:
                    ESP_LOGE(TAG, "Blob and image support not implemented yet");
                    e = ESP_FAIL;
                    //nvs_set_blob(handle->nvs_handle, entry->key, entry->value, size);
                break;
                default:
                    ESP_LOGE(TAG, "Entry %s.%s is of an unknown type", handle->namespace, entry->key);
                    e = ESP_OK;
                break;
            }
            if(e == ESP_OK) { // Entry read successfully from NVS
                ESP_LOGD(TAG, "Entry %s.%s read from NVS", handle->namespace, entry->key);
                error_counter = 0;
            } else if(e == ESP_ERR_NVS_NOT_FOUND) { // Entry not found in NVS. Not an error.
                error_counter = 0;
                ESP_LOGD(TAG, "Entry %s.%s not found in NVS", handle->namespace, entry->key); // Todo: Should this be a warning, informational or just debug?
            } else {
                ESP_LOGW(TAG, "Entry %s.%s could not be read from NVS. It will be erased.", handle->namespace, entry->key); // Something went wrong
                if(error_counter > 0) { // If we tried already
                    ESP_LOGE(TAG, "Erasing entry %s.%s.", handle->namespace, entry->key);
                    e = nvs_erase_key(handle->nvs_handle, entry->key); // Erase the entry
                    if(e != ESP_OK) {
                        ESP_LOGE(TAG, "Entry %s.%s could not be erased from NVS: %s", handle->namespace, entry->key, esp_err_to_name(e));
                        return ESP_FAIL;
                    }
                } else { // If this is the first attempt to read the entry, try to read it again.
                    ESP_LOGD(TAG, "Retrying to read entry %s.%s", handle->namespace, entry->key);
                    --i; // Decrement i so it the loop goes over the same entry again
                    ++error_counter; // Count it as an error
                }
                
            }
        }
    }

    return ESP_OK;
}

esp_err_t settings_manager_entry_to_string(char * dest, settings_manager_entry_t * entry)
{
    if(entry == NULL || dest == NULL) {
        ESP_LOGE(TAG, "Invalid arguments");
        return ESP_ERR_INVALID_ARG;
    }

    switch(entry->type) {
        case i8:
            sprintf(dest, "%d", (signed int) ((int8_t *) entry->value));
        break;
        case u8:
            sprintf(dest, "%x", (unsigned int) ((uint8_t *) entry->value));
        break;
        case i16:
            sprintf(dest, "%d", (signed int) ((int16_t *) entry->value));
        break;
        case u16:
            sprintf(dest, "%x", (unsigned int) ((uint16_t *) entry->value));
        break;
        case i32:
            sprintf(dest, "%d", (signed int) ((int32_t *) entry->value));
        break;
        case u32:
            sprintf(dest, "%x", (unsigned int) ((uint32_t *) entry->value));
        break;
        case i64:
            sprintf(dest, "%ld", (signed long int) ((int64_t *) entry->value));
        break;
        case u64:
            sprintf(dest, "%lu", (unsigned long int) ((uint64_t *) entry->value));
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
            //nvs_set_blob(handle->nvs_handle, entry->key, entry->value, size);
        break;
        default:
            ESP_LOGE(TAG, "Entry %s is of an unknown type", entry->key);
            return ESP_FAIL;
        break;
    }

    return ESP_OK;
}