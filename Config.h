#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "secrets.h"

#ifdef DEBUG
#define DBG_PRINT(...) Serial.print(__VA_ARGS__)
#define DBG_PRINTLN(...) Serial.println(__VA_ARGS__)
#define DBG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DBG_PRINT(...)
#define DBG_PRINTLN(...)
#define DBG_PRINTF(...)
#endif

#ifndef DEFAULT_WIFI_SSID
#define DEFAULT_WIFI_SSID ""
#endif

#ifndef DEFAULT_WIFI_PASSWORD
#define DEFAULT_WIFI_PASSWORD ""
#endif

#ifndef DEFAULT_HOSTNAME
#define DEFAULT_HOSTNAME ""
#endif

#ifndef DEFAULT_MQTT_HOST
#define DEFAULT_MQTT_HOST ""
#endif

#ifndef DEFAULT_MQTT_PORT
#define DEFAULT_MQTT_PORT 1883
#endif

#ifndef DEFAULT_MQTT_USER
#define DEFAULT_MQTT_USER ""
#endif

#ifndef DEFAULT_MQTT_PASSWORD
#define DEFAULT_MQTT_PASSWORD ""
#endif

#ifndef DEFAULT_MQTT_TOPIC
#define DEFAULT_MQTT_TOPIC "ikea-air-monitor"
#endif

#ifndef DEFAULT_SEND_INTERVAL
#define DEFAULT_SEND_INTERVAL 10000
#endif

#ifndef DEFAULT_OTA_PASSWORD
#define DEFAULT_OTA_PASSWORD ""
#endif

struct DeviceConfig {
  char ssid[32];
  char password[64];
  char hostname[32];
  char mqttHost[64];
  uint16_t mqttPort;
  char mqttUser[32];
  char mqttPassword[64];
  char mqttTopic[64];
  uint32_t sendInterval;
  float tempOffset;
  uint32_t defaultsHash;
};

inline uint32_t hashStr(const char *s, uint32_t h = 2166136261UL) {
  while (*s) {
    h = (h ^ static_cast<uint8_t>(*s++)) * 16777619UL;
  }
  return h;
}

inline uint32_t calcDefaultsHash() {
  uint32_t h = 2166136261UL;
  h = hashStr(DEFAULT_WIFI_SSID, h);
  h = hashStr(DEFAULT_WIFI_PASSWORD, h);
  h = hashStr(DEFAULT_HOSTNAME, h);
  h = hashStr(DEFAULT_MQTT_HOST, h);
  h = hashStr(DEFAULT_MQTT_USER, h);
  h = hashStr(DEFAULT_MQTT_PASSWORD, h);
  h = hashStr(DEFAULT_MQTT_TOPIC, h);
  h = (h ^ DEFAULT_MQTT_PORT) * 16777619UL;
  return h;
}

inline void resetConfig(DeviceConfig &cfg) {
  memset(&cfg, 0, sizeof(cfg));
  cfg.tempOffset = -2.0f;
  strncpy(cfg.ssid, DEFAULT_WIFI_SSID, sizeof(cfg.ssid) - 1);
  cfg.ssid[sizeof(cfg.ssid) - 1] = '\0';
  strncpy(cfg.password, DEFAULT_WIFI_PASSWORD, sizeof(cfg.password) - 1);
  cfg.password[sizeof(cfg.password) - 1] = '\0';
  strncpy(cfg.hostname, DEFAULT_HOSTNAME, sizeof(cfg.hostname) - 1);
  cfg.hostname[sizeof(cfg.hostname) - 1] = '\0';
  strncpy(cfg.mqttHost, DEFAULT_MQTT_HOST, sizeof(cfg.mqttHost) - 1);
  cfg.mqttHost[sizeof(cfg.mqttHost) - 1] = '\0';
  cfg.mqttPort = DEFAULT_MQTT_PORT;
  strncpy(cfg.mqttUser, DEFAULT_MQTT_USER, sizeof(cfg.mqttUser) - 1);
  cfg.mqttUser[sizeof(cfg.mqttUser) - 1] = '\0';
  strncpy(cfg.mqttPassword, DEFAULT_MQTT_PASSWORD, sizeof(cfg.mqttPassword) - 1);
  cfg.mqttPassword[sizeof(cfg.mqttPassword) - 1] = '\0';
  strncpy(cfg.mqttTopic, DEFAULT_MQTT_TOPIC, sizeof(cfg.mqttTopic) - 1);
  cfg.mqttTopic[sizeof(cfg.mqttTopic) - 1] = '\0';
  cfg.sendInterval = DEFAULT_SEND_INTERVAL;
  cfg.defaultsHash = calcDefaultsHash();
}

inline bool loadConfig(DeviceConfig &cfg) {
  if (!LittleFS.begin()) return false;
  File f = LittleFS.open("/config.bin", "r");
  if (!f) {
    LittleFS.end();
    return false;
  }
  size_t r = f.read(reinterpret_cast<uint8_t*>(&cfg), sizeof(cfg));
  f.close();
  LittleFS.end();
  if (r == sizeof(cfg) && cfg.defaultsHash == calcDefaultsHash()) {
    return true;
  }
  resetConfig(cfg);
  return false;
}

inline bool saveConfig(DeviceConfig &cfg) {
  cfg.defaultsHash = calcDefaultsHash();
  if (!LittleFS.begin()) return false;
  File f = LittleFS.open("/config.bin", "w");
  if (!f) {
    LittleFS.end();
    return false;
  }
  size_t w = f.write(reinterpret_cast<const uint8_t*>(&cfg), sizeof(cfg));
  f.close();
  LittleFS.end();
  return w == sizeof(cfg);
}
