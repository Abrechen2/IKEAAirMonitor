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
  if (!f) return false;
  size_t r = f.read((uint8_t*)&cfg, sizeof(cfg));
  f.close();
  return r == sizeof(cfg);
}

inline bool saveConfig(const Config &cfg) {
  if (!LittleFS.begin()) LittleFS.begin();
  File f = LittleFS.open("/config.bin", "w");
  if (!f) return false;
  size_t w = f.write((const uint8_t*)&cfg, sizeof(cfg));
  f.close();
  return w == sizeof(cfg);
}

