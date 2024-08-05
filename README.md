
# ESPWebConnect Library for ESP32

## Overview

`ESPWebConnect` is a comprehensive library designed for the ESP32 platform. It provides easy integration for WiFi, MQTT, WebSocket, and web-based dashboards to monitor and control your IoT devices. This library enables seamless connectivity and interaction with sensors, switches, and buttons through a web interface. More control, more customization, no longer need depending on 3rd party cloud provider.

You can try use our installer at to test some example projects: https://danielamani.com/project/core_firmware/index.html
(Note only on Chrome Desktop, ensure disable Serial Monitor and Serial Plotter first)

------------


## Note
This library is in **BETA** development and for internal usage of ProjectEDNA. Don't use in deployment or mission critical project as there are many changes ahead. This library are the one of key part of `Integration Of Iot Platform Environment For Web Server & Global Connectivity Based On Esp32`

## Features

- Easy WiFi and MQTT configuration through a web interface (No more hardcoded WiFi).
- Connects to WiFi networks and sets up Access Point mode for configuration.
- Creates a web server for configuration and control.
- Customizable dashboard with icons and colors.
- Publishes and subscribes to MQTT topics.
- Supports LittleFS for file storage.
- Integrates WebSockets for real-time updates.
- Allows the addition of sensors, switches, and buttons to a customizable dashboard.

------------


## Installation

1. Download the `ESPWebConnect` library (On top Green button that show `<> Code`, Click download Zip).
2. On Arduino IDE, click Sketch, Include Library, Add .ZIP library.
3. Ensure you have the necessary dependencies or library installed:
   - `Wifi.h`
   - `WebServer.h`
   - `WebSocketsServer.h`
   - `ArduinoJson.h`
   - `LittleFS.h`
   - `ESPmDNS.h`
   - `PubSubClient.h`
   - `Update.h`

------------


## Usage

### Initialization

Create an instance of `ESPWebConnect` and call the `begin()` method to start the library.

```cpp
ESPWebConnect webConnect;

void setup() {
    // Initialize Serial Monitor
    Serial.begin(115200);

    // Initialize the ESPWebConnect library
    webConnect.begin();
}

void loop() {
    // Handle client requests
    webConnect.handleClient();
}
```

### SPIFF Structure and system configurations

For fast configuration you don't need to recompile code to change Wi-Fi, MQTT, Web-Setting and style. On Arduino IDE `CTRL + SHIFT + P` type `Upload LittleFS to..` the select that. Ensure before perform this operation, close Serial Monitor and Serial Plotter. Using this much faster than change detail hardcoded.

File structure
```markdown
yourproject.ino
	/data
		espwebc.html
		settings-wifi.json
		settings-mqtt.json
		settings-web.json
		style.css
```
You need to follow the JSON and file structure for the library can running properly.

## Dashboard Elements
[![Dashboard Image](https://raw.githubusercontent.com/officialdanielamani/ESPWebConnect/main/image/Dashboard.png "Dashboard Image")](https://raw.githubusercontent.com/officialdanielamani/ESPWebConnect/main/image/Dashboard.png "Dashboard Image")

By default Dashboard is on `http://your-esp-ip/dashboard`

### Dashboard Configuration

Optional: Set Dashboard Path
`webConnect.setDashPath("/dashboard");`
Default value if not set is `/dashboard`

Optional: Set Title
`webConnect.setTitle("My IoT Dashboard");`
Default value if not set is `Dashboard Interface`

Optional: Set Description
`webConnect.setDesc("Control my Smart home here");`
Default value if not set is `Example interface using the ESPWebConnect library`

Optional: Set Icon URL
`webConnect.setIconUrl("https://your-icon-css");`
Default value using Fontawesome icons repo.

Optional: Set Website fetch data interval
`webConnect.setAutoUpdate(2500);`
Default value is 1000 millis if not set.

### Adding Sensors

This function will take reading and show it on the dashboard. It takes 5 arguments:
`webConnect.addSensor("ID", "Text-to-display", "Unit", "Icon", function-return-float);`

Example:
`webConnect.addSensor("tempDHT11", "Temperature", "°C", "fa fa-thermometer-half", readTemperature);`
1. `tempDHT11` is the ID (This is needed for icon color)
2. `Temperature` will show up as text in the sensor title.
3. `°C` displays the unit of temperature
4. `fa fa-thermometer-half` thermometer icon in Fontawesome
5. `readTemperature()` function that returns a float value

```cpp
float readTemperature() {
    float temperature = dht.readTemperature();
    if (isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
        return NAN;
    }
    tempDHT = temperature;
    return temperature;
}
```

### Adding Switches

You can add switches to control digital outputs (e.g., relays). The `addSwitch` method takes 4 arguments:
`webConnect.addSwitch("ID", "Text-to-display", "Icon", &variable);`

Example:
`webConnect.addSwitch("relay1", "Relay 1", "fa fa-lightbulb", &relay1);`
1. `relay1` is the ID.
2. `Relay 1` will show up as text in the switch title.
3. `fa fa-lightbulb` is the lightbulb icon in Fontawesome.
4. `&relay1` is the reference to the relay1 variable.

### Adding Buttons
- Note this function still on testing

Buttons can be added to trigger actions. The `addButton` method takes 4 arguments:
`webConnect.addButton("ID", "Text-to-display", "Icon", callbackFunction);`

Example:
`webConnect.addButton("btn1", "Press Me", "fa fa-hand-pointer", onButtonPress);`
1. `btn1` is the ID.
2. `Press Me` will show up as text on the button.
3. `fa fa-hand-pointer` is the hand pointer icon in Fontawesome.
4. `onButtonPress` is the function to be called when the button is pressed.

```cpp
void onButtonPress() {
    Serial.println("Button Pressed!");
}
```

### Colour the icon

To set the color of the icon, use the `setIconColor` method:
`webConnect.setIconColor("ID", "color");`

Example:
`webConnect.setIconColor("relay1", "#FF0000");// Set Relay 1 icon to red`
For colour can use HEX value or colour name as `red`

### Sending Notifications

Send notifications to the dashboard:
`webConnect.sendNotification("ID", "Message", "bg-color", "icon", "icon-color", duration);`

Example:
`webConnect.sendNotification("notify1", "Hello World", "blue", "fa fa-info", "white", 5);`
1. `notify1` is the ID.
2. `Hello World` is the message to display.
3. `blue` is the background color.
4. `fa fa-info` is the icon in Fontawesome.
5. `white` is the icon color.
6. `5` is the duration in seconds.

------------

## ESP32 Setting via web interface

[![Example ESP32 Configuration page](https://raw.githubusercontent.com/officialdanielamani/ESPWebConnect/main/image/Webc.png "Example ESP32 Configuration page")](hthttps://raw.githubusercontent.com/officialdanielamani/ESPWebConnect/main/image/Webc.pngtp:// "Example ESP32 Configuration page")

By default WebConfigurations is on `http://your-esp-ip/espwebc`

------------

### Wi-Fi Functions

To change the Wi-Fi detail you can use web-interface, change it on `settings-mqtt.json` or update it in the code. Using it to read and write to code this is the name to read or save to JSON.

1. `SSID_Name` Your Wi-Fi name 
2. `SSID_Pass` Leave blank if the Wi-Fi don't have password
3. `ESP_MAC` Leave the value blank
4. `SSID_AP_Name` The SSID of ESP name when in AP Wi-Fi mode
5. `SSID_AP_Pass` The SSID password of ESP when in AP Wi-Fi mode

Example use to save
```cpp
    webConnect.wifiSettings.SSID_Name = "Wifi-Jiran";
    webConnect.saveWifiSettings(webConnect.wifiSettings);
    Serial.println(webConnect.wifiSettings.SSID_Name);
```
Example to get value
```cpp
Serial.println(webConnect.wifiSettings.ESP_MAC);
```

Please note the **Wi-Fi need on 2.4Ghz** as ESP32 Hardware not support 5Ghz Wi-Fi. Some pin fucntionality also disable when using Wi-Fi functionality. For more detail visit here:

[ESP32 Pinout Reference: Which GPIO pins should you use?](https://randomnerdtutorials.com/esp32-pinout-reference-gpios "ESP32 Pinout Reference: Which GPIO pins should you use?")

Default ip when in AP mode is 

    http://192.168.4.1

------------


### MQTT Functions

To enable and use MQTT functions:
```cpp
void loop() {
// All your initializations
webConnect.enableMQTT(); //Use this to enable MQTT
}
void loop() {
    webConnect.handleClient();
    webConnect.checkMQTT(); //Check for new data from MQTT
    if (webConnect.hasNewMQTTMsg()) {
        String message = webConnect.getMQTTMsg();
        Serial.print("New message: ");
        Serial.println(message);
        webConnect.sendNotification("mqtt", message , "white", "fa-regular fa-envelope", "white", 10);
    }
}
```

To change the MQTT detail you can use web-interface, change it on `settings-mqtt.json` or update it in the code. Using it to read and write to code this is the name to read or save to JSON.

1. `MQTT_Broker` The broker that you use
2. `MQTT_Port` Port that broker use
3. `MQTT_Send` The MQTT data to broker
4. `MQTT_Recv` The MQTT data from broker
5. `MQTT_User` Username of MQTT
6. `MQTT_Pass` Password of MQTT

Example use to save

```cpp
    webConnect.mqttSettings.MQTT_Broker = "mqtt.example.com";
    webConnect.saveMQTTSettings(webConnect.mqttSettings);
    Serial.println(webConnect.mqttSettings.MQTT_Broker);
```
Example to get value

```cpp
Serial.println(webConnect.mqttSettings.MQTT_Broker);
```

*(Note MQTT function will automatically block when AP mode)*

------------

### Web Settings Functions
When Web Lock is tick and reboot, you need to enter Web Username and Web Password. If forget can reset by flash new JSON file.

[![Lock ESP32 Web Interface](https://raw.githubusercontent.com/officialdanielamani/ESPWebConnect/main/image/Web-lock.png "Lock ESP32 Web Interface")](https://raw.githubusercontent.com/officialdanielamani/ESPWebConnect/main/image/Web-lock.png "Lock ESP32 Web Interface")

If Web Name is set, you can use it's name rather than IP address (if you router our network support .local dns name). Example if set to **ESPWebConnect** you can access via `http://ESPWebConnect.local` you in same network with the ESP-32.


------------


## Example Project

Here is an example sketch that integrates several features of the `ESPWebConnect` library:

```cpp
#include "ESPWebConnect.h"

ESPWebConnect webConnect;

bool relay1 = true; //16
bool relay2 = true; //17
float count = 0;

void setup() {
    Serial.begin(115200);

    pinMode(16, OUTPUT);
    pinMode(17, OUTPUT);

    digitalWrite(16, relay1 ? HIGH : LOW);
    digitalWrite(17, relay2 ? HIGH : LOW);

    webConnect.begin();
    webConnect.setIconUrl("https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.2/css/all.min.css");
    webConnect.setCSS("style.css");
    webConnect.setAutoUpdate(2500); //change interval to 2500 ms
    webConnect.addSwitch("relay-1", "Relay-1", "fa-regular fa-lightbulb", &relay1);
    webConnect.setIconColor("relay-1", "#D3D876");
    webConnect.addSwitch("relay-2", "Relay-2", "fa-solid fa-fan", &relay2); 
    webConnect.addSensor("counter", "Counter", "", "fa fa-infinity", updateCount);
    webConnect.enableMQTT();
}

void loop() {
    webConnect.handleClient();
    webConnect.checkMQTT();
    digitalWrite(16, relay1 ? HIGH : LOW);
    digitalWrite(17, relay2 ? HIGH : LOW);

    if (webConnect.hasNewMQTTMsg()) {
        String message = webConnect.getMQTTMsg();
        Serial.print("New message: ");
        Serial.println(message);
        webConnect.sendNotification("mqtt", message , "white", "fa-regular fa-envelope", "white", 10);
    }
}

float updateCount() {
    count++;
   if (count > 50) {
        count = 0;
        webConnect.sendNotification("mqtt", "~ Reset Counter" , "white", "fa-regular fa-envelope", "white", 10);
    }
    return count;
}
```

------------


## To-Do
- API Support
- New Protocol such as Matter, Lo-Ra and ESP-Now
- Spilit the codebase to smaller chunk so can use ala-carte
- Better memory management
- More Dashboard option such as Input, Graph, Image, Joystick, Group, Tab

------------

## License

This project is will consider on GPL-3.0

## Acknowledgements

Special thanks to the contributors and the open-source community for their continuous support. Thanks to Nakamoto Albert for supporting with ProjectEDNA.
