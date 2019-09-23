# esp32-manager

**esp32-manager** helps with the configuration and management of settings on ESP32.

## Features

- **settings_manager** simplifies reading from and writing to flash using NVS for any variable in your application. Just register any variable with settings_manager and you will get simple read and write functions for NVS.
- **network_manager** manages WiFi connection to networks or creation of APs.
- **webconfig_manager** creates a web interface to configure any setting in your application registered with *settings_manager*.

## Usage

### Setup workspace

This is an ESP-IDF component. To include it in your project, you can clone this repository into the *components* folder in your project.

    git clone https://github.com/pablobacho/esp32-manager.git

Include `settings_manager.h`, `network_manager.h` and `webconfig_manager.h` as needed in your source files to use it.

    #include "settings_manager.h"
    #include "network_manager.h"
    #include "webconfig_manager.h"

### Initialization

Initialize components using these functions.

    settings_manager_init();
    network_manager_init();
    webconfig_manager_init();

`network_manager_init()`depends on `settings_manager`to be initialized. Likewise, `webconfig_manager_init()` depends on both `settings_manager` and `network_manager` to be initialized.

### Namespaces

Settings are grouped into *namespaces*. To register a namespace use:

    settings_manager_handle_t settings_handle = settings_manager_register_namespace("my_settings");

Where `settings_handle` is a pointer to the namespace handle that will be created, and `"my_settings"` the name of the namespace.

### Settings entries

Settings are regular variables that exist in your program. To use them with `settings_manager` you need to register them under a namespace:

    uint32_t timeout = 1280u;
    settings_manager_register_setting(settings_handle, "timeout", u32, (void *) &timeout, SETTINGS_MANAGER_ATTR_READWRITE);

Where:
- `uint32_t timeout` is the variable to be registered. In this example its value is 1280.
- `settings_handle` is the namespace handle created earlier.
- `"timeout"` is the key by which the setting will be identified. The key needs to be unique inside a namespace.
- `u32` is the type of the setting to register. Check out `settings_manager.h` for the complete list of types.
- `(void *) &timeout` is the address to the variable as a pointer to `void`.
- And `SETTINGS_MANAGER_ATTR_READWRITE` are the attributes assigned to the setting. Check out `settings_manager.h` for the complete list of attributes.

To write or read a setting, just use the referenced variable in your program. In the example above this is the variable `timeout`.

### Load from and save to NVS (Flash)

To load settings from NVS:

    settings_manager_read_from_nvs(settings_handle);

This function will try to find all settings registered under this namespace in NVS, and load their values. Namespace and settings need to be registered before calling this function. This function overwrites settings' values with the contents read from NVS.

To save settings to NVS:

    settings_manager_commit_to_nvs(settings_handle);

This function saves all settings of a given namespace in NVS. It overwrites any previous value present in flash for the same settings.

### Networking

The module `network_manager` helps managing WiFi connections.

Initialize `network_manager` with:

    network_manager_init();

This function registers the namespace `network` with the `settings_manager` and some of its settings, such as `ssid` and `password` of a WiFi network.

After initialization, start WiFi using:

    network_manager_wifi_start(mode);

Where `mode` can be any of the following:

- `STA` or *station mode*: Will try to connect to a WiFi network with SSID and passwords already known. It uses the variables `network_manager_ssid` and `network_manager_password` to connect to a known WiFi network. These are intended to be populated from NVS by the `settings_manager` module or configured through the web interface using `webconfig_manager`.
- `AP` or *AP mode*: Creates an AP (access point) a 3rd device can connect to, such as a smartphone or computer. This is useful if there is no WiFi infrastructure deployed or its configuration has not been done.
- `AUTO`: Auto will check whether there is a known SSID to connect to, and start in `STA` mode if there is, or `AP` if there is not.

### Webconfig

Initialize `webconfig_manager` using:

    webconfig_manager_init();

After initialization, `webconfig_manager` will listen to events from `network_manager` starting and stopping a webserver automatically as the device obtains an IP address, disconnects, creates an AP or other events.

After successfully connecting to a WiFi network or creating an AP, the webserver is accessible through the device's IP address. In AP mode, the device IP address is 192.168.4.1. In station mode, check your router tools to find it.

#### Configuration using a web browser

For readability the following information use the access point IP address.

Browsing to `http://192.168.4.1/` will take you to the homepage. It will show youa title and a link to the setup page. This page is intended to show some information on the device, but it has not been implemented yet.

Clicking the link *Setup* or pointing a browser to `http://192.168.4.1/setup` will show you a list of namespaces registered with `settings_manager`.

Clicking one of the namespaces will allow you to edit its settings. Click *Save* to update them.

#### Scripting or software from a remote machine

All same operations can be perform programmatically from another machine connected to the same network via HTTP GET methods.

On the `/setup` uri, use the query parameters `namespace` and the setting key to update a value. For example:

    http://192.168.4.1/setup?namespace=network&ssid=mywifi

The link above will update the setting with the key "ssid" in the namespace "network" with the value "mywifi". You can also update more than one setting at a time in the same namespace:

    http://192.168.4.1/setup?namespace=network&ssid=mywifi&password=mypassword

There is a `get` uri that allows retrieving raw values from settings. The following example returns a string with the content of the setting `network.ssid`:

    http://192.168.4.1/get?namespace=network&key=ssid

### Typical workflow

A typical workflow could be:

1. Device is booting for the first time.
2. Initialize components.
3. Start WiFi in AUTO mode. Being the first time, there is no SSID registered to connect to, so it will create an access point and wait for the user to connect to it.
4. Using a web browser, user connects to AP and configures WiFi using *webconfig*. After saving configuration, click *reboot*.
5. Device reboots, initializes components and, again, starts in AUTO mode. This time there is an SSID and a password stored, so it will go to STA mode and try to connect to the given WiFi network.

## Roadmap

I am building this component for a project I currently have, and I will be adding features as needed. As of right now, there are a few features that I might be implementing next (not necessarily in this particular order):

- Not all types are implemented. In particular, support for types such as *multiple choice*, *single choice* and *images* is not implemented.
- A network scanner to configure WiFi instead of entering the information manually.
- Using [PREACT](https://preactjs.com/) to create a rich web interface. As of today the UI is plain HTML with no styling.
- Allowing alternative methods to get/set settings. Thinking of MQTT support.

## Contributing

If you find a bug, improve documentation, add a feature, or anything else, I encourage you to open an issue and/or make a pull request.

When contributing, please follow [Espressif IoT Development Framework Style Guide](https://docs.espressif.com/projects/esp-idf/en/latest/contribute/style-guide.html) and their [Documenting Code Guide](https://docs.espressif.com/projects/esp-idf/en/latest/contribute/documenting-code.html)

Licensed under the MIT License.