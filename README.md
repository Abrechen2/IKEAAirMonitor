# IKEA Air Monitor

ArduinoIDE-Projekt für einen Wemos D1 mini, der die Luftqualität mit einem
umgebauten IKEA Vindriktning Sensor sowie einem BME280 erfasst. Die Messwerte

werden als rohe Bytes per HTTP-POST an einen Node-RED Server gesendet. Ein
kleiner Webserver erlaubt die Konfiguration ähnlich zu Tasmota.


## Funktionen
- Liest PM2.5-Werte aus dem Vindriktning über UART.
- Liest Temperatur, Luftfeuchtigkeit und Luftdruck aus einem BME280 (I2C).

- Sendet die Messwerte als rohe Bytes per HTTP-POST an Node‑RED.

 - Weboberfläche zur Anzeige der Werte und zur Konfiguration von WLAN,
   Hostname, Node‑RED-Adresse sowie Kalibrierung.
 - Erster Start im Access-Point-Modus zur einfachen WLAN-Einrichtung.
 - Optional können WLAN-Zugangsdaten im Code hinterlegt werden; der Access-Point
   startet dann nur, wenn keine Verbindung hergestellt werden konnte.

## Abhängigkeiten
Im Arduino IDE müssen folgende Bibliotheken installiert sein:
- ESP8266 Board Pakete



- Adafruit BME280 Library

## Verwendung
1. Sketch in die Arduino IDE laden und auf den Wemos D1 mini flashen.
2. Nach dem ersten Start erstellt der Controller ein WLAN **IKEAAirMonitor-Setup**.
3. Mit diesem WLAN verbinden und auf `http://192.168.4.1` die eigenen Daten

 (WLAN, Node‑RED usw.) eintragen.
4. Der Controller startet neu und verbindet sich anschließend mit dem
   konfigurierten WLAN und Node‑RED.

### WLAN-Daten im Code hinterlegen
In der Datei `Config.h` können `DEFAULT_WIFI_SSID` und
`DEFAULT_WIFI_PASSWORD` gesetzt werden. Dadurch versucht das Gerät, sich
automatisch mit diesem WLAN zu verbinden; der Konfigurationsmodus wird nur
gestartet, wenn diese Verbindung fehlschlägt.

## Node-RED Flow
1. `http in` Node hinzufügen
   - Methode: **POST**
   - URL: `/sensor`
   - Option „Return the raw body“ aktivieren
2. Function-Node zum Parsen der 14 Bytes
   ```javascript
   const buf = msg.payload;
   const pm  = buf.readUInt16LE(0);
   const t   = buf.readFloatLE(2);
   const h   = buf.readFloatLE(6);
   const p   = buf.readFloatLE(10);
   msg.payload = { pm25: pm, temp: t, humidity: h, pressure: p };
   return msg;
   ```
3. `http response` Node anhängen, damit der Client eine Antwort erhält.
4. Optional weitere Nodes (z. B. Debug oder Dashboard) zum Anzeigen der Werte
   verbinden.
=======
   (WLAN, Node‑RED usw.) eintragen.
4. Der Controller startet neu und verbindet sich anschließend mit dem
   konfigurierten WLAN und Node‑RED.

