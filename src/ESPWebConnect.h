#ifndef ESPWEBCONNECT_H
#define ESPWEBCONNECT_H
// #define ENABLE_MQTT
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
    void setAllCardSize(int width, int height);

    void addSensor(const char *id, const char *name, const char *unit, const char *icon, int *intValue);
    void addSensor(const char *id, const char *name, const char *unit, const char *icon, float *floatValue);
    void addSensor(const char* id, const char* name, const char* unit, const char* icon, String* stringValue);

    void addSwitch(const char *id, const char *name, const char *icon, bool *state);
    void addButton(const char *id, const char *name, const char *icon, std::function<void()> onPress);
    void addInputNum(const char *id, const char *name, const char *icon, int *variable);
    void addInputNum(const char *id, const char *name, const char *icon, float *variable);
    void addInputText(const char* id, const char* name, const char* icon, String* variable);

    void setCardSize(const char* id, float multiplierX, float multiplierY);
    void setIconColor(const char* id, const char* color);

    void setDashPath(const String &path);
    void setTitle(const String &title);
    void setDesc(const String &description);

    void updateDashboard();
    void sendNotification(const String &id, const String &message, const String &messageColor, const String &icon, const String &iconColor, int timeout);
    void handleButtonPress(AsyncWebServerRequest *request);

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

    void saveWifiSettings(const WifiSettings &settings);
    double convDec(double value, int decimal_point = 2);

    void performOTAUpdateFromURL(const String& firmwareURL);
    void handleUpdateError(int error);

    void setVersion(const String &sysversion);
    void setDeviceInfo(const String &sysinfo);

    struct LORASettings
    {
        String LORA_Key;           // Value between 0 and 0xFF
        bool LORA_CRC;              // Checkbox, true if checked
        int LORA_RSSI;              // Display only, can be an integer
        long LORA_PacketHZErr;       // Display only, can be an integer
        uint8_t LORA_Spread;        // Dropdown, values between 6 and 12
        uint8_t LORA_Coding;        // Dropdown, values between 5 and 8
        uint8_t LORA_TxPwr;         // Dropdown, values between 2 and 17
        String LORA_Reg;            // Dropdown, frequency bands like "433E6", "868E6", "915E6"
    } loraSettings;
    const LORASettings &getLORASettings() const;

    void saveLORASettings(const LORASettings &settings);

private:
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

    struct DashboardElement {
        enum Type { SENSOR_INT, SENSOR_FLOAT, SENSOR_STRING, SWITCH, BUTTON, INPUT_NUM, INPUT_TEXT } type;
        const char* id;
        const char* name;
        const char* unit;
        const char* icon;
        const char* color;
        union {
            int* intValue;
            float* floatValue;
            String* stringValue;  // Changed back to String* for mutable string handling
            bool* state;
        };
        std::function<void()> onPress;
        float sizeMultiplierX; // Multiplier for width
        float sizeMultiplierY; // Multiplier for height

        // Constructor for int sensor
        DashboardElement(const char* id, const char* name, const char* unit, const char* icon, int* intValue)
            : type(SENSOR_INT), id(id), name(name), unit(unit), icon(icon), color(nullptr), intValue(intValue), sizeMultiplierX(1.0), sizeMultiplierY(1.0) {}

        // Constructor for float sensor
        DashboardElement(const char* id, const char* name, const char* unit, const char* icon, float* floatValue)
            : type(SENSOR_FLOAT), id(id), name(name), unit(unit), icon(icon), color(nullptr), floatValue(floatValue), sizeMultiplierX(1.0), sizeMultiplierY(1.0) {}

        // Constructor for string sensor
        DashboardElement(const char* id, const char* name, const char* unit, const char* icon, String* stringValue)
            : type(SENSOR_STRING), id(id), name(name), unit(unit), icon(icon), color(nullptr), stringValue(stringValue), sizeMultiplierX(1.0), sizeMultiplierY(1.0) {}

        // Constructor for switches
        DashboardElement(const char* id, const char* name, const char* icon, bool* state)
            : type(SWITCH), id(id), name(name), unit(""), icon(icon), color(nullptr), state(state), sizeMultiplierX(1.0), sizeMultiplierY(1.0) {}

        // Constructor for buttons
        DashboardElement(const char* id, const char* name, const char* icon, std::function<void()> onPress)
            : type(BUTTON), id(id), name(name), unit(""), icon(icon), color(nullptr), onPress(onPress), sizeMultiplierX(1.0), sizeMultiplierY(1.0) {}

        // Constructor for integer input
        DashboardElement(const char* id, const char* name, const char* icon, int* intValue)
            : type(INPUT_NUM), id(id), name(name), unit(""), icon(icon), color(nullptr), intValue(intValue), sizeMultiplierX(1.0), sizeMultiplierY(1.0) {}

        // Constructor for float input
        DashboardElement(const char* id, const char* name, const char* icon, float* floatValue)
            : type(INPUT_NUM), id(id), name(name), unit(""), icon(icon), color(nullptr), floatValue(floatValue), sizeMultiplierX(1.0), sizeMultiplierY(1.0) {}

        // Constructor for text input
        DashboardElement(const char* id, const char* name, const char* icon, String* stringValue)
            : type(INPUT_TEXT), id(id), name(name), unit(""), icon(icon), color(nullptr), stringValue(stringValue), sizeMultiplierX(1.0), sizeMultiplierY(1.0) {}
    };
    std::vector<DashboardElement> dashboardElements;

    int baseWidth = 200;  // Default base width in pixels
    int baseHeight = 200; // Default base height in pixels

    String iconUrl;
    String cssUrl;
    String title;
    String description;
    String dashPath;
    unsigned long updateInterval = 5000;

    String sysversion;
    String sysinfo;

    void handleToggleSwitch(AsyncWebServerRequest *request);
    void handleNotification(AsyncWebServerRequest *request);
    //void handleGetWifiSettings(AsyncWebServerRequest *request);
    //void handleGetLORASettings(AsyncWebServerRequest *request);
    //void handleGetWebSettings(AsyncWebServerRequest *request);
    void handleFirmwareUpload(AsyncWebServerRequest *request);

    void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

    bool checkAuth(AsyncWebServerRequest *request);
    bool readWifiSettings(WifiSettings &settings);
    bool readWebSettings(WebSettings &settings);
    void sendOTAUpdateProgress(float progress);

    String generateDashboardHTML();
    String generateDashboardItem(const DashboardElement &element);
    String generateReadingsJSON();
    String generateSwitchesJSON();
    void handleReboot();
    void startAP(const char *ssid, const char *password);
    void clearMemory();
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
