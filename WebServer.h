#pragma once
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <stdio.h>
#include "Config.h"
#include "Sensors.h"

extern ESP8266WebServer server;
extern DNSServer dns;
extern DeviceConfig config;
extern bool shouldRestart;
extern unsigned long long uptimeMillis;

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

inline String formatUptime(unsigned long long ms) {
  unsigned long seconds = ms / 1000;
  unsigned long days = seconds / 86400;
  seconds %= 86400;
  unsigned long hours = seconds / 3600;
  seconds %= 3600;
  unsigned long minutes = seconds / 60;
  seconds %= 60;
  char buf[32];
  snprintf(buf, sizeof(buf), "%lu d %02lu:%02lu:%02lu", days, hours, minutes, seconds);
  return String(buf);
}

inline void handleRoot() {
  uint16_t pm; float t, h, p;
  readMeasurements(pm, t, h, p, config);
  String html = htmlHeader();
  html += F("<h1>Status</h1>");
  html += "<p>PM2.5: " + String(pm) + " µg/m³</p>";
  html += "<p>Temperatur: " + String(t,1) + " °C</p>";
  html += "<p>Luftfeuchte: " + String(h,1) + " %</p>";
  html += "<p>Luftdruck: " + String(p,1) + " hPa</p>";
  html += "<p>Uptime: " + formatUptime(uptimeMillis) + "</p>";
  html += F("</div></body></html>");
  server.send(200, "text/html", html);
}

inline void handleConfig() {
  String html = htmlHeader();
  html += F("<h1>Konfiguration</h1><form method='POST' action='/save'>");
  html += "<label>SSID<input name='ssid' value='" + String(config.ssid) + "'></label>";
  html += "<label>Passwort<input type='password' name='password' value='" + String(config.password) + "'></label>";
  html += "<label>Hostname<input name='hostname' value='" + String(config.hostname) + "'></label>";
  html += "<label>Node-RED Host<input name='nodeHost' value='" + String(config.nodeHost) + "'></label>";
  html += "<label>Node-RED Port<input name='nodePort' value='" + String(config.nodePort) + "'></label>";
  html += "<label>Node-RED Pfad<input name='nodePath' value='" + String(config.nodePath) + "'></label>";
  html += "<label>Sendeintervall (s)<input name='sendInterval' value='" + String(config.sendInterval/1000) + "'></label>";
  html += "<label>Temperatur-Offset<input name='tempOffset' value='" + String(config.tempOffset,1) + "'></label>";
  html += F("<button type='submit'>Speichern</button></form></div></body></html>");
  server.send(200, "text/html", html);
}

inline void handleSave() {
  if (server.hasArg("ssid")) server.arg("ssid").toCharArray(config.ssid, sizeof(config.ssid));
  if (server.hasArg("password")) server.arg("password").toCharArray(config.password, sizeof(config.password));
  if (server.hasArg("hostname")) server.arg("hostname").toCharArray(config.hostname, sizeof(config.hostname));
  if (server.hasArg("nodeHost")) server.arg("nodeHost").toCharArray(config.nodeHost, sizeof(config.nodeHost));
  if (server.hasArg("nodePort")) config.nodePort = server.arg("nodePort").toInt();
  if (server.hasArg("nodePath")) {
    String path = server.arg("nodePath");
    if (!path.startsWith("/")) path = "/" + path;
    path.toCharArray(config.nodePath, sizeof(config.nodePath));
  }
  if (server.hasArg("sendInterval")) config.sendInterval = server.arg("sendInterval").toInt() * 1000;
  if (server.hasArg("tempOffset")) config.tempOffset = server.arg("tempOffset").toFloat();
  ensureNodePath(config);
  saveConfig(config);
  DBG_PRINTLN("Configuration saved");

  WiFi.mode(WIFI_STA);
  WiFi.hostname(config.hostname);
  WiFi.begin(config.ssid, config.password);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
  }
  String html = htmlHeader();
  if (WiFi.status() == WL_CONNECTED) {
    html += "<p>Verbunden mit " + String(config.ssid) + "</p>";
    html += "<p>IP: " + WiFi.localIP().toString() + "</p>";
  } else {
    html += "<p>Verbindung fehlgeschlagen.</p>";
  }
  html += F("<p>Neustart in 5s...</p></div></body></html>");
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

