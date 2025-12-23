#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>

#include "Config.h"
#include "Sensors.h"
#include "WebServer.h"
#include "MQTTManager.h"
#include "Calculations.h"

DeviceConfig config;
Adafruit_BME280 bme;
// Use D1 for the Vindriktning's TX line (Vindriktning TX -> D1 RX)
SoftwareSerial pms(D1, D8); // RX=D1, TX=D8 (unused)
ESP8266WebServer server(80);
DNSServer dns;
bool shouldRestart = false;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
bool mqttConnected = false;

unsigned long lastSend = 0;
unsigned long lastMillis = 0;
unsigned long uptimeMillis = 0;

// MQTT state variables (moved from MQTTManager.h to avoid ODR violations)
unsigned long lastMQTTReconnect = 0;
const unsigned long MQTT_RECONNECT_INTERVAL = 5000;
bool discoveryPublished = false;
bool pendingDataSend = false;
bool firstDataSent = false;
uint16_t lastPM25 = 0;
float lastTemp = 0;
float lastHumidity = 0;
float lastPressure = 0;
uint16_t lastAQI = 0;
uint8_t lastAQICategory = 0;
float lastDewPoint = 0;
float lastComfortIndex = 0;
uint32_t lastUptime = 0;

// MQTT topic variables
char deviceUniqueId[32] = "";
char baseTopic[96] = "";
char discoveryPrefix[96] = "";
bool mqttTopicsInitialized = false;

void setup() {
  Serial.begin(115200);
  DBG_PRINTLN("Booting IKEAAirMonitor");

  resetConfig(config);
  bool cfgLoaded = loadConfig(config);
  if (cfgLoaded) {
    DBG_PRINTLN("Config loaded from flash");
  } else {
    DBG_PRINTLN("Using default config");
    saveConfig(config);
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
      initMQTT();
      
      // Setup OTA
      ArduinoOTA.setHostname(config.hostname);
      ArduinoOTA.setPassword(DEFAULT_OTA_PASSWORD);
      
      ArduinoOTA.onStart([]() {
        DBG_PRINTLN("OTA Start");
      });
      ArduinoOTA.onEnd([]() {
        DBG_PRINTLN("\nOTA End");
      });
      ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        DBG_PRINTF("Progress: %u%%\r", (progress / (total / 100)));
      });
      ArduinoOTA.onError([](ota_error_t error) {
        DBG_PRINTF("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
          DBG_PRINTLN("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
          DBG_PRINTLN("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
          DBG_PRINTLN("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
          DBG_PRINTLN("Receive Failed");
        } else if (error == OTA_END_ERROR) {
          DBG_PRINTLN("End Failed");
        }
      });
      
      ArduinoOTA.begin();
      DBG_PRINTLN("OTA ready");
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
  loopMQTT();
  
  // Handle OTA updates (only when WiFi is connected)
  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.handle();
  }
  
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
      // Calculate derived values
      uint16_t aqi = calculatePM25AQI(pm);
      uint8_t aqiCategory = getAQICategory(aqi);
      float dewPoint = calculateDewPoint(t, h);
      float comfortIndex = calculateComfortIndex(t, h);
      uint32_t uptime = uptimeMillis / 1000;
      
      DBG_PRINT("Calculated - AQI: ");
      DBG_PRINT(aqi);
      DBG_PRINT(", Category: ");
      DBG_PRINT(aqiCategory);
      DBG_PRINT(", Dew Point: ");
      DBG_PRINT(dewPoint, 1);
      DBG_PRINT("°C, Comfort: ");
      DBG_PRINT(comfortIndex, 1);
      DBG_PRINTLN();
      
      publishSensorData(pm, t, h, p, aqi, aqiCategory, dewPoint, comfortIndex, uptime);
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