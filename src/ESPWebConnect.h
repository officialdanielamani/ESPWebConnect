#ifndef ESPWEBCONNECT_H
#define ESPWEBCONNECT_H

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <vector>
#include <functional>
#include <map>
#include <WebSocketsServer.h>

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

    void enableMQTT();
    void reconnectMQTT();
    void publishToMQTT(const String& payload);
    void checkMQTT();
    bool hasNewMQTTMsg() const;
    String getMQTTMsg();

    void saveWifiSettings(const WifiSettings& settings);
    void saveMQTTSettings(const MQTTSettings& settings);
    
    double convDec(double value, int decimal_point = 2);
private:
    WiFiClient wifiClient;
    WebServer server;
    WebSocketsServer webSocket;

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

    void handleRoot();
    void handleESPWebC();
    void handleCSS();
    void handleDashboard();
    void handleReadings();
    void handleSwitches();
    void handleToggleSwitch();
    void handleNotification();
    void handlePressButton();

    void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);

    bool readWifiSettings(WifiSettings& settings);
    bool readMQTTSettings(MQTTSettings& settings);
    bool readWebSettings(WebSettings& settings);
    void handleSaveWifi();
    void handleSaveMQTT();
    void handleSaveWeb();
    void handleGetWifiSettings();
    void handleGetMQTTSettings();
    void handleGetWebSettings();
    bool configureWiFi(const char* ssid, const char* password);
    void startAP(const char* ssid, const char* password);
    void handleOTAUpdate();
    void handleReboot();
    bool checkAuth();
    void saveSettings(const char* filename, const JsonDocument& doc);

    void mqttCallback(char* topic, byte* payload, unsigned int length);
    PubSubClient mqttClient;
    bool newMessage = false;
    String latestMessage = "";

    void clearMemory(); // Add the clearMemory function declaration
};

#endif // ESPWEBCONNECT_H
