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
        if (!checkAuth(request)) return;

        if (request->hasParam("firmwareUrl")) {
            String firmwareURL = request->getParam("firmwareUrl")->value();
            request->send(200, "text/plain", "Starting OTA update...");
            performOTAUpdateFromURL(firmwareURL);
        } else {
            request->send(LittleFS, "/espwebc.html", "text/html");
        }
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

    server.on("/espwebc-reboot", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Rebooting...");
        handleReboot();
    });

    server.on("/getWifiSettings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        request->send(LittleFS, "/settings-wifi.json", "application/json");
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

    server.on("/getWebSettings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        request->send(LittleFS, "/settings-web.json", "application/json");
    });

    server.on("/update-firmware", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
    }, [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
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
            }
        }
    });

    server.on("/ota", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;

        if (request->hasParam("url", true)) {
            String firmwareURL = request->getParam("url", true)->value();
            request->send(200, "text/plain", "OTA update started...");
            performOTAUpdateFromURL(firmwareURL);
        } else {
            request->send(400, "text/plain", "Missing URL parameter");
        }
    });

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

void ESPWebConnect::addSensor(const String& id, const String& name, const String& unit, const String& icon, std::function<float()> readFunction) {
    dashboardElements.emplace_back(id, name, unit, icon, readFunction);
}

void ESPWebConnect::addSwitch(const String& id, const String& name, const String& icon, bool* state) {
    dashboardElements.emplace_back(id, name, icon, state);

    // Register the toggle switch handler
    server.on(("/toggleSwitch?id=" + id).c_str(), HTTP_GET, [this, state](AsyncWebServerRequest *request) {
        if (request->hasParam("state")) {
            *state = request->getParam("state")->value().equalsIgnoreCase("true");
            request->send(200, "text/plain", "Switch state updated");
        } else {
            request->send(400, "text/plain", "Missing 'state' parameter");
        }
    });
}

void ESPWebConnect::addButton(const String& id, const String& name, const String& icon, bool update, std::function<void()> onPress) {
    dashboardElements.emplace_back(id, name, icon, onPress);

    // Register the button press handler
    server.on(("/pressButton?id=" + id).c_str(), HTTP_GET, [this, onPress](AsyncWebServerRequest *request) {
        onPress();
        request->send(200, "text/plain", "Button pressed");
    });
}

void ESPWebConnect::addInputNum(const String& id, const String& name, const String& icon, int* variable) {
    dashboardElements.emplace_back(id, name, icon, variable);

    // Register a URL handler for form submission
    server.on(("/" + id).c_str(), HTTP_POST, [this, variable](AsyncWebServerRequest *request) {
        if (request->hasParam("value", true)) {
            *variable = request->getParam("value", true)->value().toInt();
            request->send(200, "text/plain", "Value updated successfully");
        } else {
            request->send(400, "text/plain", "Invalid request: Missing 'value' parameter.");
        }
    });
}

void ESPWebConnect::addInputNum(const String& id, const String& name, const String& icon, float* variable) {
    dashboardElements.emplace_back(id, name, icon, variable);

    // Register a URL handler for form submission
    server.on(("/" + id).c_str(), HTTP_POST, [this, variable](AsyncWebServerRequest *request) {
        if (request->hasParam("value", true)) {
            *variable = request->getParam("value", true)->value().toFloat();
            request->send(200, "text/plain", "Value updated successfully");
        } else {
            request->send(400, "text/plain", "Invalid request: Missing 'value' parameter.");
        }
    });
}



void ESPWebConnect::setIconColor(const String& id, const String& color) {
    for (auto& element : dashboardElements) {
        if (element.id == id) {
            element.color = color; // Set color regardless of element type
            break;
        }
    }
}


void ESPWebConnect::performOTAUpdateFromURL(const String& firmwareURL) {
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 30000, // 30 seconds timeout
        .idle_core_mask = (1 << 0) | (1 << 1), // Watchdog on both cores
        .trigger_panic = true // Trigger a panic on watchdog timeout
    };

    if (esp_task_wdt_status(NULL) == ESP_ERR_NOT_FOUND) {
        esp_task_wdt_init(&wdt_config);
    }

    WiFiClientSecure client;
    client.setInsecure();

    String host = firmwareURL.substring(8, firmwareURL.indexOf("/", 8));
    String path = firmwareURL.substring(firmwareURL.indexOf("/", 8));

    if (client.connect(host.c_str(), 443)) {
        Serial.println("Connected to server");

        client.print("GET " + path + " HTTP/1.1\r\n");
        client.print("Host: " + host + "\r\n");
        client.println("Connection: close\r\n");
        client.println();

        bool endOfHeaders = false;
        String headers = "";
        String http_response_code = "error";
        const size_t bufferSize = 256;
        uint8_t buffer[bufferSize];
        String location;

        while (client.connected() && !endOfHeaders) {
            if (client.available()) {
                String line = client.readStringUntil('\n');
                headers += line + "\n";
                if (line.startsWith("HTTP/1.1")) {
                    http_response_code = line.substring(9, 12);
                }
                if (line.startsWith("Location: ")) {
                    location = line.substring(10);
                    location.trim();
                }
                if (line == "\r") {
                    endOfHeaders = true;
                }
            }
        }

        Serial.println("HTTP response code: " + http_response_code);

        if (http_response_code == "302" && location.length() > 0) {
            Serial.println("Following redirect to: " + location);
            performOTAUpdateFromURL(location);
            return;
        }

        Serial.printf("Free sketch space: %d bytes\n", ESP.getFreeSketchSpace());

        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
            Serial.println("Cannot do the update");
            return;
        }

        size_t written = 0;
        while (client.connected()) {
            if (client.available()) {
                size_t bytesRead = client.readBytes(buffer, bufferSize);

                size_t writtenNow = 0;
                size_t retries = 0;
                const size_t maxRetries = 3;

                while (retries < maxRetries && writtenNow != bytesRead) {
                    writtenNow = Update.write(buffer, bytesRead);
                    if (writtenNow != bytesRead) {
                        retries++;
                        Serial.printf("Retrying flash write (%d/%d)...\n", retries, maxRetries);
                    }
                }

                if (writtenNow != bytesRead) {
                    Serial.printf("Failed to write to flash after %d retries: only %d/%d bytes written\n", retries, writtenNow, bytesRead);
                    return;
                }

                written += writtenNow;

                Serial.printf("Written %d bytes; Total written: %d bytes\n", writtenNow, written);

                esp_task_wdt_reset();
            } else {
                vTaskDelay(1);
                esp_task_wdt_reset();
            }
        }

        Serial.printf("Total written: %d bytes\n", written);

        if (Update.end()) {
            Serial.println("Successful update");
            Serial.println("Resetting in 4 seconds...");
            delay(4000);
            ESP.restart();
        } else {
            int error = Update.getError();
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
        client.stop();
    } else {
        Serial.println("Failed to connect to server");
    }

    if (esp_task_wdt_status(NULL) == ESP_ERR_NOT_FOUND) {
        wdt_config.timeout_ms = 5000;
        esp_task_wdt_init(&wdt_config);
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
        html += "<link rel='stylesheet' href='" + cssUrl + "'>";
    } else {
        html += "<link rel='stylesheet' href='/style.css'>";
    }

    // Add necessary JavaScript for handling sensors and switches updates
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

    // WebSocket for notifications
    html += "<script>var webSocket = new WebSocket('ws://' + window.location.hostname + '/ws');";
    html += "webSocket.onmessage = function(event) {";
    html += "var data = JSON.parse(event.data);";
    html += "showNotification(data.id, data.message, data.messageColor, data.icon, data.iconColor, data.timeout);";
    html += "};</script>";
    
    // Notification handling script
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
    
    // Add necessary JavaScript for handling form submission via AJAX
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
    html += "    alert('Value updated successfully');";  // Optional: give feedback to the user
    html += "  }).catch(error => {";
    html += "    console.error('Error:', error);";
    html += "    alert('Failed to update value');";  // Optional: alert in case of failure
    html += "  });";
    html += "  return false;";  // Prevent default form submission
    html += "}";
    html += "</script>";

    html += "</head><body onload='updateReadings(); updateSwitches();'>";
    html += "<div id='notificationBanner' class='notification-banner' style='display: none;'>";
    html += "<i id='notificationIcon' class='notification-icon' style='font-size: 32px;'></i>";
    html += "<span id='notificationMessage' class='notification-message'></span>";
    html += "<button id='closeButton' class='close-button' onclick='closeNotification()'>X</button>";
    html += "</div>";
    html += "<br><h1>" + title + "</h1>";
    html += "<br><p>" + description + "</p>";
    html += "<div class='dashboard'>";

    // Consolidated loop to handle all dashboard elements
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
        } else if (element.type == DashboardElement::INPUT_NUM) {
            html += "<form onsubmit='return submitInputNum(\"" + elementId + "\")'>";
            html += "<input type='number' id='" + elementId + "' name='value'>";
            html += "<button type='submit'>OK</button>";
            html += "</form>";
        }

        html += "</div>";
    }

    html += "</div>";
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
                element.onPress();
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
    json.remove(json.length() - 1);
    json += "}";
    return json;
}

String ESPWebConnect::generateReadingsJSON() {
    String json = "{";
    for (auto &element : dashboardElements) {
        if (element.type == DashboardElement::SENSOR) {
            json += "\"" + element.id + "\":" + String(element.readFunction()) + ",";
        }
    }
    if (json.endsWith(",")) json.remove(json.length() - 1); // Remove the trailing comma
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
