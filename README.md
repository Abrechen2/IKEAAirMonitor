# IKEA Air Monitor

ArduinoIDE-Projekt für einen Wemos D1 mini, der die Luftqualität mit einem
umgebauten IKEA Vindriktning Sensor sowie einem BME280 erfasst. Die Messwerte

werden als JSON per HTTP-POST an einen Node-RED Server gesendet. Ein
kleiner Webserver erlaubt die Konfiguration ähnlich zu Tasmota.


## Funktionen
- Liest PM2.5-Werte aus dem Vindriktning über UART.
- Liest Temperatur, Luftfeuchtigkeit und Luftdruck aus einem BME280 (I2C).

- Sendet die Messwerte als JSON per HTTP-POST an Node‑RED.

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
Ein Beispiel-Flow liegt im Ordner `node-red/ikea_air_monitor_flow.json` und kann
direkt in Node-RED importiert werden. Er besteht aus folgenden Schritten:

1. Ein `http in` Node nimmt POST-Anfragen unter `/sensor` entgegen und
   übergibt den JSON-Body weiter.
2. Der Function-Node **format + thresholds** bereitet die Daten für InfluxDB v2
   auf: Er setzt Tags (`device_id`, `location`, `device_type`, `data_type`) und
   wendet individuelle Schwellwerte auf AQI, CO₂, PM2.5, TVOC sowie
   Luftfeuchtigkeit an. Werden keine Schwellwerte überschritten, wird die
   Nachricht verworfen.
3. Nur Daten oberhalb der Grenzwerte werden an einen `influxdb out` Node
   weitergeleitet.
4. Ein `http response` Node bestätigt den Empfang.

Vor der Nutzung müssen in der `influxdb`-Konfiguration URL, Organisation,
Bucket und Token angepasst werden. Die Schwellwerte können direkt im Function
Node angepasst werden (`thresholds`-Objekt).
