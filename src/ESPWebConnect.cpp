#include "ESPWebConnect.h"
//#define ENABLE_MQTT

ESPWebConnect::ESPWebConnect()
    : server(80), ws("/ws"),
      dashPath("/dashboard")
{
#ifdef ENABLE_MQTT
    mqttClient.setClient(wifiClient);
#endif

    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
               { this->onWebSocketEvent(server, client, type, arg, data, len); });
    server.addHandler(&ws);
}

void ESPWebConnect::begin()
{
    Serial.begin(115200);
    if (!LittleFS.begin())
    {
        #ifdef ENABLE_DEBUG
        Serial.println("LittleFS mount failed, attempting to format...");
        #endif
        LittleFS.format();
        if (!LittleFS.begin())
        {
            #ifdef ENABLE_DEBUG
            Serial.println("LittleFS mount failed after formatting");
            #endif
            return;
        }
    }

    if (!readWebSettings(webSettings))
    {
        webSettings.Web_Lock = false;
        webSettings.Web_User = "admin";
        webSettings.Web_Pass = "admin";
        webSettings.Web_name = "ESPWebConnect";
    }

    bool wifiConnected = false;
    if (readWifiSettings(wifiSettings))
    {
        wifiConnected = configureWiFi(wifiSettings.SSID_Name.c_str(), wifiSettings.SSID_Pass.c_str());
    }
    if (!wifiConnected)
    {
        startAP(wifiSettings.SSID_AP_Name.c_str(), wifiSettings.SSID_AP_Pass.c_str());
    }

#ifdef ENABLE_MQTT
    if (!readMQTTSettings(mqttSettings))
    {
        Serial.println("Failed to load MQTT settings");
    }
#endif

    if (webSettings.Web_name.length() > 0 && MDNS.begin(webSettings.Web_name.c_str()))
    {
        #ifdef ENABLE_DEBUG_INFO
        Serial.print("mDNS responder started: ");
        Serial.print(webSettings.Web_name);
        Serial.println(".local");
        #endif
    }
    else
    {
#ifdef ENABLE_DEBUG
        Serial.println("mDNS responder not started due to invalid or blank web name.");
#endif
    }

    server.on("/espwebc", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
    if (!checkAuth(request)) return;
        request->send(LittleFS, "/espwebc.html", "text/html"); });

    server.on("/style.css", HTTP_GET, [this](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/style.css", "text/css"); });

    server.on("/dash.js", HTTP_GET, [this](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/dash.js", "text/javascript"); });

    server.on("/systeminfo", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        String jsonResponse = "{";
        jsonResponse += "\"libWebC\":\"" + ESPwebCVersion + "\",";
        jsonResponse += "\"developer\":\"" + manufacturerDeveloper + "\",";
        jsonResponse += "\"device\":\"" + manufacturerDevice + "\",";
        jsonResponse += "\"description\":\"" + manufacturerDescDevice + "\",";
        jsonResponse += "\"version\":\"" + manufacturerVersionDevice + "\"";
        jsonResponse += "}";
        request->send(200, "application/json", jsonResponse);
    });

    server.on(dashPath.c_str(), HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        if (!checkAuth(request)) return;
        request->send(200, "text/html", generateDashboardHTML()); });

    server.on("/allReadings", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
                  // Check for authorization
                  if (!checkAuth(request))
                  {
                      request->send(401, "text/plain", "Unauthorized");
                      return;
                  }

                  // Create a JSON document to store all readings
                  // Dynamically allocate size based on the number of elements (128 bytes per element is estimated)
                  DynamicJsonDocument doc(128 * dashboardElements.size());

                  // Loop through all dashboard elements
                  for (auto &element : dashboardElements)
                  {
                      String sensorId = String(element.id);
                      sensorId.toLowerCase(); // Convert to lowercase for consistency
                      sensorId += "-val";

                      // Handle different types of dashboard elements
                      if (element.type == DashboardElement::SENSOR_INT)
                      {
                          doc[sensorId] = *element.intValue; // Add integer sensor value
                      }
                      else if (element.type == DashboardElement::SENSOR_FLOAT)
                      {
                          doc[sensorId] = *element.floatValue; // Add float sensor value
                      }
                      else if (element.type == DashboardElement::SENSOR_STRING)
                      {
                          doc[sensorId] = String(*element.stringValue); // Add string sensor value
                      }
                      else if (element.type == DashboardElement::SWITCH)
                      {
                          doc[sensorId] = *element.state; // Add switch state (true/false)
                      }
                      /*
                      else {
                          // Add unsupported or unknown element type as null
                          doc[sensorId] = nullptr;
                      }
                      */
                  }

                  // Serialize the JSON response
                  String jsonResponse;
                  if (serializeJson(doc, jsonResponse) == 0)
                  {
                      // Serialization failed
                      request->send(500, "text/plain", "Failed to serialize JSON");
                      return;
                  }

                  // Send the JSON response
                  request->send(200, "application/json", jsonResponse);

// For debugging purposes (optional)
#ifdef ENABLE_DEBUG
                  Serial.println("All Readings JSON Response:");
                  Serial.println(jsonResponse);
#endif
              });

    server.on("/toggleSwitch", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        if (!checkAuth(request)) return;
        handleToggleSwitch(request);
        request->send(200, "text/plain", "OK"); });

    server.on("/pressButton", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        if (!checkAuth(request)) return;
        handleButtonPress(request); });

    server.on("/notify", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        if (!checkAuth(request)) return;
        handleNotification(request);
        request->send(200, "text/plain", "Notification sent"); });

    server.on("/saveWifi", HTTP_POST, [](AsyncWebServerRequest *request) {},
              NULL, // No file upload handler
              [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
              {
        if (index == 0) {
            request->_tempObject = new String();  // Initialize a String to accumulate the data
        }
        String* body = (String*)request->_tempObject;
        body->concat((char*)data, len);  // Accumulate the incoming data

        if (index + len == total) {  // Last chunk of data
            #ifdef ENABLE_DEBUG
            Serial.println("Received WiFi settings: " + *body);
            #endif

            // Parse the JSON from the accumulated String
            StaticJsonDocument<1024> doc;
            DeserializationError error = deserializeJson(doc, *body);
            if (error) {
                #ifdef ENABLE_DEBUG
                Serial.println("Failed to parse JSON!");
                #endif
                request->send(400, "text/plain", "Failed to parse JSON");
                delete body;  // Clean up the String object
                return;
            }

            // Save the JSON to a file
            File file = LittleFS.open("/settings-wifi.json", "w");
            if (!file) {
                #ifdef ENABLE_DEBUG
                Serial.println("Failed to open WiFi settings file for writing");
                #endif
                request->send(500, "text/plain", "Failed to open file for writing.");
                delete body;  // Clean up the String object
                return;
            }

            // Serialize JSON directly to file
            serializeJson(doc, file);
            file.close();
            #ifdef ENABLE_DEBUG_INFO
            Serial.println("WiFi settings saved successfully");
            #endif

            // Respond to the client
            request->send(200, "text/plain", "WiFi settings saved successfully");
            delete body;  // Clean up the String object
        } });

    server.on("/espwebc-reboot", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        if (!checkAuth(request)) return;
        request->send(200, "text/plain", "Rebooting...");
        handleReboot(); });

    server.on("/getWifiSettings", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        if (!checkAuth(request)) return;
        request->send(LittleFS, "/settings-wifi.json", "application/json"); });

#ifdef ENABLE_MQTT
    server.on("/saveMQTT", HTTP_POST, [](AsyncWebServerRequest *request) {},
              NULL, // No file upload handler
              [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
              {
        if (index == 0) {
            request->_tempObject = new String();  // Initialize a String to accumulate the data
        }
        String* body = (String*)request->_tempObject;
        body->concat((char*)data, len);  // Accumulate the incoming data

        if (index + len == total) {  // Last chunk of data
            #ifdef ENABLE_DEBUG_INFO
            Serial.println("Received MQTT settings: " + *body);
            #endif

            // Parse the JSON from the accumulated String
            StaticJsonDocument<1024> doc;
            DeserializationError error = deserializeJson(doc, *body);
            if (error) {
                #ifdef ENABLE_DEBUG
                Serial.println("Failed to parse JSON!");
                #endif
                request->send(400, "text/plain", "Failed to parse JSON");
                delete body;  // Clean up the String object
                return;
            }

            // Save the JSON to a file
            File file = LittleFS.open("/settings-mqtt.json", "w");
            if (!file) {
                #ifdef ENABLE_DEBUG
                Serial.println("Failed to open MQTT settings file for writing");
                #endif
                request->send(500, "text/plain", "Failed to open file for writing.");
                delete body;  // Clean up the String object
                return;
            }

            // Serialize JSON directly to file
            serializeJson(doc, file);
            file.close();
            #ifdef ENABLE_DEBUG_INFO
            Serial.println("MQTT settings saved successfully");
            #endif

            // Respond to the client
            request->send(200, "text/plain", "MQTT settings saved successfully");
            delete body;  // Clean up the String object
        } });
    server.on("/getMQTTSettings", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        if (!checkAuth(request)) return;
        request->send(LittleFS, "/settings-mqtt.json", "application/json"); });
#endif

    server.on("/saveWeb", HTTP_POST, [](AsyncWebServerRequest *request) {},
              NULL, // No file upload handler
              [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
              {

    if (!checkAuth(request)) return;
        if (index == 0) {
            request->_tempObject = new String();
        }
        String* body = (String*)request->_tempObject;
        body->concat((char*)data, len);

        if (index + len == total) {  // Last chunk of data
            #ifdef ENABLE_DEBUG_INFO
            Serial.println("Received body: " + *body);
            #endif

            // Parse the JSON
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, *body);
            if (error) {
                #ifdef ENABLE_DEBUG
                Serial.println("Failed to parse JSON!");
                #endif
                request->send(400, "text/plain", "Failed to parse JSON");
                delete body;  // Clean up the String object
                return;
            }

            // Save the JSON to a file
            File file = LittleFS.open("/settings-web.json", "w");
            if (!file) {
                #ifdef ENABLE_DEBUG
                Serial.println("Failed to open web settings file for writing");
                #endif
                request->send(500, "text/plain", "Failed to open file for writing.");
                delete body;  // Clean up the String object
                return;
            }

            serializeJson(doc, file);  // Serialize JSON directly to file
            file.close();

            // Restart mDNS with new web name
            String newWebName = doc["Web_name"].as<String>();
            if (newWebName.length() > 0 && MDNS.begin(newWebName.c_str())) {
                #ifdef ENABLE_DEBUG_INFO
                Serial.print("mDNS responder restarted: ");
                Serial.println(newWebName + ".local");
                #endif
                request->send(200, "text/plain", "Web settings saved successfully and mDNS restarted");
            } else {
                #ifdef ENABLE_DEBUG_INFO
                Serial.println("mDNS responder not restarted due to invalid or blank web name.");
                #endif
                request->send(500, "text/plain", "Failed to restart mDNS with new settings");
            }

            delete body;  // Clean up the String object
        } });

    server.on("/getWebSettings", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        if (!checkAuth(request)) return;
        request->send(LittleFS, "/settings-web.json", "application/json"); });

    server.on("/update-firmware", HTTP_POST, [this](AsyncWebServerRequest *request)                                                    // Capture 'this'
              {
    if (!checkAuth(request)) return; // Ensure the user is authenticated

    if (Update.hasError()) {
        request->send(500, "text/plain", "OTA update FAILED");
    } else {
        request->send(200, "text/plain", "OTA update SUCCESS. Rebooting...");
        ESP.restart();  // Restart the device after sending the response
    } }, [this](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) // Capture 'this'
              {
    if (index == 0) {
        #ifdef ENABLE_DEBUG_INFO
        Serial.printf("Update Start: %s\n", filename.c_str());
        #endif
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { // Start with max available size
            Update.printError(Serial);
        }
    }

    if (Update.write(data, len) != len) {
        Update.printError(Serial);
    }

    if (final) {
        if (Update.end(true)) { // true to set the size to the current progress
            #ifdef ENABLE_DEBUG_INFO
            Serial.printf("Update Success: %u\nRebooting...\n", index + len);
            #endif
        } else {
            Update.printError(Serial);
            #ifdef ENABLE_DEBUG_INFO
            Serial.println("Update Failed");
            #endif
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
    mqttClient.setCallback([this](char *topic, byte *payload, unsigned int length)
                           { mqttCallback(topic, payload, length); });
    #ifdef ENABLE_DEBUG_INFO
    Serial.println("Attempting MQTT connection...");
    #endif
    if (mqttClient.connect("ESPWebConnectClient", mqttSettings.MQTT_User.c_str(), mqttSettings.MQTT_Pass.c_str()))
    {
        #ifdef ENABLE_DEBUG_INFO
        Serial.println("MQTT connected");
        #endif
        mqttClient.subscribe(mqttSettings.MQTT_Recv.c_str());
    }
    else
    {
        #ifdef ENABLE_DEBUG_INFO
        Serial.print("MQTT connection failed, rc=");
        Serial.print(mqttClient.state());
        #endif
    }
#endif
}

bool ESPWebConnect::configureWiFi(const char *ssid, const char *password)
{
    #ifdef ENABLE_DEBUG_INFO
    Serial.print("Connecting to WiFi network: ");
    Serial.println(ssid);
    #endif
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    delay(1000);

    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 25)
    {
        delay(1000);
        #ifdef ENABLE_DEBUG_INFO
        Serial.print(".");
        #endif
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        wifiSettings.ESP_MAC = WiFi.macAddress();
        saveWifiSettings(wifiSettings);
        #ifdef ENABLE_DEBUG_INFO
        Serial.println("\nConnected to WiFi successfully.");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("WiFi Channel: ");
        Serial.println(WiFi.channel());
        Serial.print("MAC Address: ");
        Serial.println(wifiSettings.ESP_MAC);
        #endif
        return true;
    }
    else
    {
        #ifdef ENABLE_DEBUG_INFO
        Serial.println("\nFailed to connect to WiFi.");
        #endif
        return false;
    }
}

void ESPWebConnect::startAP(const char *ssid, const char *password)
{
    #ifdef ENABLE_DEBUG_INFO
    Serial.println("Starting AP mode...");
    #endif
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);

    if (ssid == nullptr || strlen(ssid) == 0)
    {
        ssid = "ESPWebConnect";
    }
    if (password == nullptr || strlen(password) == 0)
    {
        password = "D@NP8888";
    }

    WiFi.softAP(ssid, password);
    wifiSettings.ESP_MAC = WiFi.softAPmacAddress();
    saveWifiSettings(wifiSettings);

    #ifdef ENABLE_DEBUG_INFO
    Serial.print("AP Mode, connect to SSID: ");
    Serial.print(ssid);
    Serial.print(" with IP: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("MAC Address: ");
    Serial.println(wifiSettings.ESP_MAC);
    #endif  
}

void ESPWebConnect::onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_DATA)
    {
        String message = String((char *)data);
    }
}


void ESPWebConnect::handleToggleSwitch(AsyncWebServerRequest *request)
{
    String id = request->arg("id");
    String state = request->arg("state");
    for (auto &element : dashboardElements)
    {
        if (element.type == DashboardElement::SWITCH)
        {
            String switchId = element.id;
            switchId.toLowerCase();
            if (switchId == id)
            {
                *element.state = (state == "true");
                break;
            }
        }
    }
    generateDashboardHTML();
}

void ESPWebConnect::handleNotification(AsyncWebServerRequest *request)
{
    String id = request->arg("id");
    String message = request->arg("message");
    String messageColor = request->arg("messageColor");
    String icon = request->arg("icon");
    String iconColor = request->arg("iconColor");
    int timeout = request->arg("timeout").toInt();

    sendNotification(id, message, messageColor, icon, iconColor, timeout);
}

void ESPWebConnect::sendNotification(const String &id, const String &message, const String &messageColor, const String &icon, const String &iconColor, int timeout)
{
    String notificationJson = "{\"id\":\"" + id + "\",\"message\":\"" + message + "\",\"messageColor\":\"" + messageColor + "\",\"icon\":\"" + icon + "\",\"iconColor\":\"" + iconColor + "\",\"timeout\":" + String(timeout) + "}";
    ws.textAll(notificationJson);
}

void ESPWebConnect::setDashPath(const String &path)
{
    this->dashPath = path;
}

void ESPWebConnect::setDashInfo(const char *title, const char *description, const char *imageurl, const char *footer) {
    // Use existing member values as defaults
    dashTitle = (title != nullptr && strlen(title) > 0) ? title : dashTitle;
    dashDescription = (description != nullptr && strlen(description) > 0) ? description : dashDescription;
    dashImageUrl = (imageurl != nullptr && strlen(imageurl) > 0) ? imageurl : dashImageUrl;
    dashFooter = (footer != nullptr && strlen(footer) > 0) ? footer : dashFooter;
}

void ESPWebConnect::setManifactureInfo(const char *developer, const char* device, const char *descDevice, const char *versionDevice) {
    // Use existing member values as defaults
    manufacturerDeveloper = (developer != nullptr && strlen(developer) > 0) ? developer : manufacturerDeveloper;
    manufacturerDevice = (device != nullptr && strlen(device) > 0) ? device : manufacturerDevice;
    manufacturerDescDevice = (descDevice != nullptr && strlen(descDevice) > 0) ? descDevice : manufacturerDescDevice;
    manufacturerVersionDevice = (versionDevice != nullptr && strlen(versionDevice) > 0) ? versionDevice : manufacturerVersionDevice;
}


void ESPWebConnect::setIconUrl(const String &url)
{
    iconUrl = url;
}

void ESPWebConnect::setCSS(const String &url)
{
    cssUrl = url;
}

void ESPWebConnect::setAutoUpdate(unsigned long interval)
{
    if(interval<500){
        interval = 3000;
    }
    updateInterval = interval;
}

void ESPWebConnect::addSensor(const char *id, const char *name, const char *desc, const char *icon, int *intValue, const char *unit)
{
    dashboardElements.emplace_back(id, name, desc, icon, intValue, unit);
}

void ESPWebConnect::addSensor(const char *id, const char *name, const char *desc, const char *icon, float *floatValue, const char *unit)
{
    dashboardElements.emplace_back(id, name, desc, icon, floatValue, unit);
}

void ESPWebConnect::addSensor(const char *id, const char *name, const char *desc, const char *icon, String *stringValue, const char *unit)
{
    dashboardElements.emplace_back(id, name, desc, icon, stringValue, unit);
}

void ESPWebConnect::addSwitch(const char *id, const char *name, const char *desc, const char *icon, bool *state)
{
    dashboardElements.emplace_back(id, name, desc, icon, state);

    server.on((String("/toggleSwitch?id=") + id).c_str(), HTTP_GET, [this, state](AsyncWebServerRequest *request)
              {
        if (!checkAuth(request)) return;
        if (request->hasParam("state")) {
            *state = request->getParam("state")->value().equalsIgnoreCase("true");
            request->send(200, "text/plain", "Switch state updated");
        } else {
            request->send(400, "text/plain", "Missing 'state' parameter");
        } });
}


void ESPWebConnect::addButton(const char *id, const char *name, const char *desc, const char *icon, std::function<void()> onPress)
{
    dashboardElements.emplace_back(id, name, desc, icon, onPress);

    server.on((String("/pressButton?id=") + id).c_str(), HTTP_GET, [this, onPress](AsyncWebServerRequest *request)
              {
        if (!checkAuth(request)) return;
        onPress();
        request->send(200, "text/plain", "Button pressed"); });
}

void ESPWebConnect::addInputNum(const char *id, const char *name, const char *desc, const char *icon, int *variable)
{
    dashboardElements.emplace_back(id, name, desc, icon, variable);

    server.on((String("/") + id).c_str(), HTTP_POST, [this, variable](AsyncWebServerRequest *request)
              {
        if (!checkAuth(request)) return;
        if (request->hasParam("value", true)) {
            *variable = request->getParam("value", true)->value().toInt();
            request->send(200, "text/plain", "Value updated successfully");
        } else {
            request->send(400, "text/plain", "Invalid request: Missing 'value' parameter.");
        } });
}

void ESPWebConnect::addInputNum(const char *id, const char *name, const char *desc, const char *icon, float *variable)
{
    dashboardElements.emplace_back(id, name, desc, icon, variable);

    server.on((String("/") + id).c_str(), HTTP_POST, [this, variable](AsyncWebServerRequest *request)
              {
        if (!checkAuth(request)) return;
        if (request->hasParam("value", true)) {
            *variable = request->getParam("value", true)->value().toInt();
            request->send(200, "text/plain", "Value updated successfully");
        } else {
            request->send(400, "text/plain", "Invalid request: Missing 'value' parameter.");
        } });
}

void ESPWebConnect::addInputText(const char *id, const char *name, const char *desc, const char *icon, String *variable)
{
    dashboardElements.emplace_back(id, name, desc, icon, variable);

    server.on((String("/") + id).c_str(), HTTP_POST, [this, variable](AsyncWebServerRequest *request)
              {
        if (!checkAuth(request)) return;
        if (request->hasParam("value", true)) {
            *variable = request->getParam("value", true)->value();  // Store the string value
            request->send(200, "text/plain", "Value updated successfully");
        } else {
            request->send(400, "text/plain", "Invalid request: Missing 'value' parameter.");
        } });
}

void ESPWebConnect::setIconColor(const char *id, const char *color)
{
    for (auto &element : dashboardElements)
    {
        if (strcmp(element.id, id) == 0)
        {
            element.color = color;
            break;
        }
    }
}

void ESPWebConnect::performOTAUpdateFromURL(const String &firmwareURL)
{
    WiFiClientSecure client;
    client.setInsecure(); // Use insecure mode (no certificate check)
    
    #ifdef ENABLE_DEBUG_INFO
    Serial.println("Connecting to " + firmwareURL);
    #endif

    if (!client.connect(firmwareURL.c_str(), 443))
    {
        #ifdef ENABLE_DEBUG_INFO
        Serial.println("Connection to server failed!");
        return;
        #endif
    }

    // Make the GET request
    client.print(String("GET ") + firmwareURL + " HTTP/1.1\r\n" +
                 "Host: " + firmwareURL + "\r\n" +
                 "Connection: close\r\n\r\n");

    // Wait for a response and read the headers
    while (client.connected())
    {
        String line = client.readStringUntil('\n');
        if (line == "\r")
        {
            break; // Headers finished
        }
    }

    // Start the OTA update
    if (!Update.begin(UPDATE_SIZE_UNKNOWN))
    {
        #ifdef ENABLE_DEBUG_INFO
        Serial.println("Cannot begin OTA update");
        #endif
        return;
    }

    // Read the payload and write it to flash
    size_t written = 0;
    uint8_t buffer[1024];
    while (client.connected() && !client.available())
        delay(1);

    while (client.available())
    {
        size_t len = client.read(buffer, sizeof(buffer));
        if (len <= 0)
            break;

        if (Update.write(buffer, len) != len)
        {
            #ifdef ENABLE_DEBUG
            Serial.println("Failed to write during OTA update");
            Update.printError(Serial);
            #endif
            return;
        }
        written += len;
    }

    if (Update.end())
    {
        if (Update.isFinished())
        {
            #ifdef ENABLE_DEBUG_INFO
            Serial.printf("OTA Update via URL completed. Written: %u bytes.\n", written);
            #endif
            ESP.restart();
        }
        else
        {
            #ifdef ENABLE_DEBUG_INFO
            Serial.println("Update did not finish.");
            #endif
        }
    }
    else
    {
        #ifdef ENABLE_DEBUG
        Serial.println("Error Occurred: ");
        #endif
        Update.printError(Serial);
    }
}

#ifdef ENABLE_MQTT

void ESPWebConnect::mqttCallback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    latestMessage = "";
    for (unsigned int i = 0; i < length; i++)
    {
        latestMessage += (char)payload[i];
    }
    Serial.println(latestMessage);
    newMessage = true;
}

void ESPWebConnect::enableMQTT()
{
    if (isAPMode())
        return;
    mqttClient.setServer(mqttSettings.MQTT_Broker.c_str(), mqttSettings.MQTT_Port);
    mqttClient.setCallback([this](char *topic, byte *payload, unsigned int length)
                           { this->mqttCallback(topic, payload, length); });
    reconnectMQTT();
}

void ESPWebConnect::reconnectMQTT()
{
    if (isAPMode())
        return;

    int attemptCount = 0;
    const int maxAttempts = 3;

    while (!mqttClient.connected() && (attemptCount < maxAttempts))
    {
        Serial.print("Attempting MQTT connection...");
        if (mqttClient.connect("ESP32Client", mqttSettings.MQTT_User.c_str(), mqttSettings.MQTT_Pass.c_str()))
        {
            Serial.println("connected");
            mqttClient.subscribe(mqttSettings.MQTT_Recv.c_str());
            Serial.print("Subscribed to topic: ");
            Serial.println(mqttSettings.MQTT_Recv.c_str());
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
            attemptCount++;
        }
    }

    if (attemptCount >= maxAttempts)
    {
        Serial.println("MQTT connection failed after 3 attempts.");
    }
}

void ESPWebConnect::publishToMQTT(const String &payload)
{
    if (isAPMode())
        return;
    reconnectMQTT();
    mqttClient.publish(mqttSettings.MQTT_Send.c_str(), payload.c_str());
}

void ESPWebConnect::checkMQTT()
{
    if (isAPMode())
        return;
    reconnectMQTT();
    mqttClient.loop();
}

bool ESPWebConnect::hasNewMQTTMsg() const
{
    return newMessage;
}

String ESPWebConnect::getMQTTMsg()
{
    newMessage = false;
    return latestMessage;
}

#endif

String ESPWebConnect::generateAllReadingsJSON()
{
    DynamicJsonDocument doc(1024); // Adjust size as needed for larger dashboards
    for (auto &element : dashboardElements)
    {
        String sensorId = String(element.id);
        sensorId.toLowerCase();
        sensorId += "-val";
        if (element.type == DashboardElement::SENSOR_INT)
        {
            doc[sensorId] = *element.intValue;
        }
        else if (element.type == DashboardElement::SENSOR_FLOAT)
        {
            doc[sensorId] = *element.floatValue;
        }
        else if (element.type == DashboardElement::SENSOR_STRING)
        {
            doc[sensorId] = String(*element.stringValue);
        }
    }
    String output;
    serializeJson(doc, output);
    return output;
}

String ESPWebConnect::generateDashboardHTML()
{
    String html = "<!DOCTYPE html><html><head><title>" + dashTitle + "</title>";
    html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";

    // Include FontAwesome for icons
    html += "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.2/css/all.min.css'>";

    html += "<script src='/dash.js'></script>";
    // Start loading the CSS as soon as DOM is ready
    html += "<script>";
    html += "document.addEventListener('DOMContentLoaded', () => {";
    html += "    if (!document.querySelector('link[href=\"/style.css\"]')) {";
    html += "        const link = document.createElement('link');";
    html += "        link.rel = 'stylesheet';";
    html += "        link.href = '/style.css';";
    html += "        document.head.appendChild(link);";
    html += "}});";

    // JS for reading update functionality
    html += "function updateReadings() {";
    html += "  fetch('/allReadings')"; // Fetch readings from the server
    html += "    .then(response => {";
    html += "      if (!response.ok) {";
    html += "        throw new Error('Network response was not ok: ' + response.statusText);";
    html += "      }";
    html += "      return response.json();"; // Parse JSON response
    html += "    })";
    html += "    .then(data => {";
    html += "      Object.keys(data).forEach(id => {";
    html += "        const element = document.getElementById(id);";
    html += "        if (element) {";
    html += "          if (element.type === 'checkbox') {";
    html += "            element.checked = data[id];";
    html += "          } else if (element.tagName === 'SPAN' || element.tagName === 'DIV') {";
    html += "            element.innerText = data[id];";
    html += "          }";
    html += "        }";
    html += "      });";
    html += "    })";
    html += "    .finally(() => {";
    html += "      setTimeout(updateReadings, " + String(updateInterval) + ");"; // Retry after interval
    html += "    });";
    html += "}";

    html += "</script></head><body>";

    // Banner Notification
    html += "<div id='notificationBanner' class='notification-banner'>";
    html += "<i id='notificationIcon' class='notification-icon'></i>";
    html += "<span id='notificationMessage' class='notification-message'></span>";
    html += "<div class='close-button-container'>";
    html += "<button id='closeButton' class='close-button' onclick='closeNotification()'>X</button>";
    html += "</div>";
    html += "</div>";

    // Navbar
    html += "<div id='navbar'>";
    html += "  <div style='display: flex; flex-direction: column; align-items: flex-start;'>";
    html += "    <div style='display: flex; align-items: center;'>";
    html += "    <img src=\"" + dashImageUrl + "\" alt='Icon' style='width: 48px; height: 48px; margin-right: 10px;'>";
    html += "      <h1>" +  dashTitle + "</h1>";
    html += "    </div>";
    html += "    <h2 style='margin: 5px 0; padding-left: 42px;'>" + dashDescription + "</h2>"; // Added padding to align with title
    html += "  </div>";
    html += "  <div style='display: flex; align-items: center; margin-top: 10px;'>";

    // Widget Dropdown
    html += "    <div class='dropdown' style='margin-right: 20px;'>";
    html += "      <button>Select Widget</button>";
    html += "      <div class='dropdown-content'>";

    // Dynamically generate dropdown items
    for (auto &element : dashboardElements)
    {
        String widgetType = getWidgetType(element.type);

        html += "<button data-widget='" + String(element.id) +
                "' data-name='" + String(element.name) +
                "' data-type='" + widgetType + "'";

        if (element.icon && strlen(element.icon) > 0)
        {
            html += " data-icon='" + String(element.icon) + "'";
        }

        if (element.color && strlen(element.color) > 0)
        {
            html += " data-color='" + String(element.color) + "'";
        }

        if (element.desc && strlen(element.desc) > 0)
        {
            html += " data-desc='" + String(element.desc) + "'";
        }

        if (element.unit && strlen(element.unit) > 0)
        {
            html += " data-unit='" + String(element.unit) + "'";
        }

        html += ">";
        html += String(element.name) + " [" + String(element.id) + "]";
        html += "</button>";
    }

    html += "      </div>";
    html += "    </div>";

    // Theme Selection Dropdown
    html += "    <div class='dropdown' style='margin-right: 20px;'>";
    html += "      <button>Theme</button>";
    html += "      <div class='dropdown-content'>";
    html += "        <button value='default' onclick='changeTheme(event)'>Keqing</button>";
    html += "        <button value='light' onclick='changeTheme(event)'>Light</button>";
    html += "        <button value='dark' onclick='changeTheme(event)'>Dark</button>";
    html += "      </div>";
    html += "    </div>";
    html += "    <div style='flex-grow: 0.9;'></div>"; // Spacer
    html += "    <button onclick='saveDashboard()' style='margin-right: 10px;'>Save</button>";
    html += "    <button onclick='clearDashboard()' style='margin-right: 10px;'>Clear</button>";
    html += "    <button onclick=\"window.location.href='/espwebc'\" style='margin-right: 10px;'>Go to ESP Web Config</button>";
    html += "  </div>";
    html += "</div>";

    // Dashboard container
    html += "<div id='dashboard'></div>";

    html += "</body></html>";
    return html;
}

String ESPWebConnect::getWidgetType(DashboardElement::Type type)
{
    switch (type)
    {
    case DashboardElement::SENSOR_INT:
    case DashboardElement::SENSOR_FLOAT:
    case DashboardElement::SENSOR_STRING:
        return "sensor";
    case DashboardElement::SWITCH:
        return "switch";
    case DashboardElement::BUTTON:
        return "button";
    case DashboardElement::INPUT_NUM:
        return "inputnumber";
    case DashboardElement::INPUT_TEXT:
        return "inputtext";
    default:
        return "unknown";
    }
}

bool ESPWebConnect::checkAuth(AsyncWebServerRequest *request)
{
    if (webSettings.Web_Lock)
    {
        if (!request->authenticate(webSettings.Web_User.c_str(), webSettings.Web_Pass.c_str()))
        {
            request->requestAuthentication();
            return false;
        }
    }
    return true;
}

void ESPWebConnect::handleButtonPress(AsyncWebServerRequest *request)
{
    if (!request->hasArg("id"))
    {
        request->send(400, "text/plain", "Bad Request: Missing button ID");
        return;
    }

    String id = request->arg("id");

    bool buttonFound = false;
    for (auto &element : dashboardElements)
    {
        if (strcmp(element.id, id.c_str()) == 0 && element.type == DashboardElement::BUTTON)
        { // Use strcmp to compare const char* with String's c_str()
            if (element.onPress)
            {
                element.onPress();
            }
            buttonFound = true;
            break;
        }
    }

    if (buttonFound)
    {
        request->send(200, "text/plain", "Button pressed");
    }
    else
    {
        request->send(404, "text/plain", "Button not found");
    }
}

void ESPWebConnect::handleReboot()
{
    delay(1000);
    ESP.restart();
}

bool ESPWebConnect::isAPMode() const
{
    return WiFi.getMode() & WIFI_AP;
}

bool ESPWebConnect::readWifiSettings(WifiSettings &settings)
{
    File file = LittleFS.open("/settings-wifi.json", "r");
    if (!file)
    {
        #ifdef ENABLE_DEBUG_INFO
        Serial.println("Failed to open WiFi settings file for reading");
        #endif
        return false;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error)
    {
        #ifdef ENABLE_DEBUG_INFO
        Serial.println("Failed to parse WiFi settings");
        #endif
        return false;
    }

    settings.SSID_Name = doc["SSID_Name"].as<String>();
    settings.SSID_Pass = doc["SSID_Pass"].as<String>();
    settings.ESP_MAC = doc["ESP_MAC"].as<String>();
    settings.SSID_AP_Name = doc["SSID_AP_Name"].as<String>();
    settings.SSID_AP_Pass = doc["SSID_AP_Pass"].as<String>();

    return true;
}

bool ESPWebConnect::readWebSettings(WebSettings &settings)
{
    File file = LittleFS.open("/settings-web.json", "r");
    if (!file)
    {
        #ifdef ENABLE_DEBUG_INFO
        Serial.println("Failed to open web settings file for reading");
        #endif
        return false;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error)
    {
        #ifdef ENABLE_DEBUG_INFO
        Serial.println("Failed to parse web settings");
        #endif
        return false;
    }

    settings.Web_User = doc["Web_User"].as<String>();
    settings.Web_Pass = doc["Web_Pass"].as<String>();
    settings.Web_name = doc["Web_name"].as<String>();
    settings.Web_Lock = doc["Web_Lock"].as<bool>();

    return true;
}

void ESPWebConnect::saveWifiSettings(const WifiSettings &settings)
{
    StaticJsonDocument<512> doc;
    doc["SSID_Name"] = settings.SSID_Name;
    doc["SSID_Pass"] = settings.SSID_Pass;
    doc["ESP_MAC"] = settings.ESP_MAC;
    doc["SSID_AP_Name"] = settings.SSID_AP_Name;
    doc["SSID_AP_Pass"] = settings.SSID_AP_Pass;

    File file = LittleFS.open("/settings-wifi.json", "w");
    if (!file)
    {
        #ifdef ENABLE_DEBUG_INFO
        Serial.println("Failed to open WiFi settings file for writing");
        #endif
        return;
    }

    serializeJson(doc, file);
    file.close();
}

#ifdef ENABLE_MQTT

bool ESPWebConnect::readMQTTSettings(MQTTSettings &settings)
{
    File file = LittleFS.open("/settings-mqtt.json", "r");
    if (!file)
    {
        Serial.println("Failed to open MQTT settings file for reading");
        return false;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error)
    {
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