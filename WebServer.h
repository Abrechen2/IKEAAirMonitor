#pragma once
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <stdio.h>
#include "Config.h"
#include "Sensors.h"
#include "MQTTManager.h"

extern ESP8266WebServer server;
extern DNSServer dns;
extern DeviceConfig config;
extern bool shouldRestart;
extern unsigned long uptimeMillis;

inline String htmlHeader() {
  return F(
    "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>IKEAAirMonitor</title><style>"
    "body{font-family:Arial,sans-serif;margin:20px;background:#f5f5f5;color:#333;}"
    "nav{margin-bottom:20px;}nav a{margin-right:15px;text-decoration:none;color:#0366d6;}"
    ".card{background:#fff;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}"
    "label{display:block;margin-top:10px;}"
    "input{width:100%;padding:8px;margin-top:5px;border:1px solid #ccc;border-radius:4px;}"
    "button{margin-top:15px;padding:10px 15px;background:#0366d6;color:#fff;border:none;border-radius:4px;}"
    "</style></head><body><nav><a href='/'>Status</a><a href='/config'>Konfiguration</a></nav><div class='card'>"
  );
}

inline void formatUptime(unsigned long ms, char* buf, size_t len) {
  unsigned long seconds = ms / 1000;
  unsigned long days = seconds / 86400;
  seconds %= 86400;
  unsigned long hours = seconds / 3600;
  seconds %= 3600;
  unsigned long minutes = seconds / 60;
  seconds %= 60;
  snprintf(buf, len, "%lu d %02lu:%02lu:%02lu", days, hours, minutes, seconds);
}

inline void handleRoot() {
  uint16_t pm; float t, h, p;
  readMeasurements(pm, t, h, p, config);
  
  char uptimeStr[32];
  formatUptime(uptimeMillis, uptimeStr, sizeof(uptimeStr));
  
  char html[1024];
  if (WiFi.status() == WL_CONNECTED) {
    snprintf(html, sizeof(html),
      "%s"
      "<h1>Status</h1>"
      "<p>PM2.5: %u µg/m³</p>"
      "<p>Temperatur: %.1f °C</p>"
      "<p>Luftfeuchte: %.1f %%</p>"
      "<p>Luftdruck: %.1f hPa</p>"
      "<p>Uptime: %s</p>"
      "<p>MQTT Status: %s</p>"
      "<p>OTA Status: Aktiv (Port 8266, Hostname: %s)</p>"
      "</div></body></html>",
      htmlHeader().c_str(),
      pm, t, h, p, uptimeStr,
      mqttConnected ? "Verbunden" : "Nicht verbunden",
      config.hostname
    );
  } else {
    snprintf(html, sizeof(html),
      "%s"
      "<h1>Status</h1>"
      "<p>PM2.5: %u µg/m³</p>"
      "<p>Temperatur: %.1f °C</p>"
      "<p>Luftfeuchte: %.1f %%</p>"
      "<p>Luftdruck: %.1f hPa</p>"
      "<p>Uptime: %s</p>"
      "<p>MQTT Status: %s</p>"
      "<p>OTA Status: Inaktiv (WiFi nicht verbunden)</p>"
      "</div></body></html>",
      htmlHeader().c_str(),
      pm, t, h, p, uptimeStr,
      mqttConnected ? "Verbunden" : "Nicht verbunden"
    );
  }
  
  server.send(200, "text/html", html);
}

inline void handleConfig() {
  char html[2048];
  int len = snprintf(html, sizeof(html),
    "%s"
    "<h1>Konfiguration</h1><form method='POST' action='/save'>"
    "<label>SSID<input name='ssid' value='%s'></label>"
    "<label>Passwort<input type='password' name='password' value='%s'></label>"
    "<label>Hostname<input name='hostname' value='%s'></label>"
    "<label>MQTT Host<input name='mqttHost' value='%s'></label>"
    "<label>MQTT Port<input name='mqttPort' value='%u'></label>"
    "<label>MQTT Benutzer<input name='mqttUser' value='%s'></label>"
    "<label>MQTT Passwort<input type='password' name='mqttPassword' value='%s'></label>"
    "<label>MQTT Topic<input name='mqttTopic' value='%s'></label>"
    "<label>Sendeintervall (s)<input name='sendInterval' value='%lu'></label>"
    "<label>Temperatur-Offset<input name='tempOffset' value='%.1f'></label>"
    "<button type='submit'>Speichern</button></form></div></body></html>",
    htmlHeader().c_str(),
    config.ssid, config.password, config.hostname, config.mqttHost, config.mqttPort,
    config.mqttUser, config.mqttPassword, config.mqttTopic,
    config.sendInterval / 1000, config.tempOffset
  );
  server.send(200, "text/html", html);
}

inline void handleSave() {
  // Validate and set SSID
  if (server.hasArg("ssid")) {
    String ssid = server.arg("ssid");
    if (ssid.length() < sizeof(config.ssid)) {
      ssid.toCharArray(config.ssid, sizeof(config.ssid));
    }
  }
  
  // Validate and set password
  if (server.hasArg("password")) {
    String password = server.arg("password");
    if (password.length() < sizeof(config.password)) {
      password.toCharArray(config.password, sizeof(config.password));
    }
  }
  
  // Validate and set hostname
  if (server.hasArg("hostname")) {
    String hostname = server.arg("hostname");
    if (hostname.length() > 0 && hostname.length() < sizeof(config.hostname)) {
      hostname.toCharArray(config.hostname, sizeof(config.hostname));
    }
  }
  
  // Validate and set MQTT host
  if (server.hasArg("mqttHost")) {
    String mqttHost = server.arg("mqttHost");
    if (mqttHost.length() < sizeof(config.mqttHost)) {
      mqttHost.toCharArray(config.mqttHost, sizeof(config.mqttHost));
    }
  }
  
  // Validate and set MQTT port (1-65535)
  if (server.hasArg("mqttPort")) {
    int port = server.arg("mqttPort").toInt();
    if (port > 0 && port <= 65535) {
      config.mqttPort = port;
    }
  }
  
  // Validate and set MQTT user
  if (server.hasArg("mqttUser")) {
    String mqttUser = server.arg("mqttUser");
    if (mqttUser.length() < sizeof(config.mqttUser)) {
      mqttUser.toCharArray(config.mqttUser, sizeof(config.mqttUser));
    }
  }
  
  // Validate and set MQTT password
  if (server.hasArg("mqttPassword")) {
    String mqttPassword = server.arg("mqttPassword");
    if (mqttPassword.length() < sizeof(config.mqttPassword)) {
      mqttPassword.toCharArray(config.mqttPassword, sizeof(config.mqttPassword));
    }
  }
  
  // Validate and set MQTT topic
  if (server.hasArg("mqttTopic")) {
    String mqttTopic = server.arg("mqttTopic");
    if (mqttTopic.length() < sizeof(config.mqttTopic)) {
      mqttTopic.toCharArray(config.mqttTopic, sizeof(config.mqttTopic));
    }
  }
  
  // Validate and set send interval (1-3600 seconds)
  if (server.hasArg("sendInterval")) {
    unsigned long interval = server.arg("sendInterval").toInt();
    if (interval >= 1 && interval <= 3600) {
      config.sendInterval = interval * 1000;
    }
  }
  
  // Validate and set temperature offset (-50.0 to 50.0)
  if (server.hasArg("tempOffset")) {
    float offset = server.arg("tempOffset").toFloat();
    if (offset >= -50.0f && offset <= 50.0f) {
      config.tempOffset = offset;
    }
  }
  
  saveConfig(config);
  DBG_PRINTLN("Configuration saved");

  WiFi.mode(WIFI_STA);
  WiFi.hostname(config.hostname);
  WiFi.begin(config.ssid, config.password);
  // Note: WiFi connection will be checked in next loop iteration
  // This is in handleSave() which is called from server.handleClient(),
  // so blocking here is acceptable for configuration save
  char html[512];
  if (WiFi.status() == WL_CONNECTED) {
    IPAddress ip = WiFi.localIP();
    snprintf(html, sizeof(html),
      "%s"
      "<p>Verbunden mit %s</p>"
      "<p>IP: %u.%u.%u.%u</p>"
      "<p>Neustart in 5s...</p></div></body></html>",
      htmlHeader().c_str(),
      config.ssid, ip[0], ip[1], ip[2], ip[3]
    );
  } else {
    snprintf(html, sizeof(html),
      "%s"
      "<p>Verbindung fehlgeschlagen.</p>"
      "<p>Neustart in 5s...</p></div></body></html>",
      htmlHeader().c_str()
    );
  }
  server.send(200, "text/html", html);
  shouldRestart = true;
}

inline void setupWeb() {
  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  DBG_PRINTLN("Web server started");
}

inline void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("IKEAAirMonitor-Setup");
  dns.start(53, "*", WiFi.softAPIP());
  DBG_PRINTLN("Starting setup access point");
  setupWeb();
}

inline void handleWeb() {
  dns.processNextRequest();
  server.handleClient();
}

