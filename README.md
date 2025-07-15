# ESPWebConnect Library for ESP32

## Overview

`ESPWebConnect` is a comprehensive library designed for the ESP32 platform. It provides easy integration for WiFi, MQTT, Web-Dashboard, OTA update to monitor and control your IoT devices. This library enables seamless connectivity and interaction with sensors, switches, and buttons through a web interface. More control, more customization, no longer need depending on 3rd party cloud provider. TLDR, it about like Blynk but can run offline with more customization

You can try use our installer at to test some example projects: ~~https://danielamani.com/project/core_firmware/index.html~~
(Note only on Chrome Desktop, ensure disable Serial Monitor and Serial Plotter first)

------------


## Note
This library is in **BETA** development and for internal usage of ProjectEDNA. Don't use in deployment or mission critical project as there are many changes ahead. This library are the one of key part of `Integration Of Iot Platform Environment For Web Server & Global Connectivity Based On Esp32`

After discussion with ProjectEDNA members, it's suggest to migrate to new architecture:
- ESPWebConnectCore for WiFi, Local Account and download add-ons
- ESPWebConnectFile for basic file browser for SPIFFS and SD-Card
- ESPWebConnectDash for dashboard 
- ESPWebConnectForm for creating form to save settings
- ESPWebConnectOTA for handle OTA

On core system it will check if the ESPWebC component is available in the SPIFFS and need update or not. If don't have it will download the component needed, bypass requirement need to SPIFFS upload to ESP32 when compile the project.

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

## To-Do
- [ ] New Protocol such as Matter, Lo-Ra and ESP-Now
- [x] Change addSensor to accept Float, Int, String
- [ ] Stable MQTT intergation
- [ ] Telegram intergation
- [x] addSensor no need callback functions
- [ ] Custom Icon size
- [ ] More CSS option
- [ ] Upload own CSS from web interface
- [ ] Adding Slider
- [ ] Adding Joystick
- [x] Adding input Number and Text
- [ ] Adding regex option on input
- [ ] More option for input 
- [ ] Adding bar graph
- [ ] Adding line graph
- [x] OTA from .bin
- [x] OTA from URL
- [x] OTA URL support MD5 hash integrity
- [ ] Upload custom CSS on web interface
- [ ] Spilit the codebase to smaller chunk so can use ala-carte
- [ ] Better memory management
- [ ] Better documentation
- [ ] More `#Define` option for reduce compilation size


- Graph,Slider,Joystick in Alpha development. We currently collabrate to using vanillaJS as main framework.

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
**- Note: This documentation is for Version 2.0 ESPWebC**

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
  // No need callback
}
```

All widgets using XHR poling (time set by `setAutoUpdate()`. When the interval time reach it will get the value of the variable it in widgets setup (eg: `&value`). Example if `int value = 25` change it value to `35` on next poling rate it will send the updated value in this case `35`.

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
		dash.js
		style.css
```
You need to follow the JSON and file structure for the library can running properly.

## Dashboard Example
[![Dashboard Image](https://raw.githubusercontent.com/officialdanielamani/ESPWebConnect/main/image/Dashboard.png "Dashboard Image")](https://raw.githubusercontent.com/officialdanielamani/ESPWebConnect/main/image/Dashboard.png "Dashboard Image")

- By default Dashboard is on `http://your-esp-ip/dashboard`.
- Click **Select Widget** and choose widgets that need to be display on dashboard.
- Drag,resize and put the widget on where you want.
- If want to remove widget, simply click `X` on top right of widget.
- To choose theme, click **Theme** and select theme that you want.
- Default theme is **Keqing**, there also **Light** and **Dark**
- To save your configurations, click **Save**, when the page reload, it will follow the last save configuration.
- To clear configuration, click **Clear**, all widgets will be remove and cleared.

**Limitation**
- Widgets selection depend on the ESP32 program
- The configuration is save on the spesific browser cache. If you clear cache or ESP32 using different IP or you change device, the configuration will not show up as it store configuration locally on browser cache.
- It is recommended to clear the Dashboard if you have make any changes on dashboard widgets in the ESP32 program.

### Dashboard Configuration

- Optional: Set Dashboard Path
`webConnect.setDashPath("/dashboard");`
Default value if not set is `/dashboard`

- Optional: Set Icon URL
`webConnect.setIconUrl("https://your-icon-css");`
Default value using Fontawesome icons repo.

- Optional: Set Dashboard fetch data interval
`webConnect.setAutoUpdate(2500);`
Default value `5000` if not set.

- Optional: Set Dashboard info
`webConnect.setDashInfo("Title", Desription", "https://danielamani.com/image/SmartHome.png", "Footer Text");`
Not all arguments are required. If to skip just ` ""` part that not needed, it will fallback to default value.

- Optional: Set Manufacture info
`setManifactureInfo("Developer", "Device", "Desription ", "Version");`
Not all arguments are required. If to skip just ` ""` part that not needed, it will fallback to default value. This will be dsipaly on system information on `/espwebc`


## Dashboard Widgets
Note: For all **widgets ID**, it need **unique** for each widgets as XHR polling will use the widgets ID to get value from the variables. It advice to **not have whitespace or special character in the ID**.

### Adding Sensors/Reading/Display

This function will take reading and show it on the dashboard. The `addSensor()` takes **6** arguments:

```cpp
void addSensor(const char *id, const char *name, const char *desc, const char *icon, [&Variable] , const char *unit);
```

*Variable can only be in float, int or String*

Example:
```cpp
webConnect.addSensor("tempDHT11", "Temperature", "Indoor sensor", "fa fa-thermometer-half", &tempDHT, "C");
```

1. `tempDHT11` is the ID (This is needed for icon color)
2. `Temperature` will show up as text in the sensor title.
3. `Indoor sensor` will show up as text for desription 
4. `fa fa-thermometer-half` thermometer icon in Fontawesome
5. `&tempDHT` the variable need updated (*Example float temperature*)
6. `C` will show up as unit (right from the value)


### Adding Switches

You can add switches to control digital outputs (e.g., relays). The `addSwitch()` method takes **5** arguments:
```cpp
void addSwitch(const char *id, const char *name, const char *desc, const char *icon, bool *state);
```
*Variable can be in bool only*

Example:
```cpp
webConnect.addSwitch("relay-1", "Fan", "Main ceiling fan", "fa-solid fa-fan", &relay1);
```

1. `relay1` is the ID.
2. `Fan` will show up as text in the switch title.
2. `Main ceiling fan` will show up as text in the description.
3. `fa-solid fa-fan` is the lightbulb icon in Fontawesome.
4. `&relay1` is the reference to the relay1 variable.

*Switch is instant, will not follow `setAutoUpdate()` value. But the status variable data from ESP32 will update according `setAutoUpdate()` *


### Adding Buttons
Buttons can be added to trigger actions. The `addButton()` method takes **5** arguments:

```cpp
void addButton(const char *id, const char *name, const char *desc, const char *icon, std::function<void()> onPress);
```
*Note: It will need function void() to run*

Example:
```cpp
webConnect.addButton("btn1", "Emergency", "Trigger the alarm", "fa-solid fa-triangle-exclamation", onButtonPress);
//Other code
void onButtonPress() {
    Serial.println("Button Pressed!");
}
```

1. `btn1` is the ID.
2. `Emergency` will show up as text on the button.
3. `Trigger the alarm` will show up as text for desription.
4. `"fa-solid fa-triangle-exclamation` is the hand pointer icon in Fontawesome.
5. `onButtonPress` is the function to be called when the button is pressed.

*Button is instant, will not follow `setAutoUpdate()` value*


### Adding Input (Number)
Input number can take and save numeral value to the variable. The `addInputNum()` method takes **5** arguments. It only accept *int and float* as variable

```cpp
void addInputNum(const char *id, const char *name, const char *desc, const char *icon, [&Variable] );
```
Example:
```cpp
webConnect.addInputNum("setWarnTemp", "Set High Temperature", "", "fa-solid fa-temperature-high", &setTemp);
```

1. `setWarnTemp` is the ID.
2. `Set High Temperature` will show up as text in the Input title.
3. ` ` will show up as text for desription (This case set as blank).
4. `"fa-solid fa-temperature-high` is icon in Fontawesome.
5. `&setTemp`  is the reference to the setTemp variable.

*Input is instant, will not follow `setAutoUpdate()` value*

### Adding Input (String)
Input number can take and save numeral value to the variable. The `addInputNum()` method takes **5** arguments. It only accept *String* as variable.

```cpp
void addInputNum(const char *id, const char *name, const char *desc, const char *icon, [&Variable] );
```
Example:
```cpp
webConnect.addInputNum("setTextDisplay", "Send Text", "Display on OLED", "fa-solid fa-pencil", &displaytext);
```

1. `setTextDisplay` is the ID.
2. `Send Text` will show up as text in the Input title.
3. `Display on OLED` will show up as text for desription.
4. `"fa-solid fa-temperature-high` is icon in Fontawesome.
5. `&displaytext`  is the reference to the setTemp variable.

*Input is instant, will not follow `setAutoUpdate()` value*

### Sending Notifications

Input number can take and save numeral value to the variable. The `addInputNum()` method takes 5 arguments. It only accept *int and float* as variable

```cpp
void sendNotification(const String &id, const String &message, const String &messageColor, const String &icon, const String &iconColor, int timeout);
```

Example:
```cpp
webConnect.sendNotification("mqtt", displaytext, "white", "fa-regular fa-envelope", "blue", 10);
```

1. `mqtt` is the ID.
2. `displaytext` is the String to display text. You also can use like ` "Hello" `
3. `white` is the message color.
4. `fa-regular fa-envelope` is the icon in Fontawesome.
5. `blue` is the icon color.
6. `5` is the duration in seconds to display notification.

*This function can be call in anywhere that you want to show notification. No need to initiatize.*

### Set Icon Color

To set the color of the icon, use the `setIconColor` method:
```cpp
webConnect.setIconColor("ID", "color");
```
Example:
```cpp
webConnect.setIconColor("relay1", "#FF0000");// Set Relay 1 icon to red
```
For colour can use HEX value or colour name as `red`
If the icon colour not set, it will use default theme icon colour.


------------

## ESPWebC Configurations Page

In this page there are:
- Wi-Fi Settings
- MQTT Settings (Ignore is cannot fetch as this will show up if MQTT is not used)
- Web Settings
- System Information
- OTA Update via File Upload
- OTA Update via URL
- Reboot ESP32

Below example of Wi-Fi Settings

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

## OTA update

### OTA via File Upload

To update firware can go to ESP32 configuration page at `http://your-esp-ip/espwebc`. Can be updated using .bin file or via URL.
For generating .bin file, on Arduino IDE top navigation go to `Sketch -> Export Compile Binary`. If success on your Arduino project there will be a new folder called build. Go inside and find folder related to ESP32. In last folder there will be;
```markdown
yourproject.ino
	/build
		/esp32.esp32.esp32da
			yourproject.ino.bin
			yourproject.ino.bootloader.bin
			yourproject.ino.elf
			yourproject.ino.map
			yourproject.ino.merged.bin
			yourproject.ino.partitions.bin
```
Select `yourproject.ino.bin` as .bin file to upload. Wait awhile until your ESP32 reboot. If success you can see the changes and update.

### OTA via URL

You can update via URL. It need URL that point to publicly JSON with this scheme:

```JSON
[
    {
        "Developer": "EDNA",
        "Author": "Daniel Amani",
        "Image": "https://danielamani.com/image/logo.jpg",
        "Title": "Basic Smart Home",
        "Info": "Control 4 relays only",
        "Version": "V1.2",
        "URL": "https://raw.githack.com/officialdanielamani/officialdanielamani.github.io/main/project/core_firmware/firmware/smarthome_basic/program.bin",
        "MD5": "c356421dc4ca935fc001efdb7b771a57"
    },
    {
        "Developer": "",
        "Author": "",
        "Image": "",
        "Title": "",
        "Info": "",
        "Version": "",
        "URL": "",
		"MD5": ""
    }
]
```

The first is example of how the JSON will be look.

1. `Developer` or manufacture (Optional)
2. `Author` or who publish it (Optional)
3. `Image` point to icon / image related to device (Optional)
4. `Title` is the project/device name ()ptional)
5. `URL` is where `project.bin` is located, need public access link **(Required)**
6. `MD5` is hash of `project.bin` **(Required)**

[![OTA URL](https://raw.githubusercontent.com/officialdanielamani/ESPWebConnect/main/image/OTA-URL.png "Update via URL")](https://raw.githubusercontent.com/officialdanielamani/ESPWebConnect/main/image/OTA-URL.png "Update via URL")

**How to update via URL**
- Put the URL that pointing to the `JSON` list of firmware. 
- If not set it will use default built-in URL.
- Click `Load Firmware Option` will show up the available list. 
- Click `Choose this Firmware` to update.
- If the `program.bin` and `MD5` hash is not match the OTA will fail as it may corrupted.
- If no problem, the ESP32 will reboot and updated firmware will be running.

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

    webConnect.setAutoUpdate(2500);
	webConnect.setDashInfo("Smart Home", "basic control", "https://danielamani.com/image/SmartHome.png", "");
  webConnect.setManifactureInfo("EDNA", "Smart Home", "Basic", "V0.1.7");
    webConnect.addSwitch("relay-1", "Switch 1","Light", "fa-regular fa-lightbulb", &relay1);
    webConnect.setIconColor("relay-1", "#D3D876");
    webConnect.addSwitch("relay-2", "Switch 2", "Fan", fa-solid fa-fan", &relay2); 
    webConnect.addSensor("counter", "Counter", "", "fa fa-infinity", &count, "++");
    webConnect.addButton("btn1", "Press Me", "fa fa-hand-pointer", onButtonPress);
    webConnect.addInputText("input-text", "Enter text:", "","fa-solid fa-pencil", &text);
	webConnect.addInputNum("input-text", "Enter number:", "","fa-solid fa-pencil",  &count);
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
  Serial.println("Button is press and show stored text");
  webConnect.sendNotification("mqtt", text, "white", "fa-regular fa-envelope", "blue", 10);
}

float updateCount() {
    count++;
    if (count > 50) {
        count = 0;
        webConnect.sendNotification("countNoti", "Reset Count", "white", "fa-regular fa-envelope", "black", 10);
    }
    return count;
}
```

------------
## Docker Compose Setup
Below is example of Docker compose containing:
- Node-Red
- MQTT Mosquitto
- SQLitebrowser
- Cloudflare Tunnel

This setup using Portainer container manager.

```YAML
version: '3.8'

services:
  node-red:
    image: nodered/node-red:latest
    container_name: noderedtest
    restart: always
    ports:
      - "1880:1880"
    volumes:
      - /portainer/Files/AppData/Test/nodered:/data

  mqtt:
    image: eclipse-mosquitto:latest
    container_name: mosquittotest
    restart: always
    ports:
      - "1883:1883"
      - "9001:9001"
    volumes:
      - /portainer/Files/AppData/Test/mqtt/data:/mosquitto/data
      - /portainer/Files/AppData/Test/mqtt/config:/mosquitto/config
      - /portainer/Files/AppData/Test/mqtt/log:/mosquitto/log
      
  sqlitebrowser:
    image: lscr.io/linuxserver/sqlitebrowser:latest
    container_name: sqlitebrowser
    environment:
      - PUID=1000
      - PGID=1000
      - TZ=Etc/UTC
      - CUSTOM_USER=CHANGEME
      - PASSWORD=CHANGEME
    volumes:
      - /portainer/Files/AppData/Test/sqlite/config:/config
    ports:
      - "3000:3000"
      - "3001:3001"
    restart: unless-stopped

  cloudflared:
    image: cloudflare/cloudflared:latest
    container_name: cloudflaredtest
    restart: always
    environment:
      TUNNEL_TOKEN: "<YOUR_CLOUDFLARE_TUNNEL_TOKEN>"
```


## License

This project is will consider on GPL-3.0

## Acknowledgements

Special thanks to the contributors and the open-source community for their continuous support. Thanks to Nakamoto Albert for supporting with ProjectEDNA.
