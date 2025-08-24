#define DEBUG
#include <ESP8266WiFi.h>

#include "Config.h"
#include "Sensors.h"
#include "WebServer.h"
#include "Sender.h"

DeviceConfig config;
Adafruit_BME280 bme;
SoftwareSerial pms(D1, D4); // RX=D1, TX=D4 (unused)
ESP8266WebServer server(80);
DNSServer dns;
bool shouldRestart = false;

unsigned long lastSend = 0;

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
}

void loop() {
  handleWeb();
  if (WiFi.status() == WL_CONNECTED && millis() - lastSend > 60000) {
    uint16_t pm; float t, h, p;
    readMeasurements(pm, t, h, p, config);
    DBG_PRINT("Measurements - PM2.5: ");
    DBG_PRINT(pm);
    DBG_PRINT(", T: ");
    DBG_PRINT(t);
    DBG_PRINT(", H: ");
    DBG_PRINT(h);
    DBG_PRINT(", P: ");
    DBG_PRINTLN(p);
    sendToNodeRed(pm, t, h, p, config);
    lastSend = millis();
  }
  if (shouldRestart) {
    delay(5000);

    ESP.restart();
  }
}

