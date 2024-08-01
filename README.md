
# ESPWebConnect

ESPWebConnect is a library designed for the ESP32 microcontroller that facilitates easy web-based configuration and control of connected devices. It supports WiFi and MQTT settings, OTA updates, and provides a user-friendly web interface for monitoring sensors and controlling actuators.

## Background

The ESP32 is a powerful microcontroller with built-in WiFi and Bluetooth capabilities, making it ideal for IoT projects. However, setting up and managing IoT devices can be complex. ESPWebConnect aims to simplify this process by providing a web interface for configuring and managing your ESP32-based projects. With this library, you can easily connect your ESP32 to a WiFi network, configure MQTT settings, and monitor and control your devices via a web dashboard.

## Features

- Easy WiFi and MQTT configuration through a web interface
- Support for Over-the-Air (OTA) firmware updates
- Real-time sensor monitoring
- Control of actuators (e.g., relays) via the web interface
- Customizable dashboard with icons and colors

## Installation

1. Download the `ESPWebConnect` library.
2. Copy the library to your Arduino `libraries` folder.
3. Ensure you have the necessary dependencies installed:
   - `ArduinoJson`
   - `LittleFS`
   - `DHT` (if using DHT sensors)

## Usage

### Setup

1. Include the necessary headers in your sketch:
   ```cpp
   #include "ESPWebConnect.h"
   #include <DHT.h>
   #include <DHT_U.h>
   ```

2. Initialize the `ESPWebConnect` and sensor objects:
   ```cpp
   ESPWebConnect webConnect;
   #define DHTPIN 32
   #define DHTTYPE DHT11
   DHT dht(DHTPIN, DHTTYPE);
   ```

3. Define your variables for relays or other actuators:
   ```cpp
   bool relay1 = true;
   bool relay2 = true;
   float count = 0;
   ```

### Example Sketch

Here is an example sketch demonstrating how to use the `ESPWebConnect` library:

```cpp
#include "ESPWebConnect.h"
#include <DHT.h>
#include <DHT_U.h>

ESPWebConnect webConnect;

#define DHTPIN 32
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

bool relay1 = true;
bool relay2 = true;
float count = 0;

void setup() {
    Serial.begin(115200);
    dht.begin();

    pinMode(16, OUTPUT);
    pinMode(17, OUTPUT);
    digitalWrite(16, relay1);
    digitalWrite(17, relay2);

    webConnect.begin();
    webConnect.setIconUrl("https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.2/css/all.min.css");
    webConnect.setCSS("style.css");
    webConnect.setAutoUpdate(2000);

    webConnect.addSensor("Temperature", "°C", "fa fa-thermometer-half", readTemperature, "#76B1D8");
    webConnect.addSensor("Humidity", "%", "fa fa-tint", readHumidity, "#B1D876");
    webConnect.addSensor("Counter", "", "fa fa-infinity", updateCount);
    webConnect.addSwitch("Relay1", "fa-regular fa-lightbulb", &relay1);
    webConnect.addSwitch("Relay2", "fa-regular fa-lightbulb", &relay2, "yellow");

    Serial.println(webConnect.wifiSettings.ESP_MAC);
    Serial.println(webConnect.mqttSettings.MQTT_Broker);

    webConnect.mqttSettings.MQTT_Broker = "holla.example.com";
    webConnect.saveMQTTSettings(webConnect.mqttSettings);
    Serial.println(webConnect.mqttSettings.MQTT_Broker);
}

void loop() {
    webConnect.handleClient();
    delay(1000);
    digitalWrite(16, relay1);
    digitalWrite(17, relay2);
}

float readTemperature() {
    float temperature = dht.readTemperature();
    if (isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
        return NAN;
    }
    return temperature;
}

float readHumidity() {
    float humidity = dht.readHumidity();
    if (isnan(humidity)) {
        Serial.println("Failed to read from DHT sensor!");
        return NAN;
    }
    return humidity;
}

float updateCount() {
    count++;
    return count;
}
```

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Contributing

Contributions are welcome! Please submit pull requests or open issues for any bugs or feature requests.
