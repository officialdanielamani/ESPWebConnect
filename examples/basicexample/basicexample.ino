#include "ESPWebConnect.h"

ESPWebConnect webConnect;

bool relay1 = true; //16
bool relay2 = true; //17
float count = 0;

void setup() {
    Serial.begin(115200);

    pinMode(16, OUTPUT);
    pinMode(17, OUTPUT);

    digitalWrite(16, relay1 ? HIGH : LOW);
    digitalWrite(17, relay2 ? HIGH : LOW);

    webConnect.begin();
    webConnect.setIconUrl("https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.2/css/all.min.css");
    webConnect.setCSS("style.css");
    webConnect.setAutoUpdate(2500);
    webConnect.addSwitch("relay-1", "Relay-1", "fa-regular fa-lightbulb", &relay1);
    webConnect.setIconColor("relay-1", "#D3D876");
    webConnect.addSwitch("relay-2", "Relay-2", "fa-solid fa-fan", &relay2); 
    webConnect.addSensor("counter", "Counter", "", "fa fa-infinity", updateCount);
    webConnect.addButton("btn1", "Press Me", "fa fa-hand-pointer", true, onButtonPress);
}

void loop() {
    digitalWrite(16, relay1 ? HIGH : LOW);
    digitalWrite(17, relay2 ? HIGH : LOW);
}

void onButtonPress() {
  Serial.println("Button is press");
  webConnect.sendNotification("noti-count", "Button is pressed" , "white", "fa-regular fa-envelope", "white", 10);
}

float updateCount() {
    count++;
    if (count > 50) {
        count = 0;
        webConnect.sendNotification("noti-count", "~ Reset Counter" , "white", "fa-regular fa-envelope", "white", 10);
    }
    return count;
}
