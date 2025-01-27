#ifndef ESPWEBCONNECT_H
#define ESPWEBCONNECT_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <vector>
#include <functional>
#include <map>
#include "esp_task_wdt.h"

//#define ENABLE_MQTT
// #define ENABLE_DEBUG
// #define ENABLE_DEBUG_INFO

#ifdef ENABLE_MQTT
#include <PubSubClient.h>
#endif

class ESPWebConnect
{
public:
    ESPWebConnect();
    void begin();

    struct WifiSettings
    {
        String SSID_Name;
        String SSID_Pass;
        String ESP_MAC;
        String SSID_AP_Name;
        String SSID_AP_Pass;
    } wifiSettings;

    const WifiSettings &getWifiSettings() const;

    void setIconUrl(const String &url);
    void setCSS(const String &url);
    void setAutoUpdate(unsigned long interval);

    void addSensor(const char *id, const char *name, const char *desc, const char *icon, int *intValue, const char *unit);
    void addSensor(const char *id, const char *name, const char *desc, const char *icon, float *floatValue, const char *unit);
    void addSensor(const char *id, const char *name, const char *desc, const char *icon, String *stringValue, const char *unit);

    void addSwitch(const char *id, const char *name, const char *desc, const char *icon, bool *state);
    void addButton(const char *id, const char *name, const char *desc, const char *icon, std::function<void()> onPress);
    void addInputNum(const char *id, const char *name, const char *desc, const char *icon, int *variable);
    void addInputNum(const char *id, const char *name, const char *desc, const char *icon, float *variable);
    void addInputText(const char *id, const char *name, const char *desc, const char *icon, String *variable);

    void setIconColor(const char *id, const char *color);

    void setDashPath(const String &path);
    void setDashInfo(const char *title = nullptr, const char *description = nullptr, const char *imageurl = nullptr, const char *footer = nullptr);
    void setManifactureInfo(const char *developer = nullptr, const char* device = nullptr, const char *descDevice = nullptr, const char *versionDevice = nullptr);

    void updateDashboard();
    void sendNotification(const String &id, const String &message, const String &messageColor, const String &icon, const String &iconColor, int timeout);
    void handleButtonPress(AsyncWebServerRequest *request);

    void sendGraphData();

    struct DashboardElement
    {
        enum Type
        {
            SENSOR_INT,
            SENSOR_FLOAT,
            SENSOR_STRING,
            SWITCH,
            BUTTON,
            INPUT_NUM,
            INPUT_TEXT
        };
        Type type;

        const char *id;
        const char *name;
        const char *desc;
        const char *unit;
        const char *icon;
        const char *color;

        union
        {
            int *intValue;
            float *floatValue;
            String *stringValue;
            bool *state;
        };
        std::function<void()> onPress;

        // Updated constructor for Display Sensor [Int]
        DashboardElement(const char *id, const char *name, const char *desc, const char *icon, int *intValue, const char *unit)
            : type(SENSOR_INT), id(id), name(name), desc(desc), icon(icon), color(nullptr), intValue(intValue), unit(unit) {}

        // Updated constructor for Display Sensor [Float]
        DashboardElement(const char *id, const char *name, const char *desc, const char *icon, float *floatValue, const char *unit)
            : type(SENSOR_FLOAT), id(id), name(name), desc(desc), icon(icon), color(nullptr), floatValue(floatValue), unit(unit) {}

        // Updated constructor for Display Sensor [String]
        DashboardElement(const char *id, const char *name, const char *desc, const char *icon, String *stringValue, const char *unit)
            : type(SENSOR_STRING), id(id), name(name), desc(desc), icon(icon), color(nullptr), stringValue(stringValue), unit(unit) {}

        // Updated constructor for Input Number [Int]
        DashboardElement(const char *id, const char *name, const char *desc, const char *icon, int *intValue)
            : type(INPUT_NUM), id(id), name(name), desc(desc), unit(""), icon(icon), color(nullptr), intValue(intValue) {}

        // Updated constructor for Input Number [Float]
        DashboardElement(const char *id, const char *name, const char *desc, const char *icon, float *floatValue)
            : type(INPUT_NUM), id(id), name(name), desc(desc), unit(""), icon(icon), color(nullptr), floatValue(floatValue) {}

        // Updated constructor for Text Input [String]
        DashboardElement(const char *id, const char *name, const char *desc, const char *icon, String *stringValue)
            : type(INPUT_TEXT), id(id), name(name), desc(desc), unit(""), icon(icon), color(nullptr), stringValue(stringValue) {}

        // Constructor for Switch [Bool]
        DashboardElement(const char *id, const char *name, const char *desc, const char *icon, bool *state)
            : type(SWITCH), id(id), name(name), desc(desc), unit(""), icon(icon), color(nullptr), state(state) {}

        // Constructor for Button [Function]
        DashboardElement(const char *id, const char *name, const char *desc, const char *icon, std::function<void()> onPress)
            : type(BUTTON), id(id), name(name), desc(desc), unit(""), icon(icon), color(nullptr), onPress(onPress) {}

    };

    String getWidgetType(DashboardElement::Type type);

    std::vector<DashboardElement> dashboardElements;

    void saveWifiSettings(const WifiSettings &settings);
    void performOTAUpdateFromURL(const String &firmwareURL);

#ifdef ENABLE_MQTT
    struct MQTTSettings
    {
        String MQTT_Broker;
        int MQTT_Port;
        String MQTT_Send;
        String MQTT_Recv;
        String MQTT_User;
        String MQTT_Pass;
    } mqttSettings;

    const MQTTSettings &getMQTTSettings() const;
    void enableMQTT();
    void reconnectMQTT();
    void publishToMQTT(const String &payload);
    void checkMQTT();
    bool hasNewMQTTMsg() const;
    String getMQTTMsg();
    void saveMQTTSettings(const MQTTSettings &settings);
#endif

private:
    String ESPwebCVersion = "2.0.1"; //ESPWebC Version , do not change
    WiFiClient wifiClient;
    AsyncWebServer server;
    AsyncWebSocket ws;

    struct WebSettings
    {
        String Web_User;
        String Web_Pass;
        String Web_name;
        bool Web_Lock;
    };
    WebSettings webSettings;
    bool isAPMode() const;

    String iconUrl;
    String cssUrl;

    String dashTitle = "Dashboard Interface";
    String dashDescription = "Example interface using the ESPWebConnect library";
    String dashImageUrl = "https://danielamani.com/image/logo.jpg";
    String dashFooter = "";
    
    String manufacturerDeveloper = "DANP Technologies";
    String manufacturerDevice = "ESP32";
    String manufacturerDescDevice = "EDNA Edge IoT";
    String manufacturerVersionDevice = "1.0";

    String dashPath;

    unsigned long updateInterval = 5000;

    String generateAllReadingsJSON();
    void handleToggleSwitch(AsyncWebServerRequest *request);
    void handleNotification(AsyncWebServerRequest *request);
    void handleFirmwareUpload(AsyncWebServerRequest *request);

    void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

    bool checkAuth(AsyncWebServerRequest *request);
    bool readWifiSettings(WifiSettings &settings);
    bool readWebSettings(WebSettings &settings);

    String generateDashboardHTML();

    void handleReboot();
    void startAP(const char *ssid, const char *password);
    bool configureWiFi(const char *ssid, const char *password);

#ifdef ENABLE_MQTT
    void handleGetMQTTSettings(AsyncWebServerRequest *request);
    bool readMQTTSettings(MQTTSettings &settings);
    void mqttCallback(char *topic, byte *payload, unsigned int length);
    PubSubClient mqttClient;
    bool newMessage = false;
    String latestMessage = "";
#endif
};

#endif // ESPWEBCONNECT_H
