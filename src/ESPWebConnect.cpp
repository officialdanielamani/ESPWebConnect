#include "ESPWebConnect.h"

ESPWebConnect::ESPWebConnect()
    : server(80), ws("/ws"),
      title("Dashboard Interface"),
      description("Example interface using the ESPWebConnect library"),
      dashPath("/dashboard"),
      sysversion("V1.0"),
      sysinfo("Home Automation")
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

    //server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    //    request->send(200, "text/html", generateDashboardHTML());
    //});

    server.on("/espwebc", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;

        // No longer handling OTA update via firmware URL here
        request->send(LittleFS, "/espwebc.html", "text/html");
    });

    server.on("/style.css", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/style.css", "text/css");
    });

    server.on(dashPath.c_str(), HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        request->send(200, "text/html", generateDashboardHTML());
    });


    server.on("/readings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        request->send(200, "application/json", generateReadingsJSON());
    });

    server.on("/switches", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        request->send(200, "application/json", generateSwitchesJSON());
    });

    server.on("/toggleSwitch", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        handleToggleSwitch(request);
        request->send(200, "text/plain", "OK");
    });

    server.on("/pressButton", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        handleButtonPress(request);
    });

    server.on("/notify", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        handleNotification(request);
        request->send(200, "text/plain", "Notification sent");
    });

    server.on("/saveWifi", HTTP_POST, [](AsyncWebServerRequest *request) {},
    NULL, // No file upload handler
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index == 0) {
            request->_tempObject = new String();  // Initialize a String to accumulate the data
        }
        String* body = (String*)request->_tempObject;
        body->concat((char*)data, len);  // Accumulate the incoming data

        if (index + len == total) {  // Last chunk of data
            Serial.println("Received WiFi settings: " + *body);

            // Parse the JSON from the accumulated String
            StaticJsonDocument<1024> doc;
            DeserializationError error = deserializeJson(doc, *body);
            if (error) {
                Serial.println("Failed to parse JSON!");
                request->send(400, "text/plain", "Failed to parse JSON");
                delete body;  // Clean up the String object
                return;
            }

            // Save the JSON to a file
            File file = LittleFS.open("/settings-wifi.json", "w");
            if (!file) {
                Serial.println("Failed to open WiFi settings file for writing");
                request->send(500, "text/plain", "Failed to open file for writing.");
                delete body;  // Clean up the String object
                return;
            }

            // Serialize JSON directly to file
            serializeJson(doc, file);
            file.close();
            Serial.println("WiFi settings saved successfully");

            // Respond to the client
            request->send(200, "text/plain", "WiFi settings saved successfully");
            delete body;  // Clean up the String object
        }
    }
);

server.on("/saveLORA", HTTP_POST, [](AsyncWebServerRequest *request) {},
NULL, // No file upload handler
[this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        request->_tempObject = new String();  // Initialize a String to accumulate the data
    }
    String* body = (String*)request->_tempObject;
    body->concat((char*)data, len);  // Accumulate the incoming data

    if (index + len == total) {  // Last chunk of data
        Serial.println("Received LoRa settings: " + *body);

        // Parse the JSON from the accumulated String
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, *body);
        if (error) {
            Serial.println("Failed to parse JSON!");
            request->send(400, "text/plain", "Failed to parse JSON");
            delete body;  // Clean up the String object
            return;
        }

        // Save the JSON to a file
        File file = LittleFS.open("/settings-lora.json", "w");
        if (!file) {
            Serial.println("Failed to open LoRa settings file for writing");
            request->send(500, "text/plain", "Failed to open file for writing.");
            delete body;  // Clean up the String object
            return;
        }

        // Serialize JSON directly to file
        serializeJson(doc, file);
        file.close();
        Serial.println("LoRa settings saved successfully");

        // Respond to the client
        request->send(200, "text/plain", "LoRa settings saved successfully");
        delete body;  // Clean up the String object
    }
});


    server.on("/espwebc-reboot", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        request->send(200, "text/plain", "Rebooting...");
        handleReboot();
    });

    server.on("/getWifiSettings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        request->send(LittleFS, "/settings-wifi.json", "application/json");
    });

    server.on("/getLORASettings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        request->send(LittleFS, "/settings-lora.json", "application/json");
    });

    #ifdef ENABLE_MQTT
    server.on("/saveMQTT", HTTP_POST, [](AsyncWebServerRequest *request) {},
    NULL, // No file upload handler
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index == 0) {
            request->_tempObject = new String();  // Initialize a String to accumulate the data
        }
        String* body = (String*)request->_tempObject;
        body->concat((char*)data, len);  // Accumulate the incoming data

        if (index + len == total) {  // Last chunk of data
            Serial.println("Received MQTT settings: " + *body);

            // Parse the JSON from the accumulated String
            StaticJsonDocument<1024> doc;
            DeserializationError error = deserializeJson(doc, *body);
            if (error) {
                Serial.println("Failed to parse JSON!");
                request->send(400, "text/plain", "Failed to parse JSON");
                delete body;  // Clean up the String object
                return;
            }

            // Save the JSON to a file
            File file = LittleFS.open("/settings-mqtt.json", "w");
            if (!file) {
                Serial.println("Failed to open MQTT settings file for writing");
                request->send(500, "text/plain", "Failed to open file for writing.");
                delete body;  // Clean up the String object
                return;
            }

            // Serialize JSON directly to file
            serializeJson(doc, file);
            file.close();
            Serial.println("MQTT settings saved successfully");

            // Respond to the client
            request->send(200, "text/plain", "MQTT settings saved successfully");
            delete body;  // Clean up the String object
        }
    }
);
    server.on("/getMQTTSettings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        request->send(LittleFS, "/settings-mqtt.json", "application/json");
    });
    #endif

    server.on("/saveWeb", HTTP_POST, [](AsyncWebServerRequest *request) {},
    NULL, // No file upload handler
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {

    if (!checkAuth(request)) return;
        if (index == 0) {
            request->_tempObject = new String();
        }
        String* body = (String*)request->_tempObject;
        body->concat((char*)data, len);

        if (index + len == total) {  // Last chunk of data
            Serial.println("Received body: " + *body);

            // Parse the JSON
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, *body);
            if (error) {
                Serial.println("Failed to parse JSON!");
                request->send(400, "text/plain", "Failed to parse JSON");
                delete body;  // Clean up the String object
                return;
            }

            // Save the JSON to a file
            File file = LittleFS.open("/settings-web.json", "w");
            if (!file) {
                Serial.println("Failed to open web settings file for writing");
                request->send(500, "text/plain", "Failed to open file for writing.");
                delete body;  // Clean up the String object
                return;
            }

            serializeJson(doc, file);  // Serialize JSON directly to file
            file.close();

            // Restart mDNS with new web name
            String newWebName = doc["Web_name"].as<String>();
            if (newWebName.length() > 0 && MDNS.begin(newWebName.c_str())) {
                Serial.print("mDNS responder restarted: ");
                Serial.println(newWebName + ".local");
                request->send(200, "text/plain", "Web settings saved successfully and mDNS restarted");
            } else {
                Serial.println("mDNS responder not restarted due to invalid or blank web name.");
                request->send(500, "text/plain", "Failed to restart mDNS with new settings");
            }

            delete body;  // Clean up the String object
        }
    }
);

    server.on("/getWebSettings", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        if (!checkAuth(request)) return;
        request->send(LittleFS, "/settings-web.json", "application/json"); 
    });

    server.on("/update-firmware", HTTP_POST, [](AsyncWebServerRequest *request)
              {
            if (Update.hasError()) {
                request->send(500, "text/plain", "OTA update FAILED");
            } else {
                request->send(200, "text/plain", "OTA update SUCCESS. Rebooting...");
                ESP.restart();  // Restart the device after sending the response
            } }, [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
              {
            if (index == 0) {
                Serial.printf("Update Start: %s\n", filename.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { // Start with max available size
                    Update.printError(Serial);
                }
            }

            if (Update.write(data, len) != len) {
                Update.printError(Serial);
            }

            if (final) {
                if (Update.end(true)) { // true to set the size to the current progress
                    Serial.printf("Update Success: %u\nRebooting...\n", index + len);
                } else {
                    Update.printError(Serial);
                    Serial.println("Update Failed");
                }
    } });

    server.on("/ota-url", HTTP_POST, [this](AsyncWebServerRequest *request)
              {
    if (!checkAuth(request)) return; // Ensure the user is authenticated

    if (request->hasParam("url", true)) {
        String firmwareURL = request->getParam("url", true)->value();
        request->send(200, "text/plain", "OTA update started from URL...");
        performOTAUpdateFromURL(firmwareURL); // Function to handle the OTA update from the URL
    } else {
        request->send(400, "text/plain", "Missing URL parameter");
    } });

    server.begin();

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

void ESPWebConnect::setAutoUpdate(unsigned long interval) {
    updateInterval = interval;
}

void ESPWebConnect::addSensor(const char* id, const char* name, const char* unit, const char* icon, int* intValue) {
    dashboardElements.emplace_back(id, name, unit, icon, intValue);
}

void ESPWebConnect::addSensor(const char* id, const char* name, const char* unit, const char* icon, float* floatValue) {
    dashboardElements.emplace_back(id, name, unit, icon, floatValue);
}

void ESPWebConnect::addSensor(const char* id, const char* name, const char* unit, const char* icon, String* stringValue) {
    dashboardElements.emplace_back(id, name, unit, icon, stringValue);
}


void ESPWebConnect::addSwitch(const char* id, const char* name, const char* icon, bool* state) {
    dashboardElements.emplace_back(id, name, icon, state);

    // Register the toggle switch handler
    server.on((String("/toggleSwitch?id=") + id).c_str(), HTTP_GET, [this, state](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        if (request->hasParam("state")) {
            *state = request->getParam("state")->value().equalsIgnoreCase("true");
            request->send(200, "text/plain", "Switch state updated");
        } else {
            request->send(400, "text/plain", "Missing 'state' parameter");
        }
    });
}

void ESPWebConnect::addButton(const char* id, const char* name, const char* icon, std::function<void()> onPress) {
    dashboardElements.emplace_back(id, name, icon, onPress);

    // Register the button press handler
    server.on((String("/pressButton?id=") + id).c_str(), HTTP_GET, [this, onPress](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        onPress();
        request->send(200, "text/plain", "Button pressed");
    });
}

void ESPWebConnect::addInputNum(const char* id, const char* name, const char* icon, int* variable) {
    dashboardElements.emplace_back(id, name, icon, variable);

    // Register a URL handler for form submission
    server.on((String("/") + id).c_str(), HTTP_POST, [this, variable](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        if (request->hasParam("value", true)) {
            *variable = request->getParam("value", true)->value().toInt();
            request->send(200, "text/plain", "Value updated successfully");
        } else {
            request->send(400, "text/plain", "Invalid request: Missing 'value' parameter.");
        }
    });
}

void ESPWebConnect::addInputNum(const char* id, const char* name, const char* icon, float* variable) {
    dashboardElements.emplace_back(id, name, icon, variable);

    // Register a URL handler for form submission
    server.on((String("/") + id).c_str(), HTTP_POST, [this, variable](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        if (request->hasParam("value", true)) {
            *variable = request->getParam("value", true)->value().toFloat();
            request->send(200, "text/plain", "Value updated successfully");
        } else {
            request->send(400, "text/plain", "Invalid request: Missing 'value' parameter.");
        }
    });
}

void ESPWebConnect::addInputText(const char* id, const char* name, const char* icon, String* variable) {
    dashboardElements.emplace_back(id, name, icon, variable);

    // Register a URL handler for form submission
    server.on((String("/") + id).c_str(), HTTP_POST, [this, variable](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        if (request->hasParam("value", true)) {
            *variable = request->getParam("value", true)->value();  // Store the string value
            request->send(200, "text/plain", "Value updated successfully");
        } else {
            request->send(400, "text/plain", "Invalid request: Missing 'value' parameter.");
        }
    });
}

void ESPWebConnect::setAllCardSize(int width, int height) {
    baseWidth = width;
    baseHeight = height;
}

void ESPWebConnect::setCardSize(const char* id, float multiplierX, float multiplierY) {
    for (auto& element : dashboardElements) {
        if (strcmp(element.id, id) == 0) {  // Compare id using strcmp
            element.sizeMultiplierX = multiplierX;
            element.sizeMultiplierY = multiplierY;
            break;
        }
    }
}

void ESPWebConnect::setIconColor(const char* id, const char* color) {
    for (auto& element : dashboardElements) {
        if (strcmp(element.id, id) == 0) {
            element.color = color;
            break;
        }
    }
}

void ESPWebConnect::performOTAUpdateFromURL(const String& firmwareURL) {
    WiFiClientSecure client;
    client.setInsecure();  // Use insecure mode (no certificate check)

    Serial.println("Connecting to " + firmwareURL);

    if (!client.connect(firmwareURL.c_str(), 443)) {
        Serial.println("Connection to server failed!");
        return;
    }

    // Make the GET request
    client.print(String("GET ") + firmwareURL + " HTTP/1.1\r\n" +
                 "Host: " + firmwareURL + "\r\n" +
                 "Connection: close\r\n\r\n");

    // Wait for a response and read the headers
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            break; // Headers finished
        }
    }

    // Start the OTA update
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Serial.println("Cannot begin OTA update");
        return;
    }

    // Read the payload and write it to flash
    size_t written = 0;
    uint8_t buffer[1024];
    while (client.connected() && !client.available()) delay(1);

    while (client.available()) {
        size_t len = client.read(buffer, sizeof(buffer));
        if (len <= 0) break;

        if (Update.write(buffer, len) != len) {
            Serial.println("Failed to write during OTA update");
            Update.printError(Serial);
            return;
        }
        written += len;
    }

    if (Update.end()) {
        if (Update.isFinished()) {
            Serial.printf("OTA Update via URL completed. Written: %u bytes.\n", written);
            ESP.restart();
        } else {
            Serial.println("Update did not finish.");
        }
    } else {
        Serial.println("Error Occurred: ");
        Update.printError(Serial);
    }
}

void ESPWebConnect::handleUpdateError(int error) {
    Serial.printf("Error Occurred during Update.end(): %d\n", error);
    switch (error) {
        case UPDATE_ERROR_WRITE:
            Serial.println("Write error occurred during the update.");
            break;
        case UPDATE_ERROR_ERASE:
            Serial.println("Erase error occurred during the update.");
            break;
        case UPDATE_ERROR_READ:
            Serial.println("Read error occurred during the update.");
            break;
        case UPDATE_ERROR_SPACE:
            Serial.println("Not enough space to store the firmware.");
            break;
        case UPDATE_ERROR_SIZE:
            Serial.println("Firmware size mismatch.");
            break;
        case UPDATE_ERROR_STREAM:
            Serial.println("Stream read error during the update.");
            break;
        case UPDATE_ERROR_MAGIC_BYTE:
            Serial.println("Invalid magic byte in the firmware.");
            break;
        case UPDATE_ERROR_MD5:
            Serial.println("MD5 checksum failed.");
            break;
        default:
            Serial.println("Unknown error occurred.");
            break;
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
    newMessage = false;
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
        html += "<link rel='stylesheet' type='text/css' href='" + cssUrl + "'>";
    } else {
        html += "<link rel='stylesheet' href='/style.css'>";
    }
    // Add JavaScript for updating sensor readings
    html += "<script>function updateReadings() {";
    html += "fetch('/readings').then(response => response.json()).then(data => {";
    for (auto &element : dashboardElements) {
        String sensorId = String(element.id);
        sensorId.toLowerCase();
        if (element.type == DashboardElement::SENSOR_INT || element.type == DashboardElement::SENSOR_FLOAT || element.type == DashboardElement::SENSOR_STRING) {
            html += "document.getElementById('" + sensorId + "').innerText = data." + sensorId + ";";
        }
    }
    html += "});";
    html += "setTimeout(updateReadings, " + String(updateInterval) + ");";
    html += "}</script>";

    // Add JavaScript for updating switches
    html += "<script>function updateSwitches() {";
    html += "fetch('/switches').then(response => response.json()).then(data => {";
    for (auto &element : dashboardElements) {
        if (element.type == DashboardElement::SWITCH) {
            String switchId = String(element.id);
            switchId.toLowerCase();
            html += "document.getElementById('" + switchId + "').checked = data['" + switchId + "'];";
        }
    }
    html += "});";
    html += "setTimeout(updateSwitches, " + String(updateInterval) + ");";
    html += "}</script>";

    // WebSocket for notifications (unchanged)
    html += "<script>var webSocket = new WebSocket('ws://' + window.location.hostname + '/ws');";
    html += "webSocket.onmessage = function(event) {";
    html += "var data = JSON.parse(event.data);";
    html += "showNotification(data.id, data.message, data.messageColor, data.icon, data.iconColor, data.timeout);";
    html += "};";
    html += "function showNotification(id, message, messageColor, icon, iconColor, timeout) {";
    html += "var banner = document.getElementById('notificationBanner');";
    html += "var messageElement = document.getElementById('notificationMessage');";
    html += "var iconElement = document.getElementById('notificationIcon');";
    html += "messageElement.textContent = message;";
    html += "if (messageColor) { messageElement.style.color = messageColor; }";
    html += "if (icon) { iconElement.className = icon; iconElement.style.color = iconColor; }";
    html += "banner.style.display = 'flex';";
    html += "if (timeout > 0) { setTimeout(function() { banner.style.display = 'none'; }, timeout * 1000); }";
    html += "}";
    html += "</script>";

    // Add JavaScript for handling form submissions via AJAX (unchanged)
    html += "<script>";
    html += "function submitInputNum(id) {";
    html += "  var value = document.getElementById(id).value;";
    html += "  fetch('/' + id, {";
    html += "    method: 'POST',";
    html += "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },";
    html += "    body: 'value=' + encodeURIComponent(value)";
    html += "  }).then(response => response.text())";
    html += "  .then(text => {";
    html += "    console.log(text);";
    html += "    alert('Value updated successfully');";
    html += "  }).catch(error => {";
    html += "    console.error('Error:', error);";
    html += "    alert('Failed to update value');";
    html += "  });";
    html += "  return false;";
    html += "}";
    html += "</script>";

    // JavaScript function to handle text input submission
    html += "<script>";
    html += "function submitInputText(id) {";
    html += "  var value = document.getElementById(id).value;";
    html += "  fetch('/' + id, {";
    html += "    method: 'POST',";
    html += "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },";
    html += "    body: 'value=' + encodeURIComponent(value)";
    html += "  }).then(response => response.text())";
    html += "  .then(text => {";
    html += "    console.log(text);";
    html += "    alert('Value updated successfully');";
    html += "  }).catch(error => {";
    html += "    console.error('Error:', error);";
    html += "    alert('Failed to update value');";
    html += "  });";
    html += "  return false;";
    html += "}";
    html += "</script>";

    // Body onload function includes sensor and switch updates
    html += "</head><body onload='updateReadings(); updateSwitches();'>";
    html += "<div id='notificationBanner' class='notification-banner' style='display: none;'>";
    html += "<i id='notificationIcon' class='notification-icon' style='font-size: 32px;'></i>";
    html += "<span id='notificationMessage' class='notification-message'></span>";
    html += "<button id='closeButton' class='close-button' onclick='closeNotification()'>X</button>";
    html += "</div>";
    html += "<br><h1>" + title + "</h1>";
    html += "<br><p>" + description + "</p>";
    html += "<div class='dashboard'>";

    // Generate the dashboard items
    for (auto &element : dashboardElements) {
        html += generateDashboardItem(element);
    }
    
    html += "</div>";
    html += "<footer><button onclick=\"window.location.href='/espwebc'\">Go to ESP Web Config</button>";
    html += "</br><a href=\"https://danielamani.com\" target=\"_blank\" style=\"color: white;\">Code by: DanielAmani.com</a><br></footer></br>";

    // Add JavaScript functions for toggling switches and pressing buttons (unchanged)
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

String ESPWebConnect::generateDashboardItem(const DashboardElement& element) {
    String html;
    String elementId = String(element.id);
    elementId.toLowerCase();

    int calculatedWidth = baseWidth * element.sizeMultiplierX;
    int calculatedHeight = baseHeight * element.sizeMultiplierY;

    if (calculatedWidth < 64) {
        calculatedWidth = 64;
    }

    if (calculatedHeight < 64) {
        calculatedHeight = 64;
    }

    String style = "style='";
    style += "width:" + String(calculatedWidth) + "px;";
    style += "height:" + String(calculatedHeight) + "px;";
    style += "'";

    html += "<div class='card' " + style + ">";

    // Check if icon is not empty before adding it to HTML
    if (element.icon && String(element.icon).length() > 0) {
        html += "<div class='icon'><i";
        if (element.color && String(element.color).length() > 0) {
            html += " style='color: " + String(element.color) + ";'";
        }
        html += " class='" + String(element.icon) + "'></i></div>";
    }

    // Check if name is not empty before adding it to HTML
    if (element.name && String(element.name).length() > 0) {
        if(element.type != DashboardElement::BUTTON){
            html += "<div class='sensor-name'>" + String(element.name) + "</div>";
        }
    }

    String inputType;  // Declare outside the switch

    switch (element.type) {
        case DashboardElement::SENSOR_INT:
            html += "<div class='sensor-value'><span id='" + elementId + "'>" + String(*element.intValue) + "</span>";
            if (element.unit && String(element.unit).length() > 0) {
                html += " <span class='sensor-unit'>" + String(element.unit) + "</span>";
            }
            html += "</div>";
            break;

        case DashboardElement::SENSOR_FLOAT:
            html += "<div class='sensor-value'><span id='" + elementId + "'>" + String(*element.floatValue) + "</span>";
            if (element.unit && String(element.unit).length() > 0) {
                html += " <span class='sensor-unit'>" + String(element.unit) + "</span>";
            }
            html += "</div>";
            break;

        case DashboardElement::SENSOR_STRING:
            html += "<div class='sensor-value'><span id='" + elementId + "'>" + *element.stringValue + "</span>";
            if (element.unit && String(element.unit).length() > 0) {
                html += " <span class='sensor-unit'>" + String(element.unit) + "</span>";
            }
            html += "</div>";
            break;

        case DashboardElement::SWITCH:
            html += "<label class='switch'><input type='checkbox' id='" + elementId + "' onclick='toggleSwitch(\"" + elementId + "\")'><span class='slider_sw'></span></label>";
            break;

        case DashboardElement::BUTTON:
            html += "<button id='" + elementId + "' class='button' onclick='pressButton(\"" + elementId + "\")'>" + String(element.name) + "</button>";
            break;

        case DashboardElement::INPUT_NUM:
        case DashboardElement::INPUT_TEXT:
            inputType = element.type == DashboardElement::INPUT_NUM ? "number" : "text";  // Assign within the case
            html += "<form onsubmit='return submitInputNum(\"" + elementId + "\")'>";
            html += "<input type='" + inputType + "' id='" + elementId + "' name='value'>";
            html += "<button type='submit'>OK</button>";
            html += "</form>";
            break;

        default:
            // Handle any other types if necessary
            break;
    }

    html += "</div>";
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
        request->send(400, "text/plain", "Bad Request: Missing button ID");
        return;
    }

    String id = request->arg("id");

    bool buttonFound = false;
    for (auto &element : dashboardElements) {
        if (strcmp(element.id, id.c_str()) == 0 && element.type == DashboardElement::BUTTON) {  // Use strcmp to compare const char* with String's c_str()
            if (element.onPress) {
                element.onPress();
            }
            buttonFound = true;
            break;
        }
    }

    if (buttonFound) {
        request->send(200, "text/plain", "Button pressed");
    } else {
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
}

void ESPWebConnect::handleReboot() {
    delay(1000);
    ESP.restart();
}

bool ESPWebConnect::isAPMode() const {
    return WiFi.getMode() & WIFI_AP;
}

String ESPWebConnect::generateReadingsJSON() {
    String json = "{";
    for (auto &element : dashboardElements) {
        String sensorId = String(element.id);
        sensorId.toLowerCase();

        if (element.type == DashboardElement::SENSOR_INT) {
            json += "\"" + sensorId + "\":\"" + String(*element.intValue) + "\",";
        } else if (element.type == DashboardElement::SENSOR_FLOAT) {
            json += "\"" + sensorId + "\":\"" + String(*element.floatValue) + "\",";
        } else if (element.type == DashboardElement::SENSOR_STRING) {
            json += "\"" + sensorId + "\":\"" + String(*element.stringValue) + "\",";
        }
    }
    if (json.length() > 1) {
        json.remove(json.length() - 1); // Remove the last comma
    }
    json += "}";
    return json;
}


String ESPWebConnect::generateSwitchesJSON() {
    String json = "{";
    for (auto &element : dashboardElements) {
        if (element.type == DashboardElement::SWITCH) {
            // Convert the const char* id to lowercase
            String switchId = String(element.id);
            switchId.toLowerCase();
            json += "\"" + switchId + "\": " + (*element.state ? "true" : "false") + ",";
        }
    }
    if (json.length() > 1) {
        json.remove(json.length() - 1); // Remove the last comma
    }
    json += "}";
    return json;
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

void ESPWebConnect::saveLORASettings(const LORASettings& settings) {
    StaticJsonDocument<215> doc;
    doc["LORA_Key"] = settings.LORA_Key;
    doc["LORA_CRC"] = settings.LORA_CRC;
    doc["LORA_RSSI"] = settings.LORA_RSSI;
    doc["LORA_PacketHZErr"] = settings.LORA_PacketHZErr;
    doc["LORA_Spread"] = settings.LORA_Spread;
    doc["LORA_Coding"] = settings.LORA_Coding;
    doc["LORA_TxPwr"] = settings.LORA_TxPwr;
    doc["LORA_Reg"] = settings.LORA_Reg;

    File file = LittleFS.open("/settings-lora.json", "w");
    if (!file) {
        Serial.println("Failed to open LoRa settings file for writing");
        return;
    }

    serializeJson(doc, file);
    file.close();
}


#ifdef ENABLE_MQTT

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