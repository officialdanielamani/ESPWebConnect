# ESPWebConnect Library for ESP32

## Overview

`ESPWebConnect` is a comprehensive library designed for the ESP32 platform. It provides easy integration for WiFi, MQTT, Web-Dashboard, OTA update to monitor and control your IoT devices. This library enables seamless connectivity and interaction with sensors, switches, and buttons through a web interface. More control, more customization, no longer need depending on 3rd party cloud provider.

You can try use our installer at to test some example projects: ~~https://danielamani.com/project/core_firmware/index.html~~
(Note only on Chrome Desktop, ensure disable Serial Monitor and Serial Plotter first)

------------


## Note
This library is in **BETA** development and for internal usage of ProjectEDNA. Don't use in deployment or mission critical project as there are many changes ahead. This library are the one of key part of `Integration Of Iot Platform Environment For Web Server & Global Connectivity Based On Esp32`

## Features

- Easy WiFi and MQTT configuration through a web interface (No more hardcoded WiFi).
- Connects to WiFi networks and sets up Access Point mode for configuration.
- Creates a Web Interface for configuration and control.
- Customizable dashboard with icons and colors.
- Publishes and subscribes to MQTT topics.
- Supports LittleFS for file storage.
- Integrates ESPAsyncwebserver and WebSockets for real-time updates.
- Allows the addition of sensors, switches, and buttons to a customizable dashboard.
- Over The Air (OTA) update, can update firmware using .bin or via URL

------------


## Installation

1. Download the `ESPWebConnect` library (On top Green button that show `<> Code`, Click download Zip).
2. On Arduino IDE, click Sketch, Include Library, Add .ZIP library.
3. Ensure you have the necessary dependencies or library installed:
   - `Wifi.h`
   - `WiFiUdp.h`
   - `ESPAsyncWebServer.h`
   - `WebSocketsServer.h`
   - `WiFiClientSecure.h`
   - `ArduinoJson.h`
   - `LittleFS.h`
   - `ESPmDNS.h`
   - `Update.h`
   - `PubSubClient.h`

------------


## Usage

### Core System Selection
To reduce the library code during compilation you can enable MQTT functionality by adding or enable `#define ENABLE_MQTT` on ESPWebConnect.cpp on like this example path of Arduino library `C:\Users\YOUR-PC\Documents\Arduino\libraries\ESPWebConnect`

If you try to call any MQTT functionality without enable this flag will cause the compiler errors.

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

Optional: Set Dashbaord Card size
`webConnect.setAllCardSize(180, 180); `
Default value is 200 if not set (Unit: pixel).

### Adding Sensors

This function will take reading and show it on the dashboard. It takes 5 arguments:
```cpp
webConnect.addSensor("ID", "Text-to-display", "Unit", "Icon", variable);
```

*Variable can be in float, int or String*

Example:
```cpp
webConnect.addSensor("tempDHT11", "Temperature", "°C", "fa fa-thermometer-half", &temperature);
```

1. `tempDHT11` is the ID (This is needed for icon color)
2. `Temperature` will show up as text in the sensor title.
3. `°C` displays the unit of temperature
4. `fa fa-thermometer-half` thermometer icon in Fontawesome
5. `&temperature` the variable need updated (*Example float temperature*)

The value will be update after set data interval.

### Adding Switches

You can add switches to control digital outputs (e.g., relays). The `addSwitch` method takes 4 arguments:
```cpp
webConnect.addSwitch("ID", "Text-to-display", "Icon", &variable);
```

Example:
```cpp
webConnect.addSwitch("relay1", "Relay 1", "fa fa-lightbulb", &relay1);
```

1. `relay1` is the ID.
2. `Relay 1` will show up as text in the switch title.
3. `fa fa-lightbulb` is the lightbulb icon in Fontawesome.
4. `&relay1` is the reference to the relay1 variable.

### Adding Buttons
Buttons can be added to trigger actions. The `addButton` method takes 4 arguments:
```cpp
webConnect.addButton("ID", "Text-on-button", "Icon", callbackFunction);
```

Example:
```cpp
webConnect.addButton("btn1", "Press Me", "fa fa-hand-pointer", onButtonPress);
//Other code
void onButtonPress() {
    Serial.println("Button Pressed!");
}
```

1. `btn1` is the ID.
2. `Press Me` will show up as text on the button.
3. `fa fa-hand-pointer` is the hand pointer icon in Fontawesome.
4. `onButtonPress` is the function to be called when the button is pressed.


### Colour the icon

To set the color of the icon, use the `setIconColor` method:
```cpp
webConnect.setIconColor("ID", "color");
```

Example:
```cpp
webConnect.setIconColor("relay1", "#FF0000");// Set Relay 1 icon to red
```

For colour can use HEX value or colour name as `red`

### Set Individual Card size

The icon size is using multiplier from default/setAllCardSize value.
```cpp
webConnect.setCardSize("ID",float-X,float-Y);
```
Example:
```cpp
webConnect.setCardSize("inputTimer",0.8,2);
```

1. `inputTimer` is the ID of card that need to change
2. `0.8` will be times with value X (width card).
3. `2` will be times with value Y (height card).

### Sending Notifications

Send notifications to the dashboard:
```cpp
webConnect.sendNotification("ID", "Message", "bg-color", "icon", "icon-color", duration);
```

Example:
```cpp
webConnect.sendNotification("notify1", "Hello World", "blue", "fa fa-info", "white", 5);
```

1. `notify1` is the ID.
2. `Hello World` is the message to display.
3. `blue` is the background color.
4. `fa fa-info` is the icon in Fontawesome.
5. `white` is the icon color.
6. `5` is the duration in seconds.

------------

## OTA update

###OTA via File Upload
To update firware can go to ESP32 configuration page at `http://your-esp-ip/espwebc`. Can be updated using .bin file or via URL.
For generating .bin file, on Arduino IDE top navigation go to `Sketch -> Export Compile Binary`. If success on your Arduino project there will be a new folder called build. Go inside and find folder related to ESP32. In last folder there will be;
```markdown
yourproject.ino
	/esp32.esp32.esp32da
		yourproject.ino.bin
		yourproject.ino.bootloader.bin
		yourproject.ino.elf
		yourproject.ino.map
		yourproject.ino.merged.bin
		yourproject.ino.partitions.bin
```
Select `yourproject.ino.bin` as .bin file to upload. Wait awhile until your ESP32 reboot. If success you can see the changes and update.

###OTA via URL
-TODO

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
*Note: This function is Disable by default. This function on testing, if not suitable we will remove management on Sending and Recieving*

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
int numbers = 0;
String text;
unsigned long preTime = 0;

void setup() {
    Serial.begin(115200);

    pinMode(16, OUTPUT);
    pinMode(17, OUTPUT);

    digitalWrite(16, relay1 ? HIGH : LOW);
    digitalWrite(17, relay2 ? HIGH : LOW);

    webConnect.begin();
    webConnect.setIconUrl("https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.2/css/all.min.css");
    webConnect.setCSS("style.css");
	webConnect.setAllCardSize(180, 180);
	webConnect.setDesc("My Smart Home Panel");
    webConnect.setAutoUpdate(2500);
    webConnect.addSwitch("relay-1", "Relay-1", "fa-regular fa-lightbulb", &relay1);
    webConnect.setIconColor("relay-1", "#D3D876");
    webConnect.addSwitch("relay-2", "Relay-2", "fa-solid fa-fan", &relay2); 
    webConnect.addSensor("counter", "Counter", "", "fa fa-infinity", &count);
    webConnect.addButton("btn1", "Press Me", "fa fa-hand-pointer", onButtonPress);
    webConnect.addInputText("input-text", "Enter text:", "fa-solid fa-pencil", &text);
    webConnect.addSensor("text-val", "Input text value", "", "fa-solid fa-print", &text);
}

void loop() {
    digitalWrite(16, relay1 ? HIGH : LOW);
    digitalWrite(17, relay2 ? HIGH : LOW);
	
	unsigned int taskDelay = 5000;
    	if((millis()-preTime)>taskDelay){
	       updateCount();
      preTime=millis();
    }
}

void onButtonPress() {
  Serial.println("Button is press");
  webConnect.sendNotification("noti-count", "Button is pressed" , "white", "fa-regular fa-envelope", "white", 10);
}

float updateCount() {
    count++;
    if (count > 50) {
        count = 0;
        webConnect.sendNotification("noti-count", "~ Reset Counter" , "white", "fa-regular fa-envelope", "white", 10);
    }
    return count;
}
```

------------


## To-Do
- [ ] New Protocol such as Matter, Lo-Ra and ESP-Now
- [x] Change addSensor to accept Float, Int, String
- [x] addSensor no need callback functions
- [x] Custom Card size
- [ ] Custom Icon size
- [ ] More CSS option
- [ ] Adding Slider
- [ ] Adding Joystick
- [x] Adding input Number and Text
- [ ] Adding regex option on input
- [ ] Adding bar graph
- [ ] Adding line graph
- [x] OTA from .bin
- [x] OTA from URL
- [ ] Upload custom CSS on web interface
- [ ] Device/firmware info and debug
- [ ] Adding firmware info field and JSON on SPIFF
- [ ] Spilit the codebase to smaller chunk so can use ala-carte
- [ ] Better memory management
- [x] Change `String` to `const char*` for better memory management
- [ ] Better documentation
- [ ] More `#Define` option for reduce compilation size 

------------

## License

This project is will consider on GPL-3.0

## Acknowledgements

Special thanks to the contributors and the open-source community for their continuous support. Thanks to Nakamoto Albert for supporting with ProjectEDNA.
