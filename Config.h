#pragma once
#include <Arduino.h>
#include <LittleFS.h>

struct Config {
  char ssid[32];
  char password[64];
  char hostname[32];
  char nodeHost[64];
  uint16_t nodePort;
  float pm25Cal;
};

inline void resetConfig(Config &cfg) {
  memset(&cfg, 0, sizeof(cfg));
  cfg.nodePort = 1880;
  cfg.pm25Cal = 0.0f;
}

inline bool loadConfig(Config &cfg) {
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

inline bool saveConfig(const Config &cfg) {
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

