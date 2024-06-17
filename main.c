/**
built to function in a esp32 dev board

in boards select *esp32* by Espressif
in library select *PubSubClient* by Nick
in library select *Preferences* by Espressif
in library select *SSD1306* by Adafruit

**/

char temp[1024]; // Temp buffer for HTTP requests and Display
char apSSID[20];
char apPassword[20];

//// SSD1306 
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setupDisplay() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  delay(2000);
  display.clearDisplay();
}

void printDisplay(char* text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(text);
  display.display();
}


//Generic ESP32 board Related Stuff
String generateRandomChars(int length) {
  String chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
  String randomString = "";
  for (int i = 0; i < length; i++) {
    randomString += chars[random(0, chars.length())];
  }
  return randomString;
}


// Load Save Variables
#include <Preferences.h>
Preferences preferences;

//// WIFI Module Related Stuff
#include <WiFi.h>
WiFiClient espClient;

void setupWiFi() {
  // Get wifi ssid and password from preferences
  String ssid = preferences.getString("wifi_ssid", "");             // If no ssid stored, return empty string
  String password = preferences.getString("wifi_password", "");  // If no password stored, return empty string
  // String ssid = preferences.getString("wifi_ssid", "Local_neT");             // If no ssid stored, return empty string
  // String password = preferences.getString("wifi_password", "C@net@deOuro");  // If no password stored, return empty string


  if (ssid.equals("") || password.equals("")) {
    // No SSID or password, create an access point
    sprintf(apSSID, "ESP32_AP%s", generateRandomChars(8).c_str());
    sprintf(apPassword, "%s", generateRandomChars(8).c_str());
    WiFi.softAP(apSSID, apPassword);
    // Serial.println("Access point created");
  } else {
    // Connect to WiFi
    WiFi.begin(ssid.c_str(), password.c_str());
    // Serial.println("Connecting to WiFi...");
    // int tries = 0;
    // while (WiFi.status() != WL_CONNECTED && tries < 10) {
    //   delay(500);
    //   tries += 1;
    // }
    // Serial.println("Connected to WiFi");
  }
}

//// mDNS Module Related Stuff
#include <ESPmDNS.h>
void setupmDNS() {
  // Set up mDNS responder
  if (!MDNS.begin("esp32")) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started");
    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);
  }
}


//// MQTT Client Related Stuff
#include <PubSubClient.h>
PubSubClient MQTT(espClient);
void setupMQTT() {
  // Get MQTT server and port from preferences
  String mqtt_Server = preferences.getString("mqtt_Server", "");  // If no server stored, return empty string
  int mqtt_Port = preferences.getInt("mqtt_Port", 1883);          // If no port stored, return 1883

  //// Connect to MQTT Server
  MQTT.setServer(mqtt_Server.c_str(), mqtt_Port);
  // while (!MQTT.connected()) {
  //   Serial.println("Connecting to MQTT...");
  //   if (MQTT.connect("ESP32Client")) {
  //     Serial.println("Connected to MQTT");
  //   } else {
  //     Serial.print("failed with state ");
  //     Serial.print(MQTT.state());
  //     delay(2000);
  //   }
  // }
}

//// HTTP Server Related Stuff
#include <WebServer.h>
WebServer HTTP(80);

void HTTP_handleRoot() {
  int sec = millis() / 1000;
  int hr = sec / 3600;
  int min = (sec / 60) % 60;
  sec = sec % 60;

  snprintf(temp, 800,
           "<html><head><meta http-equiv='refresh' content='5'/><title>ESP32 Demo</title></head><body><p>Uptime: %02d:%02d:%02d</p><form method='POST' action='/savecfg'>WiFi SSID:<input type='text' name='ssid'><br>WiFi Password:<input type='text' name='password'><br>MQTT Server:<input type='text' name='mqttServer'><br>MQTT Port:<input type='text' name='mqttPort'><br><input type='submit' value='Save'></form><img src=\"/test.svg\" /></body></html>",
           hr, min, sec);

  HTTP.send(200, "text/html", temp);
}
void HTTP_handleNotFound() {
  // digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += HTTP.uri();
  message += "\nMethod: ";
  message += (HTTP.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += HTTP.args();
  message += "\n";

  for (uint8_t i = 0; i < HTTP.args(); i++) {
    message += " " + HTTP.argName(i) + ": " + HTTP.arg(i) + "\n";
  }

  HTTP.send(404, "text/plain", message);
  // digitalWrite(led, 0);
}
void HTTP_saveCfg() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += HTTP.uri();
  message += "\nMethod: ";
  message += (HTTP.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += HTTP.args();
  message += "\n";

  for (uint8_t i = 0; i < HTTP.args(); i++) {
    message += " " + HTTP.argName(i) + ": " + HTTP.arg(i) + "\n";
  }

  HTTP.send(404, "text/plain", message);
  // digitalWrite(led, 0);
}

void setupHTTP() {
  HTTP.on("/", HTTP_handleRoot);
  HTTP.on("/saveCfg", HTTP_saveCfg);
  HTTP.on("/inline", []() {
    HTTP.send(200, "text/plain", "this works as well");
  });
  HTTP.onNotFound(HTTP_handleNotFound);
  HTTP.begin();
}


//// the ESP32 stuff
void setup() {
  Serial.begin(115200);

  // Open Preferences with my-app namespace.
  preferences.begin("app", false);

  setupDisplay();
  setupWiFi();
  setupmDNS();
  setupMQTT();
  setupHTTP();
}

void loop() {
  MQTT.loop();
  HTTP.handleClient();

  //TODO: MQTT.connected()
  //TODO: WiFi.status() TIMEOUT // not connected for 60+sec
  //TODO: Display Update with data TODO above

  snprintf(temp, 1024,
           "Uptime: %02d:%02d:%02d\nWifi: %02d\nMQTT: %02d\n",
           hr, min, sec, WiFi.status(), MQTT.connected());
// apSSID
// apPassword


  printDisplay( temp );
  
  delay(2);  //allow the cpu to switch to other tasks
}
