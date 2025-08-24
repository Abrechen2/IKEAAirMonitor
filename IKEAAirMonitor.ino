#include <ESP8266WiFi.h>

#include "Config.h"
#include "Sensors.h"
#include "WebServer.h"
#include "Sender.h"

Config config;
Adafruit_BME280 bme;
SoftwareSerial pms(D1, D4); // RX=D1, TX=D4 (unused)
ESP8266WebServer server(80);
DNSServer dns;
bool shouldRestart = false;

unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);

  resetConfig(config);
  if (!loadConfig(config)) {
    startAP();
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.hostname(config.hostname);
    WiFi.begin(config.ssid, config.password);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
      delay(500);
    }
    if (WiFi.status() != WL_CONNECTED) {
      startAP();
    } else {
      setupWeb();
    }
  }
  initSensors();
}

void loop() {
  handleWeb();
  if (WiFi.status() == WL_CONNECTED && millis() - lastSend > 60000) {
    uint16_t pm; float t, h, p;
    readMeasurements(pm, t, h, p, config);
    sendToNodeRed(pm, t, h, p, config);
    lastSend = millis();
  }
  if (shouldRestart) {
    delay(5000);

    ESP.restart();
  }
}

