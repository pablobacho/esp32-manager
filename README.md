# esp32-manager

**esp32-manager** helps with the configuration and management of settings on ESP32.

## Features

- Simplifies reading from and writing to flash using NVS for any variable in your application. Just register any variable with esp32_manager and you will get simple read and write functions for NVS.
- Easy WiFi connection to networks or creation of APs. Automatic AP creation for WiFi configuration.
- Creates a web interface to configure any setting in your application registered with *esp32_manager*.
- Easily publish values to MQTT in a structured and consistent way.

## Usage

### Example project

Check out the example project at [https://github.com/pablobacho/esp32-manager-example](https://github.com/pablobacho/esp32-manager-example)

### Setup workspace

This is an ESP-IDF component. To include it in your project, you can clone this repository into the *components* folder in your project.

    git clone https://github.com/pablobacho/esp32-manager.git

Include `esp32_manager.h` in your source files to use it.

    #include "esp32_manager.h"

### Initialization

Initialize the component using this function.

    esp32_manager_init();

### Entry registration

*Entry* is any variable in your application managed by `esp32_manager`. For a variable to become a valid entry it needs to be registered with `esp32_manager`. Entries are grouped into *namespaces* for easier modularization.

For example, we got this two variables that we want to manage using `esp32_manager`:

    int16_t counter = 0;
    uint32_t delay = 1000;

First, let's create their entries:

    esp32_manager_entry_t counter_entry = {
        .key = "counter",                       // a keyword to identify this entry
        .friendly = "Counter",                  // a human-readable name for the web configuration interface
        .type = i32,                            // type of the variable (check `esp32_manager_storage.h` for other options)
        .value = (void *) &counter,             // address of the variable casted to `void *`
        .attributes = ESP32_MANAGER_ATTR_READ   // attributes (check `esp32_manager_storage.h` for other options)
    };

    esp32_manager_entry_t delay_entry = {
        .key = "delay",
        .friendly = "Delay",
        .type = u32,
        .value = (void *) &delay,
        .attributes = ESP32_MANAGER_ATTR_READWRITE
    };

Create an array big enough to group these entries:

    esp32_manager_entry_t * example_entries[2];

Create the namespace these entries belong to:

    esp32_manager_namespace_t example_namespace = {
        .key = "example_ns",                // a keyword to identify this entry
        .friendly = "Example Namespace",    // a human-readable name for the web configuration interface
        .entries = example_entries,         // the array of pointers to entries previously created
        .entries_size = 2                   // number of entries in the array
    };

After calling `esp32_init()`, register the namespace:

    esp32_manager_register_namespace(&example_namespace);

And then register the entries:

    esp32_manager_register_entry(&example_namespace, &counter_entry);
    esp32_manager_register_entry(&example_namespace, &delay_entry);

This will create a webpage under the url `http://[device_ip]/setup` that will list all registered namespaces. Clicking on a namespace, will open up a form with current values of the entries registered in that namespace. You can modify the values and submit the forms to update them.

You can also get raw values by doing HTTP GET requests to the url `http://[device_ip]/get?namespace=[namespace.key]&entry=[entry.key]

### Load from and save to NVS (Flash)

Typically, after registering the entries your application will want to load their values stored in flash (if available):

    esp32_manager_read_from_nvs(&example_namespace);

This function will try to find all settings registered under this namespace in NVS, and load their values. Namespace and settings need to be registered before calling this function. This function overwrites entries' values with the contents read from NVS.

If your application changes the values of the variables associated to entries, call this function to save them to NVS:

    esp32_manager_commit_to_nvs(&example_namespace);

This function saves all settings of a given namespace in NVS. It overwrites any previous value present in flash for the same settings.

When submiting values via web configuration forms, changes are always commited to NVS.

### Networking

`esp32_manager` will also help you configuring your WiFi connection.

First, there is a namespace registered for networking, called `network`. This namespace has network settings entries registered under it, such as *SSID* and *password* of the WiFi network to connect to.

Using `esp32_manager_network_wifi_start(AUTO)` will first check the contents of the `network.ssid` entry.

- If it is empty, then an Access-Point (AP) will be created. You can connect to this access point using the default ssid `wifi-manager` and password `12345678`. These values can be changed in `esp32_manager_network.h`. After connecting to the AP, open a browser and go to `192.168.4.1` to configure WiFi.
- If it is not empty, it will try to connect to a WiFi with that SSID, and it will keep trying to connect to it until it succeeds.

You can also specify on which mode you want WiFi to start replacing AUTO by STA or AP:

- `STA` or *station mode*: Will try to connect to a WiFi network with SSID and passwords already known.
- `AP` or *AP mode*: Creates an AP (access point) a 3rd device can connect to, such as a smartphone or computer.
- `AUTO`: Auto will check whether there is a known SSID to connect to, and start in `STA` mode if there is, or `AP` if there is not.

### Accessing programmatically from a remote machine via HTTP

All operations can be perform programmatically from another machine connected to the same network via HTTP GET methods.

On the `/setup` uri, use the query parameters `namespace` and the setting key to update a value. For example:

    http://192.168.4.1/setup?namespace=network&ssid=mywifi

The link above will update the setting with the key "ssid" in the namespace "network" with the value "mywifi". You can also update more than one setting at a time in the same namespace:

    http://192.168.4.1/setup?namespace=network&ssid=mywifi&password=mypassword

There is a `get` uri that allows retrieving raw values from settings. The following example returns a string with the content of the setting `network.ssid`:

    http://192.168.4.1/get?namespace=network&key=ssid

### Accessing programmatically from a remote machine via MQTT

**NEW!** Includes preliminary MQTT support for obtaining information on entries.

Using the web interface previously described, set the *MQTT Broker URL* under the *MQTT* section. After updating this field, please reboot the device for the change to take effect (can be done remotely via the web interface).

Using the function `esp32_manager_mqtt_publish_entry` you can publish the entry information to the broker, such as:

    esp32_manager_mqtt_publish_entry(&example_namespace, &counter_entry);

This will publish the value of the entry to a topic build like: `/[hostname]/[namespace.key]/[entry.key]`. In this example the topic is:

    /esp32-device/example_ns/counter

### Typical workflow

A typical workflow could be:

1. Device is booting for the first time.
2. Initialize `esp32_manager` component.
3. Register namespaces and entries.
4. Start WiFi in AUTO mode. Being the first time, there is no SSID registered to connect to, so it will create an access point and wait for the user to connect to it.
5. Using a web browser, user connects to AP and configures WiFi using the web configuration interface. After saving configuration, click *reboot*.
6. Device reboots, initializes components and, again, starts in AUTO mode. This time there is an SSID and a password stored, so it will go to STA mode and try to connect to the given WiFi network.

## Roadmap

I am building this component for a project I currently have, and I will be adding features as needed. As of right now, there are a few features that I might be implementing next (not necessarily in this particular order):

- Not all types are implemented. In particular, support for types such as *multiple choice*, *single choice* and *images* is not implemented.
- A network scanner to configure WiFi instead of entering the information manually.
- The UI uses the Milligram framework for styling, but some lightweight JS framework would give it a much richer interface.
- Allowing alternative methods to get/set settings. I am currently working on MQTT support.

## Contributing

If you find a bug, improve documentation, add a feature, or anything else, I encourage you to open an issue and/or make a pull request.

When contributing, please follow [Espressif IoT Development Framework Style Guide](https://docs.espressif.com/projects/esp-idf/en/latest/contribute/style-guide.html) and their [Documenting Code Guide](https://docs.espressif.com/projects/esp-idf/en/latest/contribute/documenting-code.html)

Licensed under the MIT License.
