#ifndef ESPWEBCONNECT_H
#define ESPWEBCONNECT_H

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <vector>
#include <functional>

class ESPWebConnect {
public:
    ESPWebConnect();
    void begin();
    void handleClient();

    // Public settings structures
    struct WifiSettings {
        String SSID_Name;
        String SSID_Pass;
        String ESP_MAC;
        String SSID_AP_Name;
        String SSID_AP_Pass;
    } wifiSettings;

    struct MQTTSettings {
        String MQTT_Broker;
        int MQTT_Port;
        String MQTT_Send;
        String MQTT_Recv;
        String MQTT_User;
        String MQTT_Pass;
    } mqttSettings;

    const WifiSettings& getWifiSettings() const;
    const MQTTSettings& getMQTTSettings() const;

    void setIconUrl(String url);
    void setCSS(String url);
    void setAutoUpdate(unsigned long interval);
    void addSensor(String name, String unit, String icon, std::function<float()> readFunction, String color = "inherit");
    void addSwitch(String name, String icon, bool* state, String color = "inherit");

    void updateDashboard();

    void saveWifiSettings(const WifiSettings &settings);
    void saveMQTTSettings(const MQTTSettings &settings);

private:
    WebServer server;
    struct WebSettings {
        String Web_User;
        String Web_Pass;
        String Web_name;
        bool Web_Lock;
    };
    WebSettings webSettings;

    struct Sensor {
        String name;
        String unit;
        String icon;
        std::function<float()> readFunction;
        String color;
    };

    struct Switch {
        String name;
        String icon;
        bool* state;
        String color;
    };

    std::vector<Sensor> sensors;
    std::vector<Switch> switches;

    String iconUrl;
    String cssUrl;
    unsigned long updateInterval = 5000;

    void handleRoot();
    void handleESPWebC();
    void handleCSS();
    void handleDashboard();
    void handleReadings();
    void handleSwitches();
    void handleToggleSwitch();

    bool readWifiSettings(WifiSettings &settings);
    bool readMQTTSettings(MQTTSettings &settings);
    bool readWebSettings(WebSettings &settings);
    void handleSaveWifi();
    void handleSaveMQTT();
    void handleSaveWeb();
    void handleGetWifiSettings();
    void handleGetMQTTSettings();
    void handleGetWebSettings();
    bool configureWiFi(const char *ssid, const char *password);
    void startAP(const char *ssid, const char *password);
    void handleOTAUpdate();
    void handleReboot();
    bool checkAuth();
    void saveSettings(const char* filename, const JsonDocument& doc);
};

#endif
