#include "ESPWebConnect.h"
ESPWebConnect webConnect;
bool relay1 = true; //16
bool relay2 = true; //17
float count = 0;
int numbers = 0;
String text;
unsigned long preTime = 0;

void setup() {
    Serial.begin(115200);
    pinMode(16, OUTPUT);
    pinMode(17, OUTPUT);
    digitalWrite(16, relay1 ? HIGH : LOW);
    digitalWrite(17, relay2 ? HIGH : LOW);
    webConnect.begin();
    webConnect.setIconUrl("https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.2/css/all.min.css");
    webConnect.setAutoUpdate(2500);
    webConnect.setDashInfo("Smart Home", "basic control", "https://danielamani.com/image/SmartHome.png", "");
    webConnect.setManifactureInfo("EDNA", "Smart Home", "Basic", "V0.1.7");
    webConnect.addSwitch("relay-1", "Switch 1","Light", "fa-regular fa-lightbulb", &relay1);
    webConnect.setIconColor("relay-1", "#D3D876");
    webConnect.addSwitch("relay-2", "Switch 2", "Fan", "fa-solid fa-fan", &relay2); 
    webConnect.addSensor("counter", "Counter", "", "fa fa-infinity", &count, "++");
    webConnect.addButton("btn1", "Press Me","", "fa fa-hand-pointer", onButtonPress);
    webConnect.addInputText("input-text", "Enter text:", "","fa-solid fa-pencil", &text);
    webConnect.addInputNum("input-num", "Enter number:", "","fa-solid fa-pencil",  &count);
}

void loop() {
    digitalWrite(16, relay1 ? HIGH : LOW);
    digitalWrite(17, relay2 ? HIGH : LOW);
    unsigned int taskDelay = 5000;
        if((millis()-preTime)>taskDelay){
           updateCount();
      preTime=millis();
    }
}

void onButtonPress() {
  Serial.println("Button is press and show stored text");
  text = "Text:" + text;
  webConnect.sendNotification("textNoti", text, "white", "fa-regular fa-envelope", "blue", 10);
}

float updateCount() {
    count++;
    if (count > 50) {
        count = 0;
        webConnect.sendNotification("countNoti", "Reset Count", "white", "fa-regular fa-envelope", "black", 10);
    }
    return count;
}