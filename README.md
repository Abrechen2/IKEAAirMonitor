# IKEA Air Monitor

ArduinoIDE-Projekt für einen Wemos D1 mini, der die Luftqualität mit einem
umgebauten IKEA Vindriktning Sensor sowie einem BME280 erfasst. Die Messwerte
werden per TCP an einen Node-RED Server gesendet. Ein kleiner Webserver erlaubt
die Konfiguration ähnlich zu Tasmota.

## Funktionen
- Liest PM2.5-Werte aus dem Vindriktning über UART.
- Liest Temperatur, Luftfeuchtigkeit und Luftdruck aus einem BME280 (I2C).
- Sendet die Messwerte als rohe Bytes über eine TCP-Verbindung an Node‑RED.
- Weboberfläche zur Anzeige der Werte und zur Konfiguration von WLAN,
  Hostname, Node‑RED-Adresse sowie Kalibrierung.
- Erster Start im Access-Point-Modus zur einfachen WLAN-Einrichtung.

## Abhängigkeiten
Im Arduino IDE müssen folgende Bibliotheken installiert sein:
- ESP8266 Board Pakete
- ArduinoJson
- Adafruit BME280 Library

## Verwendung
1. Sketch in die Arduino IDE laden und auf den Wemos D1 mini flashen.
2. Nach dem ersten Start erstellt der Controller ein WLAN **IKEAAirMonitor-Setup**.
3. Mit diesem WLAN verbinden und auf `http://192.168.4.1` die eigenen Daten
   (WLAN, Node‑RED usw.) eintragen.
4. Der Controller startet neu und verbindet sich anschließend mit dem
   konfigurierten WLAN und Node‑RED.
