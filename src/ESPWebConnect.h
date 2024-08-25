#ifndef ESPWEBCONNECT_H
#define ESPWEBCONNECT_H
//#define ENABLE_MQTT
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <vector>
#include <functional>
#include <map>

#ifdef ENABLE_MQTT
#include <PubSubClient.h>
#endif

class ESPWebConnect {
public:
    ESPWebConnect();
    void begin();

    // Public settings structures
    struct WifiSettings {
        String SSID_Name;
        String SSID_Pass;
        String ESP_MAC;
        String SSID_AP_Name;
        String SSID_AP_Pass;
    } wifiSettings;

    const WifiSettings& getWifiSettings() const;
    void setIconUrl(const String& url);
    void setCSS(const String& url);
    void setAutoUpdate(unsigned long interval);
    void addSensor(const String& id, const String& name, const String& unit, const String& icon, std::function<float()> readFunction);
    void addSwitch(const String& id, const String& name, const String& icon, bool* state);
    void addButton(const String& id, const String& name, const String& icon, bool update, std::function<void()> onPress);
    void setIconColor(const String& id, const String& color);
    void setDashPath(const String& path);
    void setTitle(const String& title);
    void setDesc(const String& description);

    void updateDashboard();
    void sendNotification(const String& id, const String& message, const String& messageColor, const String& icon, const String& iconColor, int timeout);
    void handleButtonPress(AsyncWebServerRequest *request);
    
    #ifdef ENABLE_MQTT
    struct MQTTSettings {
        String MQTT_Broker;        
        int MQTT_Port;
        String MQTT_Send;
        String MQTT_Recv;
        String MQTT_User;
        String MQTT_Pass;
    } mqttSettings;

    const MQTTSettings& getMQTTSettings() const;
    void enableMQTT();
    void reconnectMQTT();
    void publishToMQTT(const String& payload);
    void checkMQTT();
    bool hasNewMQTTMsg() const;
    String getMQTTMsg();
    void saveMQTTSettings(const MQTTSettings& settings);
    #endif

    void saveWifiSettings(const WifiSettings& settings);
    double convDec(double value, int decimal_point = 2);

private:
    WiFiClient wifiClient;
    AsyncWebServer server;
    AsyncWebSocket ws;

    struct WebSettings {
        String Web_User;
        String Web_Pass;
        String Web_name;
        bool Web_Lock;
    };
    WebSettings webSettings;
    bool isAPMode() const;

    struct DashboardElement {
        enum Type { SENSOR, SWITCH, BUTTON } type;
        String id;
        String name;
        String unit;
        String icon;
        std::function<float()> readFunction;
        bool* state;
        std::function<void()> onPress;
        String color;

        DashboardElement(const String& id, const String& name, const String& unit, const String& icon, std::function<float()> readFunction, const String& color)
            : type(SENSOR), id(id), name(name), unit(unit), icon(icon), readFunction(readFunction), state(nullptr), onPress(nullptr), color(color) {}

        DashboardElement(const String& id, const String& name, const String& icon, bool* state, const String& color)
            : type(SWITCH), id(id), name(name), unit(""), icon(icon), readFunction(nullptr), state(state), onPress(nullptr), color(color) {}

        DashboardElement(const String& id, const String& name, const String& icon, const String& color, std::function<void()> onPress)
            : type(BUTTON), id(id), name(name), unit(""), icon(icon), readFunction(nullptr), state(nullptr), onPress(onPress), color(color) {}
    };

    std::vector<DashboardElement> dashboardElements;

    String iconUrl;
    String cssUrl;
    String title;
    String description;
    String dashPath;
    unsigned long updateInterval = 5000;

    // Updated to take AsyncWebServerRequest* parameter
    void handleToggleSwitch(AsyncWebServerRequest *request);
    void handleNotification(AsyncWebServerRequest *request);
    void handleSaveWifi(AsyncWebServerRequest *request);
    void handleSaveWeb(AsyncWebServerRequest *request);
    void handleGetWifiSettings(AsyncWebServerRequest *request);
    void handleGetWebSettings(AsyncWebServerRequest *request);
    
    void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

    bool checkAuth(AsyncWebServerRequest *request);
    bool readWifiSettings(WifiSettings& settings);
    bool readWebSettings(WebSettings& settings);
    
    void configureOTA();

    // Private methods
    String generateDashboardHTML();   // Generates the HTML for the dashboard
    String generateReadingsJSON();    // Generates the JSON data for the readings
    String generateSwitchesJSON();    // Generates the JSON data for the switches
    void handleReboot();              // Handles the reboot request
    void startAP(const char* ssid, const char* password); // Starts the AP mode
    void clearMemory();               // Clears memory related to dashboard elements

    bool configureWiFi(const char* ssid, const char* password); // Configures WiFi settings

    #ifdef ENABLE_MQTT
    void handleSaveMQTT(AsyncWebServerRequest *request);
    void handleGetMQTTSettings(AsyncWebServerRequest *request);
    bool readMQTTSettings(MQTTSettings& settings);
    void mqttCallback(char* topic, byte* payload, unsigned int length);
    PubSubClient mqttClient;
    bool newMessage = false;
    String latestMessage = "";
    #endif
};

#endif // ESPWEBCONNECT_H
