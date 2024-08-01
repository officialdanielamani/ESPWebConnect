#include "ESPWebConnect.h"

ESPWebConnect::ESPWebConnect() : server(80) {}

void ESPWebConnect::begin() {
    Serial.begin(115200);
    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed");
        return;
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
    server.on("/dashboard", HTTP_GET, std::bind(&ESPWebConnect::handleDashboard, this));
    server.on("/readings", HTTP_GET, std::bind(&ESPWebConnect::handleReadings, this));
    server.on("/switches", HTTP_GET, std::bind(&ESPWebConnect::handleSwitches, this));
    server.on("/toggleSwitch", HTTP_GET, std::bind(&ESPWebConnect::handleToggleSwitch, this));

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
}

void ESPWebConnect::handleClient() {
    server.handleClient();
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

void ESPWebConnect::setIconUrl(String url) {
    iconUrl = url;
}

void ESPWebConnect::setCSS(String url) {
    cssUrl = url;
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

void ESPWebConnect::addSensor(String name, String unit, String icon, std::function<float()> readFunction, String color) {
    sensors.push_back({name, unit, icon, readFunction, color});
}

void ESPWebConnect::addSwitch(String name, String icon, bool* state, String color) {
    switches.push_back({name, icon, state, color});
}

void ESPWebConnect::handleDashboard() {
    String html = "<html><head><title>ESPWebConnect Dashboard</title>";
    html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
    if (!iconUrl.isEmpty()) {
        html += "<link rel='stylesheet' href='" + iconUrl + "'>";
    }
    html += "<link rel='stylesheet' href='/style.css'>";
    html += "<script>function updateReadings() {";
    html += "fetch('/readings').then(response => response.json()).then(data => {";
    for (auto &sensor : sensors) {
        String sensorId = sensor.name;
        sensorId.toLowerCase();
        html += "document.getElementById('" + sensorId + "').innerText = data." + sensorId + ";";
    }
    html += "});";
    html += "setTimeout(updateReadings, " + String(updateInterval) + ");";
    html += "}</script>";
    html += "<script>function updateSwitches() {";
    html += "fetch('/switches').then(response => response.json()).then(data => {";
    for (auto &sw : switches) {
        String switchId = sw.name;
        switchId.toLowerCase();
        html += "document.getElementById('" + switchId + "').checked = data." + switchId + ";";
    }
    html += "});";
    html += "setTimeout(updateSwitches, " + String(updateInterval) + ");";
    html += "}</script></head><body onload='updateReadings(); updateSwitches();'>";
    html += "<br><h1>Dashboard Interface</h1>";
    html += "<br><p>Example interface of Smart Home using the ESPWebConnect library</p>";
    html += "<div class='dashboard'>"; // Start of dashboard div
    for (auto &sensor : sensors) {
        String sensorId = sensor.name;
        sensorId.toLowerCase();
        html += "<div class='card'>";
        html += "<div class='icon'><i class='" + sensor.icon + "' style='color: " + sensor.color + ";'></i></div>";
        html += "<div class='sensor-name'>" + sensor.name + "</div>";
        html += "<div class='sensor-value'><span id='" + sensorId + "'>Loading...</span> <span class='sensor-unit'>" + sensor.unit + "</span></div>";
        html += "</div>";
    }
    for (auto &sw : switches) {
        String switchId = sw.name;
        switchId.toLowerCase();
        html += "<div class='card'>";
        html += "<div class='icon'><i class='" + sw.icon + "' style='color: " + sw.color + ";'></i></div>";
        html += "<div class='sensor-name'>" + sw.name + "</div>";
        html += "<label class='switch'><input type='checkbox' id='" + switchId + "' onclick='toggleSwitch(\"" + switchId + "\")'><span class='slider_sw'></span></label>";
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
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void ESPWebConnect::handleReadings() {
    String json = "{";
    for (auto &sensor : sensors) {
        String sensorId = sensor.name;
        sensorId.toLowerCase();
        json += "\"" + sensorId + "\": " + String(sensor.readFunction()) + ",";
    }
    json.remove(json.length() - 1); // Remove trailing comma
    json += "}";
    server.send(200, "application/json", json);
}

void ESPWebConnect::handleSwitches() {
    String json = "{";
    for (auto &sw : switches) {
        String switchId = sw.name;
        switchId.toLowerCase();
        json += "\"" + switchId + "\": " + String(*sw.state) + ",";
    }
    json.remove(json.length() - 1); // Remove trailing comma
    json += "}";
    server.send(200, "application/json", json);
}

void ESPWebConnect::handleToggleSwitch() {
    String id = server.arg("id");
    String state = server.arg("state");
    for (auto &sw : switches) {
        String switchId = sw.name;
        switchId.toLowerCase();
        if (switchId == id) {
            *sw.state = (state == "true");
            break;
        }
    }
    server.send(200, "text/plain", "OK");
    updateDashboard();
}

bool ESPWebConnect::readWifiSettings(WifiSettings &settings) {
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

bool ESPWebConnect::readMQTTSettings(MQTTSettings &settings) {
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

bool ESPWebConnect::readWebSettings(WebSettings &settings) {
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

void ESPWebConnect::saveWifiSettings(const WifiSettings &settings) {
    StaticJsonDocument<512> doc;
    doc["SSID_Name"] = settings.SSID_Name;
    doc["SSID_Pass"] = settings.SSID_Pass;
    doc["ESP_MAC"] = settings.ESP_MAC;
    doc["SSID_AP_Name"] = settings.SSID_AP_Name;
    doc["SSID_AP_Pass"] = settings.SSID_AP_Pass;

    saveSettings("/settings-wifi.json", doc);
}

void ESPWebConnect::saveMQTTSettings(const MQTTSettings &settings) {
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

bool ESPWebConnect::configureWiFi(const char *ssid, const char *password) {
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

void ESPWebConnect::startAP(const char *ssid, const char *password) {
    Serial.println("Starting AP mode...");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);

    if (ssid == "") {
        WiFi.softAP("ESPWebConnect", "D@NP8888");
        Serial.print("AP Mode, connect to SSID: ESPWebConnect");
    } else {
        WiFi.softAP(ssid, password);
        Serial.print("AP Mode, connect to SSID: ");
        Serial.print(ssid);
    }
    Serial.print(" with IP: ");
    Serial.println(WiFi.softAPIP());

    wifiSettings.ESP_MAC = WiFi.softAPmacAddress();
    Serial.print("MAC Address: ");
    Serial.println(wifiSettings.ESP_MAC);

    saveWifiSettings(wifiSettings);
}

void ESPWebConnect::handleOTAUpdate() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
            Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
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
