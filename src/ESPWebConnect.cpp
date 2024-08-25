#include "ESPWebConnect.h"

ESPWebConnect::ESPWebConnect()
    : server(80), ws("/ws"),
      title("Dashboard Interface"),
      description("Example interface using the ESPWebConnect library"),
      dashPath("/dashboard") 
{
    #ifdef ENABLE_MQTT
    mqttClient.setClient(wifiClient);
    #endif

    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
        this->onWebSocketEvent(server, client, type, arg, data, len);
    });
    server.addHandler(&ws);
}

void ESPWebConnect::begin() {
    Serial.begin(115200);
    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed, attempting to format...");
        LittleFS.format();
        if (!LittleFS.begin()) {
            Serial.println("LittleFS mount failed after formatting");
            return;
        }
    }

    if (!readWebSettings(webSettings)) {
        webSettings.Web_Lock = false;
        webSettings.Web_User = "admin";
        webSettings.Web_Pass = "admin";
        webSettings.Web_name = "ESPWebConnect";
    }

    bool wifiConnected = false;
    if (readWifiSettings(wifiSettings)) {
        wifiConnected = configureWiFi(wifiSettings.SSID_Name.c_str(), wifiSettings.SSID_Pass.c_str());
    }
    if (!wifiConnected) {
        startAP(wifiSettings.SSID_AP_Name.c_str(), wifiSettings.SSID_AP_Pass.c_str());
    }

    #ifdef ENABLE_MQTT
    if (!readMQTTSettings(mqttSettings)) {
        Serial.println("Failed to load MQTT settings");
    }
    #endif

    if (webSettings.Web_name.length() > 0 && MDNS.begin(webSettings.Web_name.c_str())) {
        Serial.print("mDNS responder started: ");
        Serial.print(webSettings.Web_name);
        Serial.println(".local");
    } else {
        Serial.println("mDNS responder not started due to invalid or blank web name.");
    }

    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/html", generateDashboardHTML());
    });

    server.on("/espwebc", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/espwebc.html", "text/html");
    });

    server.on("/style.css", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/style.css", "text/css");
    });

    server.on(dashPath.c_str(), HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/html", generateDashboardHTML());
    });

    server.on("/readings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "application/json", generateReadingsJSON());
    });

    server.on("/switches", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "application/json", generateSwitchesJSON());
    });

    server.on("/toggleSwitch", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleToggleSwitch(request);
        request->send(200, "text/plain", "OK");
    });

    server.on("/pressButton", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleButtonPress(request);
    });

    server.on("/notify", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleNotification(request);
        request->send(200, "text/plain", "Notification sent");
    });

    server.on("/saveWifi", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        handleSaveWifi(request);
    });

    server.on("/espwebc-reboot", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Rebooting...");
        handleReboot();
    });

    server.on("/getWifiSettings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        request->send(LittleFS, "/settings-wifi.json", "application/json");
    });

    #ifdef ENABLE_MQTT
    server.on("/saveMQTT", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        handleSaveMQTT(request);
    });

    server.on("/getMQTTSettings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        request->send(LittleFS, "/settings-mqtt.json", "application/json");
    });
    #endif

    server.on("/saveWeb", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        handleSaveWeb(request);
    });

    server.on("/getWebSettings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        request->send(LittleFS, "/settings-web.json", "application/json");
    });

    server.begin();
    configureOTA();

    #ifdef ENABLE_MQTT
    mqttClient.setServer(mqttSettings.MQTT_Broker.c_str(), mqttSettings.MQTT_Port);
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        mqttCallback(topic, payload, length);
    });

    Serial.println("Attempting MQTT connection...");
    if (mqttClient.connect("ESPWebConnectClient", mqttSettings.MQTT_User.c_str(), mqttSettings.MQTT_Pass.c_str())) {
        Serial.println("MQTT connected");
        mqttClient.subscribe(mqttSettings.MQTT_Recv.c_str());
    } else {
        Serial.print("MQTT connection failed, rc=");
        Serial.print(mqttClient.state());
    }
    #endif
}

bool ESPWebConnect::configureWiFi(const char* ssid, const char* password) {
    Serial.print("Connecting to WiFi network: ");
    Serial.println(ssid);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    delay(1000);

    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 25) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi successfully.");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("WiFi Channel: ");
        Serial.println(WiFi.channel());

        wifiSettings.ESP_MAC = WiFi.macAddress();
        Serial.print("MAC Address: ");
        Serial.println(wifiSettings.ESP_MAC);

        saveWifiSettings(wifiSettings);

        return true;
    } else {
        Serial.println("\nFailed to connect to WiFi.");
        return false;
    }
}
void ESPWebConnect::startAP(const char* ssid, const char* password) {
    Serial.println("Starting AP mode...");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);

    if (ssid == nullptr || strlen(ssid) == 0) {
        ssid = "ESPWebConnect";
    }
    if (password == nullptr || strlen(password) == 0) {
        password = "D@NP8888";
    }

    WiFi.softAP(ssid, password);
    Serial.print("AP Mode, connect to SSID: ");
    Serial.print(ssid);
    Serial.print(" with IP: ");
    Serial.println(WiFi.softAPIP());

    wifiSettings.ESP_MAC = WiFi.softAPmacAddress();
    Serial.print("MAC Address: ");
    Serial.println(wifiSettings.ESP_MAC);

    saveWifiSettings(wifiSettings);
}

void ESPWebConnect::configureOTA() {
    ArduinoOTA.setHostname(webSettings.Web_name.c_str());
    ArduinoOTA.onStart([]() {
        String type = ArduinoOTA.getCommand() == U_FLASH ? "sketch" : "filesystem";
        Serial.println("Start updating " + type);
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
}

void ESPWebConnect::onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_DATA) {
        String message = String((char*)data);
        // Process WebSocket message if needed
    }
}

void ESPWebConnect::handleToggleSwitch(AsyncWebServerRequest *request) {
    String id = request->arg("id");
    String state = request->arg("state");
    for (auto &element : dashboardElements) {
        if (element.type == DashboardElement::SWITCH) {
            String switchId = element.id;
            switchId.toLowerCase();
            if (switchId == id) {
                *element.state = (state == "true");
                break;
            }
        }
    }
    generateDashboardHTML();
}

void ESPWebConnect::handleNotification(AsyncWebServerRequest *request) {
    String id = request->arg("id");
    String message = request->arg("message");
    String messageColor = request->arg("messageColor");
    String icon = request->arg("icon");
    String iconColor = request->arg("iconColor");
    int timeout = request->arg("timeout").toInt();

    sendNotification(id, message, messageColor, icon, iconColor, timeout);
}

void ESPWebConnect::sendNotification(const String& id, const String& message, const String& messageColor, const String& icon, const String& iconColor, int timeout) {
    String notificationJson = "{\"id\":\"" + id + "\",\"message\":\"" + message + "\",\"messageColor\":\"" + messageColor + "\",\"icon\":\"" + icon + "\",\"iconColor\":\"" + iconColor + "\",\"timeout\":" + String(timeout) + "}";
    ws.textAll(notificationJson);
}

void ESPWebConnect::setIconUrl(const String& url) {
    iconUrl = url;
}

void ESPWebConnect::setCSS(const String& url) {
    cssUrl = url;
}

void ESPWebConnect::setAutoUpdate(unsigned long interval) {
    updateInterval = interval;
}

void ESPWebConnect::addSensor(const String& id, const String& name, const String& unit, const String& icon, std::function<float()> readFunction) {
    dashboardElements.emplace_back(id, name, unit, icon, readFunction, "");
}

void ESPWebConnect::addSwitch(const String& id, const String& name, const String& icon, bool* state) {
    dashboardElements.emplace_back(id, name, icon, state, "");
}

void ESPWebConnect::addButton(const String& id, const String& name, const String& icon, bool update, std::function<void()> onPress) {
    dashboardElements.emplace_back(id, name, icon, "", onPress);
}

void ESPWebConnect::setIconColor(const String& id, const String& color) {
    for (auto &element : dashboardElements) {
        if (element.id == id) {
            element.color = color;
            break;
        }
    }
}

void ESPWebConnect::handleSaveWeb(AsyncWebServerRequest *request) {
    if (!request->hasArg("plain")) {
        request->send(400, "text/plain", "Bad request");
        return;
    }

    String body = request->arg("plain");
    File file = LittleFS.open("/settings-web.json", "w");
    if (!file) {
        Serial.println("Failed to open web settings file for writing");
        request->send(500, "text/plain", "Failed to open file for writing.");
        return;
    }

    file.print(body);
    file.close();
    request->send(200, "text/plain", "Web settings saved successfully");

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, body);
    if (!error) {
        String newWebName = doc["Web_name"].as<String>();
        if (newWebName.length() > 0 && MDNS.begin(newWebName.c_str())) {
            Serial.print("mDNS responder restarted: ");
            Serial.print(newWebName);
            Serial.println(".local");
        } else {
            Serial.println("mDNS responder not restarted due to invalid or blank web name.");
        }
    }
}


#ifdef ENABLE_MQTT

void ESPWebConnect::mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    latestMessage = "";
    for (unsigned int i = 0; i < length; i++) {
        latestMessage += (char)payload[i];
    }
    Serial.println(latestMessage);
    newMessage = true;
}

void ESPWebConnect::enableMQTT() {
    if (isAPMode()) return;
    mqttClient.setServer(mqttSettings.MQTT_Broker.c_str(), mqttSettings.MQTT_Port);
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->mqttCallback(topic, payload, length);
    });
    reconnectMQTT();
}

void ESPWebConnect::reconnectMQTT() {
    if (isAPMode()) return;

    int attemptCount = 0;
    const int maxAttempts = 3;

    while (!mqttClient.connected() && (attemptCount < maxAttempts)) {
        Serial.print("Attempting MQTT connection...");
        if (mqttClient.connect("ESP32Client", mqttSettings.MQTT_User.c_str(), mqttSettings.MQTT_Pass.c_str())) {
            Serial.println("connected");
            mqttClient.subscribe(mqttSettings.MQTT_Recv.c_str());
            Serial.print("Subscribed to topic: ");
            Serial.println(mqttSettings.MQTT_Recv.c_str());
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
            attemptCount++;
        }
    }

    if (attemptCount >= maxAttempts) {
        Serial.println("MQTT connection failed after 3 attempts.");
    }
}

void ESPWebConnect::publishToMQTT(const String& payload) {
    if (isAPMode()) return;
    reconnectMQTT();
    mqttClient.publish(mqttSettings.MQTT_Send.c_str(), payload.c_str());
}

void ESPWebConnect::checkMQTT() {
    if (isAPMode()) return;
    reconnectMQTT();
    mqttClient.loop();
}

bool ESPWebConnect::hasNewMQTTMsg() const {
    return newMessage;
}

String ESPWebConnect::getMQTTMsg() {
    newMessage = false; // Reset the flag after reading the message
    return latestMessage;
}

#endif

String ESPWebConnect::generateDashboardHTML() {
    String html = "<html><head><title>" + title + "</title>";
    html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
    if (!iconUrl.isEmpty()) {
        html += "<link rel='stylesheet' href='" + iconUrl + "'>";
    } else {
        html += "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.2/css/all.min.css'>";
    }
    if (!cssUrl.isEmpty()) {
        html += "<link rel='stylesheet' href='" + cssUrl + "'>";
    } else {
        html += "<link rel='stylesheet' href='/style.css'>";
    }
    html += "<script>function updateReadings() {";
    html += "fetch('/readings').then(response => response.json()).then(data => {";
    for (auto &element : dashboardElements) {
        if (element.type == DashboardElement::SENSOR) {
            String sensorId = element.id;
            sensorId.toLowerCase();
            html += "document.getElementById('" + sensorId + "').innerText = data." + sensorId + ";";
        }
    }
    html += "});";
    html += "setTimeout(updateReadings, " + String(updateInterval) + ");";
    html += "}</script>";
    html += "<script>function updateSwitches() {";
    html += "fetch('/switches').then(response => response.json()).then(data => {";
    for (auto &element : dashboardElements) {
        if (element.type == DashboardElement::SWITCH) {
            String switchId = element.id;
            switchId.toLowerCase();
            html += "document.getElementById('" + switchId + "').checked = data['" + switchId + "'];";
        }
    }
    html += "});";
    html += "setTimeout(updateSwitches, " + String(updateInterval) + ");";
    html += "}</script>";
    html += "<script>var webSocket = new WebSocket('ws://' + window.location.hostname + '/ws');";
    html += "webSocket.onmessage = function(event) {";
    html += "var data = JSON.parse(event.data);";
    html += "showNotification(data.id, data.message, data.messageColor, data.icon, data.iconColor, data.timeout);";
    html += "};</script>";
    html += "<script>function showNotification(id, message, messageColor, icon, iconColor, timeout) {";
    html += "var banner = document.getElementById('notificationBanner');";
    html += "var messageElement = document.getElementById('notificationMessage');";
    html += "var iconElement = document.getElementById('notificationIcon');";
    html += "messageElement.textContent = message;";
    html += "if (messageColor) {";
    html += "messageElement.style.color = messageColor;";
    html += "} else {";
    html += "messageElement.style.color = '';";
    html += "}";
    html += "if (icon) {";
    html += "iconElement.className = icon;";
    html += "if (iconColor) {";
    html += "iconElement.style.color = iconColor;";
    html += "} else {";
    html += "iconElement.style.color = '';";
    html += "}";
    html += "iconElement.style.display = 'inline';";
    html += "} else {";
    html += "iconElement.className = '';";
    html += "iconElement.style.display = 'none';";
    html += "}";
    html += "banner.style.display = 'flex';";
    html += "if (timeout > 0) {";
    html += "setTimeout(closeNotification, timeout * 1000);";
    html += "}";
    html += "}</script>";
    html += "<script>function closeNotification() {";
    html += "var banner = document.getElementById('notificationBanner');";
    html += "banner.style.display = 'none';";
    html += "}</script>";
    html += "</head><body onload='updateReadings(); updateSwitches();'>";
    // Notification Banner
    html += "<div id='notificationBanner' class='notification-banner' style='display: none;'>";
    html += "<i id='notificationIcon' class='notification-icon' style='font-size: 32px;'></i>";
    html += "<span id='notificationMessage' class='notification-message'></span>";
    html += "<button id='closeButton' class='close-button' onclick='closeNotification()'>X</button>";
    html += "</div>";
    // Main Content
    html += "<br><h1>" + title + "</h1>";
    html += "<br><p>" + description + "</p>";
    html += "<div class='dashboard'>"; // Start of dashboard div
    for (auto &element : dashboardElements) {
        String elementId = element.id;
        elementId.toLowerCase();
        html += "<div class='card'>";
        html += "<div class='icon'><i class='" + element.icon + "' style='color: " + element.color + ";'></i></div>";
        html += "<div class='sensor-name'>" + element.name + "</div>";
        if (element.type == DashboardElement::SENSOR) {
            html += "<div class='sensor-value'><span id='" + elementId + "'>Loading...</span> <span class='sensor-unit'>" + element.unit + "</span></div>";
        } else if (element.type == DashboardElement::SWITCH) {
            html += "<label class='switch'><input type='checkbox' id='" + elementId + "' onclick='toggleSwitch(\"" + elementId + "\")'><span class='slider_sw'></span></label>";
        } else if (element.type == DashboardElement::BUTTON) {
            html += "<button id='" + elementId + "' class='button' onclick='pressButton(\"" + elementId + "\")'>" + element.name + "</button>";
        }
        html += "</div>";
    }
    html += "</div>"; // End of dashboard div
    html += "<footer><button onclick=\"window.location.href='/espwebc'\">Go to ESP Web Config</button>";
    html += "<a href=\"danielamani.com\" style=\"color: white;\">Code by: DanielAmani.com</a><br></footer>";
    html += "<script>function toggleSwitch(id) {";
    html += "var checkbox = document.getElementById(id);";
    html += "var xhr = new XMLHttpRequest();";
    html += "xhr.open('GET', '/toggleSwitch?id=' + id + '&state=' + checkbox.checked, true);";
    html += "xhr.send();";
    html += "}</script>";
    html += "<script>function pressButton(id) {";
    html += "var xhr = new XMLHttpRequest();";
    html += "xhr.open('GET', '/pressButton?id=' + id, true);";
    html += "xhr.send();";
    html += "}</script>";
    html += "</body></html>";
    return html;
}

bool ESPWebConnect::checkAuth(AsyncWebServerRequest *request) {
    if (webSettings.Web_Lock) {
        if (!request->authenticate(webSettings.Web_User.c_str(), webSettings.Web_Pass.c_str())) {
            request->requestAuthentication();
            return false;
        }
    }
    return true;
}

void ESPWebConnect::handleButtonPress(AsyncWebServerRequest *request) {
    if (!request->hasArg("id")) {
        Serial.println("Bad Request: Missing button ID");
        request->send(400, "text/plain", "Bad Request: Missing button ID");
        return;
    }

    String id = request->arg("id");
    Serial.print("Button pressed with ID: ");
    Serial.println(id);

    bool buttonFound = false;
    for (auto &element : dashboardElements) {
        if (element.id == id && element.type == DashboardElement::BUTTON) {
            if (element.onPress) {
                element.onPress(); // Call the button's press function
                Serial.println("Button press function called");
            }
            buttonFound = true;
            break;
        }
    }

    if (buttonFound) {
        request->send(200, "text/plain", "Button pressed");
    } else {
        Serial.println("Button not found");
        request->send(404, "text/plain", "Button not found");
    }
}


void ESPWebConnect::clearMemory() {
    for (auto &element : dashboardElements) {
        if (element.type == DashboardElement::BUTTON && element.onPress) {
            element.onPress = nullptr;
        }
    }
    dashboardElements.clear();
    //Serial.println("Memory cleared.");
}


void ESPWebConnect::handleReboot() {
    delay(1000);
    ESP.restart();
}

bool ESPWebConnect::isAPMode() const {
    return WiFi.getMode() & WIFI_AP;
}

String ESPWebConnect::generateSwitchesJSON() {
    String json = "{";
    for (auto &element : dashboardElements) {
        if (element.type == DashboardElement::SWITCH) {
            String switchId = element.id;
            switchId.toLowerCase();
            json += "\"" + switchId + "\": " + (*element.state ? "true" : "false") + ",";
        }
    }
    json.remove(json.length() - 1); // Remove trailing comma
    json += "}";
    return json;
}

String ESPWebConnect::generateReadingsJSON() {
    String json = "{";
    for (auto &element : dashboardElements) {
        if (element.type == DashboardElement::SENSOR) {
            String sensorId = element.id;
            sensorId.toLowerCase();
            float value = element.readFunction();
            json += "\"" + sensorId + "\": " + String(value) + ",";
        }
    }
    json.remove(json.length() - 1); // Remove trailing comma
    json += "}";
    return json;
}

void ESPWebConnect::handleSaveWifi(AsyncWebServerRequest *request) {
    if (!request->hasArg("plain")) {
        request->send(400, "text/plain", "Bad request");
        return;
    }

    String body = request->arg("plain");
    File file = LittleFS.open("/settings-wifi.json", "w");
    if (!file) {
        Serial.println("Failed to open WiFi settings file for writing");
        request->send(500, "text/plain", "Failed to open file for writing.");
        return;
    }

    file.print(body);
    file.close();
    request->send(200, "text/plain", "WiFi settings saved successfully");
}

bool ESPWebConnect::readWifiSettings(WifiSettings& settings) {
    File file = LittleFS.open("/settings-wifi.json", "r");
    if (!file) {
        Serial.println("Failed to open WiFi settings file for reading");
        return false;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("Failed to parse WiFi settings");
        return false;
    }

    settings.SSID_Name = doc["SSID_Name"].as<String>();
    settings.SSID_Pass = doc["SSID_Pass"].as<String>();
    settings.ESP_MAC = doc["ESP_MAC"].as<String>();
    settings.SSID_AP_Name = doc["SSID_AP_Name"].as<String>();
    settings.SSID_AP_Pass = doc["SSID_AP_Pass"].as<String>();

    return true;
}

bool ESPWebConnect::readWebSettings(WebSettings& settings) {
    File file = LittleFS.open("/settings-web.json", "r");
    if (!file) {
        Serial.println("Failed to open web settings file for reading");
        return false;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("Failed to parse web settings");
        return false;
    }

    settings.Web_User = doc["Web_User"].as<String>();
    settings.Web_Pass = doc["Web_Pass"].as<String>();
    settings.Web_name = doc["Web_name"].as<String>();
    settings.Web_Lock = doc["Web_Lock"].as<bool>();

    return true;
}

void ESPWebConnect::saveWifiSettings(const WifiSettings& settings) {
    StaticJsonDocument<512> doc;
    doc["SSID_Name"] = settings.SSID_Name;
    doc["SSID_Pass"] = settings.SSID_Pass;
    doc["ESP_MAC"] = settings.ESP_MAC;
    doc["SSID_AP_Name"] = settings.SSID_AP_Name;
    doc["SSID_AP_Pass"] = settings.SSID_AP_Pass;

    File file = LittleFS.open("/settings-wifi.json", "w");
    if (!file) {
        Serial.println("Failed to open WiFi settings file for writing");
        return;
    }

    serializeJson(doc, file);
    file.close();
}

#ifdef ENABLE_MQTT
void ESPWebConnect::saveMQTTSettings(const MQTTSettings& settings) {
    StaticJsonDocument<512> doc;
    doc["MQTT_Broker"] = settings.MQTT_Broker;
    doc["MQTT_Port"] = settings.MQTT_Port;
    doc["MQTT_Send"] = settings.MQTT_Send;
    doc["MQTT_Recv"] = settings.MQTT_Recv;
    doc["MQTT_User"] = settings.MQTT_User;
    doc["MQTT_Pass"] = settings.MQTT_Pass;

    File file = LittleFS.open("/settings-mqtt.json", "w");
    if (!file) {
        Serial.println("Failed to open MQTT settings file for writing");
        return;
    }

    serializeJson(doc, file);
    file.close();
}

bool ESPWebConnect::readMQTTSettings(MQTTSettings& settings) {
    File file = LittleFS.open("/settings-mqtt.json", "r");
    if (!file) {
        Serial.println("Failed to open MQTT settings file for reading");
        return false;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("Failed to parse MQTT settings");
        return false;
    }

    settings.MQTT_Broker = doc["MQTT_Broker"].as<String>();
    settings.MQTT_Port = doc["MQTT_Port"].as<int>();
    settings.MQTT_Send = doc["MQTT_Send"].as<String>();
    settings.MQTT_Recv = doc["MQTT_Recv"].as<String>();
    settings.MQTT_User = doc["MQTT_User"].as<String>();
    settings.MQTT_Pass = doc["MQTT_Pass"].as<String>();

    return true;
}
#endif

