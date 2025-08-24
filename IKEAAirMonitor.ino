#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <SoftwareSerial.h>

// Pins for Vindriktning sensor
#define VIND_RX D5
#define VIND_TX D6

SoftwareSerial vindSerial(VIND_RX, VIND_TX);
ESP8266WebServer server(80);
DNSServer dns;
Adafruit_BME280 bme;

struct Config {
  String ssid;
  String password;
  String hostname;
  String nodeHost;
  uint16_t nodePort;
  float pm25Cal;
} config;

bool shouldRestart = false;

bool loadConfig() {
  if (!LittleFS.begin()) {
    return false;
  }
  if (!LittleFS.exists("/config.json")) {
    return false;
  }
  File f = LittleFS.open("/config.json", "r");
  if (!f) return false;
  DynamicJsonDocument doc(512);
  DeserializationError err = deserializeJson(doc, f);
  if (err) {
    f.close();
    return false;
  }
  config.ssid = doc["ssid"].as<String>();
  config.password = doc["password"].as<String>();
  config.hostname = doc["hostname"].as<String>();
  config.nodeHost = doc["nodeHost"].as<String>();
  config.nodePort = doc["nodePort"] | 1883;
  config.pm25Cal = doc["pm25Cal"] | 0.0;
  f.close();
  return true;
}

void saveConfig() {
  DynamicJsonDocument doc(512);
  doc["ssid"] = config.ssid;
  doc["password"] = config.password;
  doc["hostname"] = config.hostname;
  doc["nodeHost"] = config.nodeHost;
  doc["nodePort"] = config.nodePort;
  doc["pm25Cal"] = config.pm25Cal;
  File f = LittleFS.open("/config.json", "w");
  serializeJson(doc, f);
  f.close();
}

String htmlHeader() {
  return F("<!DOCTYPE html><html><head><meta charset='utf-8'><title>IKEAAirMonitor</title><style>body{font-family:Arial;margin:20px;}input{margin:5px;}label{display:block;margin-top:10px;}button{margin-top:10px;}nav a{margin-right:10px;}</style></head><body><nav><a href='/'>Status</a><a href='/config'>Konfiguration</a></nav>");
}

void handleRoot() {
  float t = bme.readTemperature();
  float h = bme.readHumidity();
  float p = bme.readPressure() / 100.0F;
  uint16_t pm = readPM25();
  String html = htmlHeader();
  html += F("<h1>Status</h1>");
  html += "<p>PM2.5: " + String(pm) + " µg/m³</p>";
  html += "<p>Temperatur: " + String(t,1) + " °C</p>";
  html += "<p>Luftfeuchte: " + String(h,1) + " %</p>";
  html += "<p>Luftdruck: " + String(p,1) + " hPa</p>";
  html += F("</body></html>");
  server.send(200, "text/html", html);
}

void handleConfigPage() {
  String html = htmlHeader();
  html += F("<h1>Konfiguration</h1><form method='POST' action='/save'>");
  html += F("<label>SSID<input name='ssid'></label>");
  html += F("<label>Passwort<input type='password' name='password'></label>");
  html += F("<label>Hostname<input name='hostname'></label>");
  html += F("<label>Node-RED Host<input name='nodeHost'></label>");
  html += F("<label>Node-RED Port<input name='nodePort' value='1883'></label>");
  html += F("<label>PM2.5-Kalibrierung<input name='pm25Cal' value='0'></label>");
  html += F("<button type='submit'>Speichern</button></form></body></html>");
  server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("ssid")) config.ssid = server.arg("ssid");
  if (server.hasArg("password")) config.password = server.arg("password");
  if (server.hasArg("hostname")) config.hostname = server.arg("hostname");
  if (server.hasArg("nodeHost")) config.nodeHost = server.arg("nodeHost");
  if (server.hasArg("nodePort")) config.nodePort = server.arg("nodePort").toInt();
  if (server.hasArg("pm25Cal")) config.pm25Cal = server.arg("pm25Cal").toFloat();
  saveConfig();
  server.send(200, "text/html", F("<p>Gespeichert, starte neu...</p>"));
  shouldRestart = true;
}

void startAPMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("IKEAAirMonitor-Setup");
  dns.start(53, "ikea.setup", WiFi.softAPIP());
  server.on("/", handleConfigPage);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  while (!shouldRestart) {
    dns.processNextRequest();
    server.handleClient();
    delay(10);
  }
  ESP.restart();
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname(config.hostname);
  WiFi.begin(config.ssid.c_str(), config.password.c_str());
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
  }
  if (WiFi.status() != WL_CONNECTED) {
    startAPMode();
  }
}

uint16_t readPM25() {
  const uint8_t FRAME_LEN = 20;
  static uint8_t buf[FRAME_LEN];
  while (vindSerial.available()) {
    if (vindSerial.read() == 0x16) {
      buf[0] = 0x16;
      vindSerial.readBytes(buf + 1, FRAME_LEN - 1);
      uint8_t sum = 0;
      for (int i = 0; i < FRAME_LEN - 1; i++) sum += buf[i];
      if (sum == buf[FRAME_LEN - 1]) {
        uint16_t pm = ((uint16_t)buf[5] << 8) | buf[6];
        return pm + config.pm25Cal;
      }
    }
  }
  return 0;
}

void sendToNodeRed(uint16_t pm) {
  WiFiClient client;
  if (client.connect(config.nodeHost.c_str(), config.nodePort)) {
    float t = bme.readTemperature();
    float h = bme.readHumidity();
    float p = bme.readPressure() / 100.0F;
    client.write((uint8_t*)&pm, sizeof(pm));
    client.write((uint8_t*)&t, sizeof(t));
    client.write((uint8_t*)&h, sizeof(h));
    client.write((uint8_t*)&p, sizeof(p));
    client.stop();
  }
}

unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);
  vindSerial.begin(9600);
  if (!loadConfig()) {
    startAPMode();
  }
  connectWifi();
  server.on("/", handleRoot);
  server.on("/config", handleConfigPage);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  bme.begin(0x76);
}

void loop() {
  server.handleClient();
  dns.processNextRequest();
  if (WiFi.status() == WL_CONNECTED && millis() - lastSend > 60000) {
    uint16_t pm = readPM25();
    sendToNodeRed(pm);
    lastSend = millis();
  }
  if (shouldRestart) {
    delay(1000);
    ESP.restart();
  }
}

