#pragma once
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include "Config.h"
#include "Sensors.h"

extern ESP8266WebServer server;
extern DNSServer dns;
extern Config config;
extern bool shouldRestart;

inline String htmlHeader() {
  return F("<!DOCTYPE html><html><head><meta charset='utf-8'><title>IKEAAirMonitor</title><style>body{font-family:Arial;margin:20px;}input{margin:5px;}label{display:block;margin-top:10px;}nav a{margin-right:10px;}</style></head><body><nav><a href='/'>Status</a><a href='/config'>Konfiguration</a></nav>");
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
  html += F("</body></html>");
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
  html += "<label>PM2.5-Kalibrierung<input name='pm25Cal' value='" + String(config.pm25Cal,1) + "'></label>";
  html += F("<button type='submit'>Speichern</button></form></body></html>");
  server.send(200, "text/html", html);
}

inline void handleSave() {
  if (server.hasArg("ssid")) strncpy(config.ssid, server.arg("ssid").c_str(), sizeof(config.ssid));
  if (server.hasArg("password")) strncpy(config.password, server.arg("password").c_str(), sizeof(config.password));
  if (server.hasArg("hostname")) strncpy(config.hostname, server.arg("hostname").c_str(), sizeof(config.hostname));
  if (server.hasArg("nodeHost")) strncpy(config.nodeHost, server.arg("nodeHost").c_str(), sizeof(config.nodeHost));
  if (server.hasArg("nodePort")) config.nodePort = server.arg("nodePort").toInt();
  if (server.hasArg("pm25Cal")) config.pm25Cal = server.arg("pm25Cal").toFloat();
  saveConfig(config);

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
  html += F("<p>Neustart in 5s...</p></body></html>");
  server.send(200, "text/html", html);
  shouldRestart = true;
}

inline void setupWeb() {
  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
}

inline void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("IKEAAirMonitor-Setup");
  dns.start(53, "*", WiFi.softAPIP());
  setupWeb();
}

inline void handleWeb() {
  dns.processNextRequest();
  server.handleClient();
}

