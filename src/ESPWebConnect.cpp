#include "ESPWebConnect.h"

ESPWebConnect::ESPWebConnect() : server(80), webSocket(81), mqttClient(wifiClient), title("Dashboard Interface"), description("Example interface using the ESPWebConnect library"), dashPath("/dashboard") {}

void ESPWebConnect::setDashPath(const String& path) {
    this->dashPath = path;
}

void ESPWebConnect::setTitle(const String& title) {
    this->title = title;
}

void ESPWebConnect::setDesc(const String& description) {
    this->description = description;
}

void ESPWebConnect::setIconUrl(const String& url) {
    iconUrl = url;
}

void ESPWebConnect::setCSS(const String& url) {
    cssUrl = url;
}

double ESPWebConnect::convDec(double value, int decimal_point) {
    if (decimal_point < 1 || decimal_point > 5) {
        decimal_point = 2;
    }
    double factor = 1;
    for (int i = 0; i < decimal_point; ++i) {
        factor *= 10;
    }
    return (int)(value * factor + 0.5) / factor;
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

    if (!readMQTTSettings(mqttSettings)) {
        Serial.println("Failed to load MQTT settings");
    }

    if (webSettings.Web_name.length() > 0 && MDNS.begin(webSettings.Web_name.c_str())) {
        Serial.print("mDNS responder started: ");
        Serial.print(webSettings.Web_name);
        Serial.println(".local");
    } else {
        Serial.println("mDNS responder not started due to invalid or blank web name.");
    }

    server.on("/", HTTP_GET, std::bind(&ESPWebConnect::handleRoot, this));
    server.on("/espwebc", HTTP_GET, std::bind(&ESPWebConnect::handleESPWebC, this));
    server.on("/style.css", HTTP_GET, std::bind(&ESPWebConnect::handleCSS, this));
    server.on(dashPath.c_str(), HTTP_GET, std::bind(&ESPWebConnect::handleDashboard, this));
    server.on("/readings", HTTP_GET, std::bind(&ESPWebConnect::handleReadings, this));
    server.on("/switches", HTTP_GET, std::bind(&ESPWebConnect::handleSwitches, this));
    server.on("/toggleSwitch", HTTP_GET, std::bind(&ESPWebConnect::handleToggleSwitch, this));
    server.on("/notify", HTTP_GET, std::bind(&ESPWebConnect::handleNotification, this));

    server.on("/saveWifi", HTTP_POST, [this]() {
        if (!checkAuth()) {
            return;
        }
        handleSaveWifi();
    });

    server.on("/update-firmware", HTTP_POST, [this]() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
    }, std::bind(&ESPWebConnect::handleOTAUpdate, this));

    server.on("/espwebc-reboot", HTTP_GET, std::bind(&ESPWebConnect::handleReboot, this));

    server.on("/getWifiSettings", HTTP_GET, [this]() {
        if (!checkAuth()) {
            return;
        }
        handleGetWifiSettings();
    });

    server.on("/saveMQTT", HTTP_POST, [this]() {
        if (!checkAuth()) {
            return;
        }
        handleSaveMQTT();
    });

    server.on("/getMQTTSettings", HTTP_GET, [this]() {
        if (!checkAuth()) {
            return;
        }
        handleGetMQTTSettings();
    });

    server.on("/saveWeb", HTTP_POST, [this]() {
        if (!checkAuth()) {
            return;
        }
        handleSaveWeb();
    });

    server.on("/getWebSettings", HTTP_GET, [this]() {
        if (!checkAuth()) {
            return;
        }
        handleGetWebSettings();
    });

    server.begin();
    webSocket.begin();
    webSocket.onEvent(std::bind(&ESPWebConnect::onWebSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

    mqttClient.setServer(mqttSettings.MQTT_Broker.c_str(), mqttSettings.MQTT_Port);
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        mqttCallback(topic, payload, length);
    });

    Serial.println("Attempting MQTT connection...");
    if (mqttClient.connect("ESPWebConnectClient", mqttSettings.MQTT_User.c_str(), mqttSettings.MQTT_Pass.c_str())) {
        Serial.println("MQTT connected");
        mqttClient.subscribe(mqttSettings.MQTT_Recv.c_str()); // Ensure subscription is done after connection
    } else {
        Serial.print("MQTT connection failed, rc=");
        Serial.print(mqttClient.state());
    }
}

void ESPWebConnect::handleClient() {
    server.handleClient();
    webSocket.loop();
}

void ESPWebConnect::onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_TEXT) {
        String message = String((char*)payload);
        // Process WebSocket message if needed
    }
}

void ESPWebConnect::handleRoot() {
    File file = LittleFS.open("/index.html", "r");
    if (!file) {
        server.send(500, "text/plain", "Failed to open HTML file");
        return;
    }
    server.streamFile(file, "text/html");
    file.close();
}

void ESPWebConnect::handleESPWebC() {
    File file = LittleFS.open("/espwebc.html", "r");
    if (!file) {
        server.send(500, "text/plain", "Failed to open HTML file");
        return;
    }
    server.streamFile(file, "text/html");
    file.close();
}

void ESPWebConnect::handleCSS() {
    File file = LittleFS.open("/style.css", "r");
    if (!file) {
        server.send(404, "text/plain", "404: Not Found");
        return;
    }
    server.streamFile(file, "text/css");
    file.close();
}

void ESPWebConnect::setAutoUpdate(unsigned long interval) {
    if (interval < 1000) {
        updateInterval = 1000;
    } else {
        updateInterval = interval;
    }
}

void ESPWebConnect::updateDashboard() {
    handleDashboard();
}

void ESPWebConnect::addSensor(const String& id, const String& name, const String& unit, const String& icon, std::function<float()> readFunction) {
    dashboardElements.push_back(DashboardElement(id, name, unit, icon, readFunction, "#e0e0e0"));
}

void ESPWebConnect::addSwitch(const String& id, const String& name, const String& icon, bool* state) {
    dashboardElements.push_back(DashboardElement(id, name, icon, state, "#e0e0e0"));
}

void ESPWebConnect::addButton(const String& id, const String& name, const String& icon, bool update, std::function<void()> onPress) {
    dashboardElements.push_back(DashboardElement(id, name, icon, "#e0e0e0", onPress));
    if (update) {
        webSocket.broadcastTXT("{\"type\":\"button\",\"id\":\"" + id + "\",\"name\":\"" + name + "\"}");
    }
}

void ESPWebConnect::setIconColor(const String& id, const String& color) {
    for (auto &element : dashboardElements) {
        if (element.id == id) {
            element.color = color;
            break;
        }
    }
}

void ESPWebConnect::handleDashboard() {
    String html = "<html><head><title>" + title + "</title>";
    html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
    if (!iconUrl.isEmpty()) {
        html += "<link rel='stylesheet' href='" + iconUrl + "'>";
    }else{
      "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.2/css/all.min.css'>";  
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
    html += "<script>var webSocket = new WebSocket('ws://' + window.location.hostname + ':81/');";
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

    // Send the HTML to the client
    server.send(200, "text/html", html);

    // Clear the HTML String to free up memory
    html.clear();
}


void ESPWebConnect::handleReadings() {
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
    server.send(200, "application/json", json);
}

void ESPWebConnect::handleSwitches() {
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
    server.send(200, "application/json", json);
}

void ESPWebConnect::handleToggleSwitch() {
    String id = server.arg("id");
    String state = server.arg("state");
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
    server.send(200, "text/plain", "OK");
    updateDashboard();
}

void ESPWebConnect::sendNotification(const String& id, const String& message, const String& messageColor, const String& icon, const String& iconColor, int timeout) {
    StaticJsonDocument<256> doc;
    doc["id"] = id;
    doc["message"] = message;
    doc["messageColor"] = messageColor;
    doc["icon"] = icon;
    doc["iconColor"] = iconColor;
    doc["timeout"] = timeout;

    String json;
    serializeJson(doc, json);
    webSocket.broadcastTXT(json);
}

void ESPWebConnect::handleNotification() {
    String id = server.arg("id");
    String message = server.arg("message");
    String messageColor = server.arg("messageColor");
    String icon = server.arg("icon");
    String iconColor = server.arg("iconColor");
    int timeout = server.arg("timeout").toInt();

    sendNotification(id, message, messageColor, icon, iconColor, timeout);
    server.send(200, "text/plain", "Notification sent");
}

bool ESPWebConnect::readWifiSettings(WifiSettings& settings) {
    File file = LittleFS.open("/settings-wifi.json", "r");
    if (!file) {
        Serial.println("WiFi settings file not found");
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

    Serial.print("Loaded SSID: ");
    Serial.println(settings.SSID_Name);

    return true;
}

bool ESPWebConnect::readMQTTSettings(MQTTSettings& settings) {
    File file = LittleFS.open("/settings-mqtt.json", "r");
    if (!file) {
        Serial.println("MQTT settings file not found");
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
    settings.MQTT_Port = doc["MQTT_Port"];
    settings.MQTT_Send = doc["MQTT_Send"].as<String>();
    settings.MQTT_Recv = doc["MQTT_Recv"].as<String>();
    settings.MQTT_User = doc["MQTT_User"].as<String>();
    settings.MQTT_Pass = doc["MQTT_Pass"].as<String>();

    Serial.print("Loaded MQTT Broker: ");
    Serial.println(settings.MQTT_Broker);

    return true;
}

bool ESPWebConnect::readWebSettings(WebSettings& settings) {
    File file = LittleFS.open("/settings-web.json", "r");
    if (!file) {
        Serial.println("Web settings file not found");
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

    Serial.print("Loaded Web User: ");
    Serial.println(settings.Web_User);

    return true;
}

void ESPWebConnect::saveWifiSettings(const WifiSettings& settings) {
    StaticJsonDocument<512> doc;
    doc["SSID_Name"] = settings.SSID_Name;
    doc["SSID_Pass"] = settings.SSID_Pass;
    doc["ESP_MAC"] = settings.ESP_MAC;
    doc["SSID_AP_Name"] = settings.SSID_AP_Name;
    doc["SSID_AP_Pass"] = settings.SSID_AP_Pass;

    saveSettings("/settings-wifi.json", doc);
}

void ESPWebConnect::saveMQTTSettings(const MQTTSettings& settings) {
    StaticJsonDocument<512> doc;
    doc["MQTT_Broker"] = settings.MQTT_Broker;
    doc["MQTT_Port"] = settings.MQTT_Port;
    doc["MQTT_Send"] = settings.MQTT_Send;
    doc["MQTT_Recv"] = settings.MQTT_Recv;
    doc["MQTT_User"] = settings.MQTT_User;
    doc["MQTT_Pass"] = settings.MQTT_Pass;

    saveSettings("/settings-mqtt.json", doc);
}

void ESPWebConnect::saveSettings(const char* filename, const JsonDocument& doc) {
    File file = LittleFS.open(filename, "w");
    if (!file) {
        Serial.print("Failed to open ");
        Serial.print(filename);
        Serial.println(" for writing");
        return;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.print("Failed to write to ");
        Serial.println(filename);
    } else {
        Serial.print("Settings saved successfully to ");
        Serial.println(filename);
    }

    file.close();
}

void ESPWebConnect::handleSaveWifi() {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Bad request");
        return;
    }

    String body = server.arg("plain");
    File file = LittleFS.open("/settings-wifi.json", "w");
    if (!file) {
        Serial.println("Failed to open WiFi settings file for writing");
        server.send(500, "text/plain", "Failed to open file for writing.");
        return;
    }

    file.print(body);
    file.close();
    server.send(200, "text/plain", "WiFi settings saved successfully");
}

void ESPWebConnect::handleSaveMQTT() {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Bad request");
        return;
    }

    String body = server.arg("plain");
    File file = LittleFS.open("/settings-mqtt.json", "w");
    if (!file) {
        Serial.println("Failed to open MQTT settings file for writing");
        server.send(500, "text/plain", "Failed to open file for writing.");
        return;
    }

    file.print(body);
    file.close();
    server.send(200, "text/plain", "MQTT settings saved successfully");
}

void ESPWebConnect::handleSaveWeb() {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Bad request");
        return;
    }

    String body = server.arg("plain");
    File file = LittleFS.open("/settings-web.json", "w");
    if (!file) {
        Serial.println("Failed to open web settings file for writing");
        server.send(500, "text/plain", "Failed to open file for writing.");
        return;
    }

    file.print(body);
    file.close();
    server.send(200, "text/plain", "Web settings saved successfully");

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

void ESPWebConnect::handleGetWifiSettings() {
    File file = LittleFS.open("/settings-wifi.json", "r");
    if (!file) {
        server.send(500, "application/json", "{\"error\":\"Failed to open WiFi settings file.\"}");
        return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        server.send(500, "application/json", "{\"error\":\"Failed to parse WiFi settings.\"}");
        return;
    }

    String settings;
    serializeJson(doc, settings);
    server.send(200, "application/json", settings);
}

void ESPWebConnect::handleGetMQTTSettings() {
    File file = LittleFS.open("/settings-mqtt.json", "r");
    if (!file) {
        server.send(500, "application/json", "{\"error\":\"Failed to open MQTT settings file.\"}");
        return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        server.send(500, "application/json", "{\"error\":\"Failed to parse MQTT settings.\"}");
        return;
    }

    String settings;
    serializeJson(doc, settings);
    server.send(200, "application/json", settings);
}

void ESPWebConnect::handleGetWebSettings() {
    File file = LittleFS.open("/settings-web.json", "r");
    if (!file) {
        server.send(500, "application/json", "{\"error\":\"Failed to open web settings file.\"}");
        return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        server.send(500, "application/json", "{\"error\":\"Failed to parse web settings.\"}");
        return;
    }

    String settings;
    serializeJson(doc, settings);
    server.send(200, "application/json", settings);
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

    // Check if the SSID and password are empty and use default values if they are
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


void ESPWebConnect::clearMemory() {
    // Example: Free dynamically allocated memory
    for (auto &element : dashboardElements) {
        if (element.type == DashboardElement::BUTTON && element.onPress) {
            element.onPress = nullptr;
        }
    }
    dashboardElements.clear();
    Serial.println("Memory cleared.");
}

void ESPWebConnect::handleOTAUpdate() {
    clearMemory(); // Clear memory before OTA update

    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { // Start with max available size
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        Serial.printf("Writing file: %u bytes\n", upload.currentSize);
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { // true to set the size to the current progress
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            Serial.flush();
            ESP.restart();
        } else {
            Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Update.end();
        Serial.println("Update was aborted");
    }
}

void ESPWebConnect::handleReboot() {
    server.send(200, "text/plain", "Rebooting...");
    delay(1000);
    ESP.restart();
}

bool ESPWebConnect::checkAuth() {
    if (webSettings.Web_Lock) {
        if (!server.authenticate(webSettings.Web_User.c_str(), webSettings.Web_Pass.c_str())) {
            server.requestAuthentication();
            return false;
        }
    }
    return true;
}

const ESPWebConnect::WifiSettings& ESPWebConnect::getWifiSettings() const {
    return wifiSettings;
}

const ESPWebConnect::MQTTSettings& ESPWebConnect::getMQTTSettings() const {
    return mqttSettings;
}

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

    while (!mqttClient.connected()) {
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
        }
    }
}

void ESPWebConnect::publishToMQTT(const String& payload) {
    if (isAPMode()) return;
    if (!mqttClient.connected()) {
        reconnectMQTT();
    }
    mqttClient.publish(mqttSettings.MQTT_Send.c_str(), payload.c_str());
}

void ESPWebConnect::checkMQTT() {
    if (isAPMode()) return;
    if (!mqttClient.connected()) {
        reconnectMQTT();
    }
    mqttClient.loop();
}

bool ESPWebConnect::hasNewMQTTMsg() const {
    //if (isAPMode()) return;
    return newMessage;
}

String ESPWebConnect::getMQTTMsg() {
    //if (isAPMode()) return;
    newMessage = false; // Reset the flag after reading the message
    return latestMessage;
}

bool ESPWebConnect::isAPMode() const {
    return WiFi.getMode() & WIFI_AP;
}