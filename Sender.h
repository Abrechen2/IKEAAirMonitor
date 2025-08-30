#pragma once
#include <ESP8266HTTPClient.h>
#include "Config.h"

inline void sendToNodeRed(uint16_t pm, float t, float h, float p, uint32_t uptime, const DeviceConfig &cfg) {
  WiFiClient client;
  HTTPClient http;
  String path = cfg.nodePath[0] == '/' ? cfg.nodePath : String('/') + cfg.nodePath;
  String url = String("http://") + cfg.nodeHost + ":" + cfg.nodePort + path;
  if (http.begin(client, url)) {
    http.addHeader("Content-Type", "application/octet-stream");
    uint8_t payload[18];
    payload[0] = pm & 0xFF;
    payload[1] = pm >> 8;
    memcpy(payload + 2, &t, 4);
    memcpy(payload + 6, &h, 4);
    memcpy(payload + 10, &p, 4);
    memcpy(payload + 14, &uptime, 4);
    http.POST(payload, sizeof(payload));
    http.end();
  }
}

