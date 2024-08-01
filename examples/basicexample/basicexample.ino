#include "ESPWebConnect.h"
#include <DHT.h>
#include <DHT_U.h>

ESPWebConnect webConnect;

#define DHTPIN 32        // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11

DHT dht(DHTPIN, DHTTYPE);

bool relay1 = true; // Example relay 1
bool relay2 = true; // Example relay 2

float count = 0;

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial);

    // Initialize the DHT11 sensor
    dht.begin();

    // Initialize webConnect
    pinMode(16, OUTPUT); // Example pin for relay 1
    pinMode(17, OUTPUT); // Example pin for relay 2
    digitalWrite(16, relay1); // Set initial state for relay 1
    digitalWrite(17, relay2); // Set initial state for relay 2

    webConnect.begin();

    // Set icon and CSS URLs
    webConnect.setIconUrl("https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.2/css/all.min.css");
    webConnect.setCSS("style.css");

    // Set auto-update interval
    webConnect.setAutoUpdate(2000);

    // Add sensors with colors
    webConnect.addSensor("Temperature", "°C", "fa fa-thermometer-half", readTemperature, "#76B1D8");
    webConnect.addSensor("Humidity", "%", "fa fa-tint", readHumidity, "#B1D876");
    webConnect.addSensor("Counter", "", "fa fa-infinity", updateCount);

    // Add switches with colors
    webConnect.addSwitch("Relay1", "fa-regular fa-lightbulb", &relay1);
    webConnect.addSwitch("Relay2", "fa-regular fa-lightbulb", &relay2, "yellow");

    // Example reading the saved value
    Serial.println(webConnect.wifiSettings.ESP_MAC); // get ESP MAC address
    Serial.println(webConnect.mqttSettings.MQTT_Broker); // get MQTT broker name

    // Update MQTT broker and save settings
    webConnect.mqttSettings.MQTT_Broker = "holla.example.com";
    webConnect.saveMQTTSettings(webConnect.mqttSettings); // Save the new value
    Serial.println(webConnect.mqttSettings.MQTT_Broker); // get MQTT broker name
}

void loop() {
    webConnect.handleClient();
    delay(1000); // Just to slow down the loop
    digitalWrite(16, relay1); // Update relay state based on the switch
    digitalWrite(17, relay2); // Update relay state based on the switch
}

// Function to read temperature from DHT11
float readTemperature() {
    float temperature = dht.readTemperature();
    if (isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
        return NAN;
    }
    return temperature;
}

// Function to read humidity from DHT11
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
