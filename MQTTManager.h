#pragma once
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "Config.h"

extern WiFiClient wifiClient;
extern DeviceConfig config;
extern PubSubClient mqttClient;
extern bool mqttConnected;

extern unsigned long lastMQTTReconnect;
extern const unsigned long MQTT_RECONNECT_INTERVAL;
extern unsigned long lastStatusHeartbeat;
extern const unsigned long STATUS_HEARTBEAT_INTERVAL;
extern bool discoveryPublished;
extern bool pendingDataSend;
extern bool firstDataSent;
extern uint16_t lastPM25;
extern float lastTemp;
extern float lastHumidity;
extern float lastPressure;
extern uint16_t lastAQI;
extern uint8_t lastAQICategory;
extern float lastDewPoint;
extern float lastComfortIndex;
extern uint32_t lastUptime;

// Variables for device identification and topics
extern char deviceUniqueId[32];
extern char baseTopic[96];
extern char discoveryPrefix[96];
extern bool mqttTopicsInitialized;

// Generate unique device ID from MAC address and initialize topics
inline void initMQTTTopics() {
  if (mqttTopicsInitialized) {
    return;
  }
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(deviceUniqueId, sizeof(deviceUniqueId), "%02x%02x%02x%02x%02x%02x", 
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  // Use config.mqttTopic as base, or fallback to device ID
  if (config.mqttTopic[0] != '\0') {
    snprintf(baseTopic, sizeof(baseTopic), "%s", config.mqttTopic);
  } else {
    snprintf(baseTopic, sizeof(baseTopic), "ikea_air_monitor/%s", deviceUniqueId);
  }
  
  snprintf(discoveryPrefix, sizeof(discoveryPrefix), "homeassistant/sensor/ikea_air_monitor_%s", deviceUniqueId);
  
  mqttTopicsInitialized = true;
  
  DBG_PRINT("MQTT Topics initialized - Device ID: ");
  DBG_PRINT(deviceUniqueId);
  DBG_PRINT(", Base Topic: ");
  DBG_PRINT(baseTopic);
  DBG_PRINT(", Discovery Prefix: ");
  DBG_PRINTLN(discoveryPrefix);
}

// Create device info JSON string
inline void createDeviceInfo(char* buffer, size_t len) {
  // Use hostname from config, fallback to default if empty
  const char* deviceName = (config.hostname[0] != '\0') ? config.hostname : "IKEA Air Monitor";
  snprintf(buffer, len,
    "{"
    "\"identifiers\":[\"ikea_air_monitor_%s\"],"
    "\"name\":\"%s\","
    "\"model\":\"IKEA Air Monitor\","
    "\"manufacturer\":\"DIY\","
    "\"sw_version\":\"1.0\""
    "}",
    deviceUniqueId,
    deviceName
  );
}

// Publish Home Assistant Discovery configuration for a sensor
inline void publishDiscoverySensor(const char* sensorName, const char* sensorId, 
                                   const char* unit, const char* deviceClass, 
                                   const char* valueTemplate, 
                                   const char* stateClass = nullptr,
                                   const char* icon = nullptr) {
  if (!mqttClient.connected()) {
    DBG_PRINTLN("MQTT not connected, cannot publish discovery");
    return;
  }
  
  if (!mqttTopicsInitialized) {
    initMQTTTopics();
  }
  
  char topic[192];
  snprintf(topic, sizeof(topic), "%s/%s/config", discoveryPrefix, sensorId);
  
  char deviceInfo[192];
  createDeviceInfo(deviceInfo, sizeof(deviceInfo));
  
  // Build value template - use provided or default
  char valueTemplateStr[96];
  if (valueTemplate && strlen(valueTemplate) > 0) {
    snprintf(valueTemplateStr, sizeof(valueTemplateStr), "{{ value_json.%s }}", valueTemplate);
  } else {
    snprintf(valueTemplateStr, sizeof(valueTemplateStr), "{{ value_json.%s }}", sensorId);
  }
  
  // Build state topic
  char stateTopic[96];
  snprintf(stateTopic, sizeof(stateTopic), "tele/%s/state", baseTopic);
  
  // Build availability topic
  char availabilityTopic[96];
  snprintf(availabilityTopic, sizeof(availabilityTopic), "tele/%s/status", baseTopic);
  
  // Build unique_id
  char uniqueId[96];
  snprintf(uniqueId, sizeof(uniqueId), "ikea_air_monitor_%s_%s", deviceUniqueId, sensorId);
  
  char payload[768];
  int len = snprintf(payload, sizeof(payload),
    "{"
    "\"name\":\"%s\","
    "\"unique_id\":\"%s\","
    "\"state_topic\":\"%s\","
    "\"value_template\":\"%s\","
    "\"availability_topic\":\"%s\","
    "\"payload_available\":\"online\","
    "\"payload_not_available\":\"offline\","
    "\"device\":%s",
    sensorName,
    uniqueId,
    stateTopic,
    valueTemplateStr,
    availabilityTopic,
    deviceInfo
  );
  
  // Add unit_of_measurement if provided
  if (unit && strlen(unit) > 0) {
    int newLen = snprintf(payload + len, sizeof(payload) - len, ",\"unit_of_measurement\":\"%s\"", unit);
    if (newLen > 0 && len + newLen < (int)sizeof(payload)) {
      len += newLen;
    }
  }
  
  // Add device_class if provided
  if (deviceClass && strlen(deviceClass) > 0) {
    int newLen = snprintf(payload + len, sizeof(payload) - len, ",\"device_class\":\"%s\"", deviceClass);
    if (newLen > 0 && len + newLen < (int)sizeof(payload)) {
      len += newLen;
    }
  }
  
  // Add state_class if provided (for statistical sensors like uptime)
  if (stateClass && strlen(stateClass) > 0) {
    int newLen = snprintf(payload + len, sizeof(payload) - len, ",\"state_class\":\"%s\"", stateClass);
    if (newLen > 0 && len + newLen < (int)sizeof(payload)) {
      len += newLen;
    }
  }
  
  // Add icon if provided
  if (icon && strlen(icon) > 0) {
    int newLen = snprintf(payload + len, sizeof(payload) - len, ",\"icon\":\"%s\"", icon);
    if (newLen > 0 && len + newLen < (int)sizeof(payload)) {
      len += newLen;
    }
  }
  
  // Add expire_after for better offline detection (120 seconds = 2x heartbeat interval)
  int newLen = snprintf(payload + len, sizeof(payload) - len, ",\"expire_after\":120");
  if (newLen > 0 && len + newLen < (int)sizeof(payload)) {
    len += newLen;
  }
  
  // Close JSON
  if (len + 2 < (int)sizeof(payload)) {
    int newLen = snprintf(payload + len, sizeof(payload) - len, "}");
    if (newLen > 0) {
      len += newLen;
    }
  }
  
  // Debug: Print discovery payload
  DBG_PRINT("Discovery payload for ");
  DBG_PRINT(sensorId);
  DBG_PRINT(" -> ");
  DBG_PRINT(topic);
  DBG_PRINT(": ");
  DBG_PRINTLN(payload);
  
  bool published = mqttClient.publish(topic, payload, true); // retain = true
  if (published) {
    DBG_PRINT("Published discovery for sensor: ");
    DBG_PRINT(sensorId);
    DBG_PRINT(" -> ");
    DBG_PRINTLN(topic);
  } else {
    DBG_PRINT("Failed to publish discovery for sensor: ");
    DBG_PRINT(sensorId);
    DBG_PRINT(", topic: ");
    DBG_PRINTLN(topic);
  }
}

// Publish all Home Assistant Discovery configurations
inline void publishDiscovery() {
  if (discoveryPublished) {
    DBG_PRINTLN("Discovery already published, skipping");
    return;
  }
  
  if (!mqttClient.connected()) {
    DBG_PRINTLN("MQTT not connected, skipping discovery");
    return;
  }
  
  if (!mqttTopicsInitialized) {
    initMQTTTopics();
  }
  
  DBG_PRINTLN("Publishing Home Assistant discovery configuration...");
  
  // PM2.5 sensor
  publishDiscoverySensor("PM2.5", "pm25", "µg/m³", "pm25", "pm25", "measurement", "mdi:air-filter");
  
  // Temperature sensor
  publishDiscoverySensor("Temperature", "temperature", "°C", "temperature", "temperature", "measurement", "mdi:thermometer");
  
  // Humidity sensor
  publishDiscoverySensor("Humidity", "humidity", "%", "humidity", "humidity", "measurement", "mdi:water-percent");
  
  // Pressure sensor
  publishDiscoverySensor("Pressure", "pressure", "hPa", "pressure", "pressure", "measurement", "mdi:gauge");
  
  // AQI sensor
  publishDiscoverySensor("AQI", "aqi", "", "aqi", "aqi", "measurement", "mdi:air-purifier");
  
  // AQI Category sensor
  publishDiscoverySensor("AQI Category", "aqi_category", "", "", "aqi_category", nullptr, "mdi:signal");
  
  // Dew Point sensor
  publishDiscoverySensor("Dew Point", "dew_point", "°C", "temperature", "dew_point", "measurement", "mdi:water-thermometer");
  
  // Comfort Index sensor
  publishDiscoverySensor("Comfort Index", "comfort_index", "", "", "comfort_index", "measurement", "mdi:emoticon-happy");
  
  // Uptime sensor (total_increasing for statistics)
  publishDiscoverySensor("Uptime", "uptime", "s", "duration", "uptime", "total_increasing", "mdi:timer-outline");
  
  discoveryPublished = true;
  DBG_PRINTLN("Home Assistant discovery configuration published");
}

// Forward declaration
inline void publishAvailability(bool online);

// Publish sensor data as JSON
inline void publishSensorData(uint16_t pm25, float temp, float h, float p, 
                              uint16_t aqi, uint8_t aqiCategory, 
                              float dewPoint, float comfortIndex, 
                              uint32_t uptime) {
  // Store data for retry if connection fails
  lastPM25 = pm25;
  lastTemp = temp;
  lastHumidity = h;
  lastPressure = p;
  lastAQI = aqi;
  lastAQICategory = aqiCategory;
  lastDewPoint = dewPoint;
  lastComfortIndex = comfortIndex;
  lastUptime = uptime;
  
  if (!mqttClient.connected()) {
    pendingDataSend = true;
    DBG_PRINTLN("MQTT not connected, data will be sent when connection is restored");
    return;
  }
  
  pendingDataSend = false;
  
  if (!mqttTopicsInitialized) {
    initMQTTTopics();
  }
  
  char stateTopic[96];
  snprintf(stateTopic, sizeof(stateTopic), "tele/%s/state", baseTopic);
  
  char payload[384];
  int len = snprintf(payload, sizeof(payload),
    "{"
    "\"pm25\":%u,"
    "\"temperature\":%.1f,"
    "\"humidity\":%.1f,"
    "\"pressure\":%.2f,"
    "\"aqi\":%u,"
    "\"aqi_category\":%u,"
    "\"dew_point\":%.1f,"
    "\"comfort_index\":%.1f,"
    "\"uptime\":%lu"
    "}",
    pm25, temp, h, p, aqi, aqiCategory, dewPoint, comfortIndex, uptime
  );
  
  // Validate JSON format - ensure it's properly closed
  if (len <= 0 || len >= (int)sizeof(payload)) {
    DBG_PRINT("ERROR: JSON payload generation failed or too large. len=");
    DBG_PRINT(len);
    DBG_PRINT(", max=");
    DBG_PRINTLN(sizeof(payload));
    return;
  }
  
  // Verify JSON is properly closed (should end with })
  if (len > 0 && len < (int)sizeof(payload)) {
    if (payload[len-1] != '}') {
      DBG_PRINTLN("ERROR: JSON payload not properly closed!");
      return;
    }
    DBG_PRINT("Publishing to ");
    DBG_PRINT(stateTopic);
    DBG_PRINT(": ");
    DBG_PRINTLN(payload);
    // Send first message with retain=true so Home Assistant picks it up immediately
    bool retainFlag = !firstDataSent;
    bool published = mqttClient.publish(stateTopic, payload, retainFlag);
    if (published) {
      if (retainFlag) {
        firstDataSent = true;
        DBG_PRINT("MQTT data published to ");
        DBG_PRINT(stateTopic);
        DBG_PRINTLN(" (retained)");
      } else {
        DBG_PRINT("MQTT data published to ");
        DBG_PRINTLN(stateTopic);
      }
      // Update availability topic to ensure Home Assistant knows device is online
      publishAvailability(true);
    } else {
      DBG_PRINTLN("Failed to publish MQTT data");
    }
  } else {
    DBG_PRINT("Payload too large: ");
    DBG_PRINT(len);
    DBG_PRINTLN(" bytes");
  }
  
  // Also publish in Tasmota format (tele/XXX/SENSOR)
  char tasmotaTopic[96];
  snprintf(tasmotaTopic, sizeof(tasmotaTopic), "tele/%s/SENSOR", baseTopic);
  
  // Tasmota format with Time and sensor data
  char tasmotaPayload[384];
  // Get current time (seconds since boot, formatted as uptime)
  int tasmotaLen = snprintf(tasmotaPayload, sizeof(tasmotaPayload),
    "{"
    "\"Time\":\"%lu\","
    "\"BME280\":{"
    "\"Temperature\":%.1f,"
    "\"Humidity\":%.1f,"
    "\"Pressure\":%.2f"
    "},"
    "\"PM2.5\":{"
    "\"PM2.5\":%u"
    "},"
    "\"AQI\":%u,"
    "\"AQICategory\":%u,"
    "\"DewPoint\":%.1f,"
    "\"ComfortIndex\":%.1f,"
    "\"Uptime\":%lu"
    "}",
    uptime, temp, h, p, pm25, aqi, aqiCategory, dewPoint, comfortIndex, uptime
  );
  
  // Validate Tasmota JSON format
  if (tasmotaLen <= 0 || tasmotaLen >= (int)sizeof(tasmotaPayload)) {
    DBG_PRINT("ERROR: Tasmota JSON payload generation failed or too large. len=");
    DBG_PRINT(tasmotaLen);
    DBG_PRINT(", max=");
    DBG_PRINTLN(sizeof(tasmotaPayload));
    return;
  }
  
  // Verify JSON is properly closed (should end with })
  if (tasmotaLen > 0 && tasmotaLen < (int)sizeof(tasmotaPayload)) {
    if (tasmotaPayload[tasmotaLen-1] != '}') {
      DBG_PRINTLN("ERROR: Tasmota JSON payload not properly closed!");
      return;
    }
    bool tasmotaPublished = mqttClient.publish(tasmotaTopic, tasmotaPayload, false);
    if (tasmotaPublished) {
      DBG_PRINT("MQTT data published to Tasmota format: ");
      DBG_PRINTLN(tasmotaTopic);
    } else {
      DBG_PRINTLN("Failed to publish MQTT data in Tasmota format");
    }
  } else {
    DBG_PRINT("Tasmota payload too large: ");
    DBG_PRINT(tasmotaLen);
    DBG_PRINTLN(" bytes");
  }
}

// Publish availability status
inline void publishAvailability(bool online) {
  if (!mqttClient.connected()) {
    return;
  }
  
  if (!mqttTopicsInitialized) {
    initMQTTTopics();
  }
  
  char statusTopic[96];
  snprintf(statusTopic, sizeof(statusTopic), "tele/%s/status", baseTopic);
  
  const char* status = online ? "online" : "offline";
  bool published = mqttClient.publish(statusTopic, status, true); // retain = true
  if (published) {
    DBG_PRINT("Published availability: ");
    DBG_PRINTLN(status);
  } else {
    DBG_PRINTLN("Failed to publish availability");
  }
}

// MQTT callback (not used but required by PubSubClient)
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // No subscriptions needed for this device
}

// Connect to MQTT broker
inline bool connectMQTT() {
  if (config.mqttHost[0] == '\0') {
    DBG_PRINTLN("MQTT host not configured");
    return false;
  }
  
  // Check WiFi connection first
  if (WiFi.status() != WL_CONNECTED) {
    DBG_PRINTLN("WiFi not connected, cannot connect to MQTT");
    return false;
  }
  
  mqttClient.setServer(config.mqttHost, config.mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(1024); // Increase buffer size for discovery messages
  mqttClient.setKeepAlive(60); // Set keepalive to 60 seconds
  
  if (!mqttTopicsInitialized) {
    initMQTTTopics();
  }
  
  // Use stable client ID (without millis) for better reconnection
  char clientId[80];
  snprintf(clientId, sizeof(clientId), "ikea_air_monitor_%s", deviceUniqueId);
  
  // Prepare Last Will Testament (LWT) - sends "offline" if connection is lost unexpectedly
  char willTopic[96];
  snprintf(willTopic, sizeof(willTopic), "tele/%s/status", baseTopic);
  const char* willMessage = "offline";
  
  DBG_PRINT("Connecting to MQTT broker ");
  DBG_PRINT(config.mqttHost);
  DBG_PRINT(":");
  DBG_PRINT(config.mqttPort);
  DBG_PRINT(" as ");
  DBG_PRINTLN(clientId);
  
  // Try to connect with timeout
  bool connected = false;
  unsigned long connectStart = millis();
  const unsigned long CONNECT_TIMEOUT = 5000; // 5 seconds timeout
  
  while (!connected && (millis() - connectStart < CONNECT_TIMEOUT)) {
    if (config.mqttUser[0] != '\0') {
      // Connect with LWT: willTopic, willQoS=1, willRetain=true, willMessage
      connected = mqttClient.connect(clientId, config.mqttUser, config.mqttPassword, 
                                     willTopic, 1, true, willMessage);
    } else {
      // Connect with LWT but without credentials
      connected = mqttClient.connect(clientId, willTopic, 1, true, willMessage);
    }
    
    if (!connected) {
      int state = mqttClient.state();
      DBG_PRINT("MQTT connection attempt failed, rc=");
      DBG_PRINT(state);
      DBG_PRINT(" (");
      switch(state) {
        case -4: DBG_PRINT("MQTT_CONNECTION_TIMEOUT"); break;
        case -3: DBG_PRINT("MQTT_CONNECTION_LOST"); break;
        case -2: DBG_PRINT("MQTT_CONNECT_FAILED"); break;
        case -1: DBG_PRINT("MQTT_DISCONNECTED"); break;
        case 1: DBG_PRINT("MQTT_CONNECT_BAD_PROTOCOL"); break;
        case 2: DBG_PRINT("MQTT_CONNECT_BAD_CLIENT_ID"); break;
        case 3: DBG_PRINT("MQTT_CONNECT_UNAVAILABLE"); break;
        case 4: DBG_PRINT("MQTT_CONNECT_BAD_CREDENTIALS"); break;
        case 5: DBG_PRINT("MQTT_CONNECT_UNAUTHORIZED"); break;
        default: DBG_PRINT("UNKNOWN"); break;
      }
      DBG_PRINTLN(")");
      yield(); // Allow other tasks to run
    }
  }
  
  if (connected) {
    DBG_PRINT("MQTT connected to ");
    DBG_PRINT(config.mqttHost);
    DBG_PRINT(":");
    DBG_PRINTLN(config.mqttPort);
    
    mqttConnected = true;
    // Initialize heartbeat timer
    lastStatusHeartbeat = millis();
    publishAvailability(true);
    
    // Reset discovery flag and publish discovery after connection
    discoveryPublished = false;
    publishDiscovery();
  } else {
    DBG_PRINTLN("MQTT connection failed after timeout");
    mqttConnected = false;
  }
  
  return connected;
}

// Initialize MQTT
inline bool initMQTT() {
  if (config.mqttHost[0] == '\0') {
    DBG_PRINTLN("MQTT not configured");
    return false;
  }
  
  // Initialize topics (will use WiFi MAC, so WiFi must be connected)
  if (WiFi.status() == WL_CONNECTED) {
    initMQTTTopics();
  }
  
  return connectMQTT();
}

// MQTT loop - call regularly
inline void loopMQTT() {
  unsigned long now = millis();
  
  if (!mqttClient.connected()) {
    // Connection lost - try to send offline status if we were previously connected
    if (mqttConnected) {
      // Try to send offline status before connection is fully lost
      // Note: This might not always succeed if connection is already broken
      DBG_PRINTLN("MQTT connection lost, attempting to send offline status");
      if (mqttTopicsInitialized) {
        // Try to publish offline status directly (might fail if connection is broken)
        char statusTopic[96];
        snprintf(statusTopic, sizeof(statusTopic), "tele/%s/status", baseTopic);
        mqttClient.publish(statusTopic, "offline", true); // retain = true
      }
      mqttConnected = false;
    }
    
    // Try to reconnect
    if (now - lastMQTTReconnect >= MQTT_RECONNECT_INTERVAL) {
      lastMQTTReconnect = now;
      if (WiFi.status() == WL_CONNECTED) {
        connectMQTT();
      }
    }
  } else {
    if (!mqttConnected) {
      // Just reconnected, reset discovery and send pending data
      discoveryPublished = false;
      publishDiscovery();
      
      // Send pending data if available
      if (pendingDataSend) {
        DBG_PRINTLN("Sending pending sensor data after reconnection");
        publishSensorData(lastPM25, lastTemp, lastHumidity, lastPressure, 
                         lastAQI, lastAQICategory, lastDewPoint, lastComfortIndex, lastUptime);
      }
      
      // Reset heartbeat timer on reconnect
      lastStatusHeartbeat = now;
    }
    mqttConnected = true;
    mqttClient.loop();
    
    // Send periodic "online" heartbeat
    if (now - lastStatusHeartbeat >= STATUS_HEARTBEAT_INTERVAL) {
      lastStatusHeartbeat = now;
      publishAvailability(true);
    }
  }
}