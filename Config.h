#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "secrets.h"

#ifdef DEBUG
#define DBG_PRINT(x) Serial.print(x)
#define DBG_PRINTLN(x) Serial.println(x)
#else
#define DBG_PRINT(x)
#define DBG_PRINTLN(x)
#endif

#ifndef DEFAULT_WIFI_SSID
#define DEFAULT_WIFI_SSID ""
#endif

#ifndef DEFAULT_WIFI_PASSWORD
#define DEFAULT_WIFI_PASSWORD ""
#endif

#ifndef DEFAULT_NODE_HOST
#define DEFAULT_NODE_HOST ""
#endif

#ifndef DEFAULT_NODE_PORT
#define DEFAULT_NODE_PORT 1880
#endif

#ifndef DEFAULT_SEND_INTERVAL
#define DEFAULT_SEND_INTERVAL 10000
#endif

struct DeviceConfig {
  char ssid[32];
  char password[64];
  char hostname[32];
  char nodeHost[64];
  uint16_t nodePort;
  uint32_t sendInterval;
  float pm25Cal;
};

inline void resetConfig(DeviceConfig &cfg) {
  memset(&cfg, 0, sizeof(cfg));
  cfg.pm25Cal = 0.0f;
  strncpy(cfg.ssid, DEFAULT_WIFI_SSID, sizeof(cfg.ssid) - 1);
  strncpy(cfg.password, DEFAULT_WIFI_PASSWORD, sizeof(cfg.password) - 1);
  strncpy(cfg.nodeHost, DEFAULT_NODE_HOST, sizeof(cfg.nodeHost) - 1);
  cfg.nodePort = DEFAULT_NODE_PORT;
  cfg.sendInterval = DEFAULT_SEND_INTERVAL;
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
  return r == sizeof(cfg);
}


inline bool saveConfig(const DeviceConfig &cfg) {

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

