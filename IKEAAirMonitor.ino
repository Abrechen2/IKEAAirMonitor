#define DEBUG
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "Config.h"
#include "Sensors.h"
#include "WebServer.h"
#include "Sender.h"

DeviceConfig config;
Adafruit_BME280 bme;
// Use D1 for the Vindriktning's TX line (Vindriktning TX -> D1 RX)
SoftwareSerial pms(D1, D8); // RX=D1, TX=D8 (unused)
ESP8266WebServer server(80);
DNSServer dns;
bool shouldRestart = false;

unsigned long lastSend = 0;
unsigned long lastMillis = 0;
unsigned long long uptimeMillis = 0;

void setup() {
  Serial.begin(115200);
  DBG_PRINTLN("Booting IKEAAirMonitor");

  resetConfig(config);
  bool cfgLoaded = loadConfig(config);
  if (cfgLoaded) {
    DBG_PRINTLN("Config loaded from flash");
  } else {
    DBG_PRINTLN("Using default config");
  }

  if (config.ssid[0] != '\0') {
    WiFi.mode(WIFI_STA);
    WiFi.hostname(config.hostname);
    DBG_PRINT("Connecting to ");
    DBG_PRINTLN(config.ssid);
    WiFi.begin(config.ssid, config.password);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
      delay(500);
      DBG_PRINT('.');
    }
    DBG_PRINTLN("");
    if (WiFi.status() == WL_CONNECTED) {
      DBG_PRINT("Connected, IP: ");
      DBG_PRINTLN(WiFi.localIP());
      if (config.nodeHost[0] != '\0') {
        WiFiClient client;
        HTTPClient http;
        String url = String("http://") + config.nodeHost + ":" + config.nodePort + "/";
        bool ok = false;
        if (http.begin(client, url)) {
          int code = http.GET();
          http.end();
          ok = code > 0;
        }
        if (ok) {
          DBG_PRINTLN("Node-RED reachable");
        } else {
          DBG_PRINTLN("Node-RED not reachable");
        }
      }
      setupWeb();
    } else {
      DBG_PRINTLN("WiFi not reachable, starting AP");
      startAP();
    }
  } else {
    DBG_PRINTLN("No WiFi configured, starting AP");
    startAP();
  }

  if (initSensors()) {
    DBG_PRINTLN("Sensors initialized");
  } else {
    DBG_PRINTLN("Failed to init sensors");
  }
  
  // Give Vindriktning time to start sending data
  DBG_PRINTLN("Waiting for Vindriktning to send data...");
  delay(2000);
  lastMillis = millis();
}

void loop() {
  unsigned long now = millis();
  uptimeMillis += (unsigned long)(now - lastMillis);
  lastMillis = now;

  handleWeb();
  
  if (WiFi.status() == WL_CONNECTED && millis() - lastSend > config.sendInterval) {
    uint16_t pm; float t, h, p;
    
    DBG_PRINTLN("Reading measurements...");
    readMeasurements(pm, t, h, p, config);
    
    DBG_PRINT("Measurements - PM2.5: ");
    DBG_PRINT(pm);
    DBG_PRINT(" µg/m³, T: ");
    DBG_PRINT(t);
    DBG_PRINT("°C, H: ");
    DBG_PRINT(h);
    DBG_PRINT("%, P: ");
    DBG_PRINT(p);
    DBG_PRINTLN(" hPa");
    
    if (pm > 0 || t > -40) { // Only send if we have valid data
      sendToNodeRed(pm, t, h, p, uptimeMillis / 1000, config);
    } else {
      DBG_PRINTLN("No valid sensor data, skipping send");
    }
    
    lastSend = millis();
  }
  
  if (shouldRestart) {
    delay(5000);
    ESP.restart();
  }
}