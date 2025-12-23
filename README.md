# IKEA Air Monitor

ArduinoIDE-Projekt für einen Wemos D1 mini, der die Luftqualität mit einem
umgebauten IKEA Vindriktning Sensor sowie einem BME280 erfasst. Die Messwerte
werden per MQTT an Home Assistant gesendet. Ein kleiner Webserver erlaubt die
Konfiguration ähnlich zu Tasmota.

## Funktionen

- Liest PM2.5-Werte aus dem Vindriktning über UART.
- Liest Temperatur, Luftfeuchtigkeit und Luftdruck aus einem BME280 (I2C).
- Berechnet AQI (Air Quality Index), Taupunkt und Comfort-Index direkt im ESP8266.
- Sendet alle Messwerte per MQTT an Home Assistant mit automatischer Discovery.
- Weboberfläche zur Anzeige der Werte und zur Konfiguration von WLAN, Hostname,
  MQTT-Einstellungen sowie Temperatur-Offset.
- Erster Start im Access-Point-Modus zur einfachen WLAN-Einrichtung.
- Optional können WLAN- und MQTT-Zugangsdaten im Code hinterlegt werden; der
  Access-Point startet dann nur, wenn keine Verbindung hergestellt werden konnte.

## Abhängigkeiten

Im Arduino IDE müssen folgende Bibliotheken installiert sein:
- ESP8266 Board Pakete
- Adafruit BME280 Library
- PubSubClient (für MQTT)

## Verwendung

1. Sketch in die Arduino IDE laden und auf den Wemos D1 mini flashen.
2. Nach dem ersten Start erstellt der Controller ein WLAN **IKEAAirMonitor-Setup**.
3. Mit diesem WLAN verbinden und auf `http://192.168.4.1` die eigenen Daten
   (WLAN, MQTT usw.) eintragen.
4. Der Controller startet neu und verbindet sich anschließend mit dem
   konfigurierten WLAN und MQTT-Broker.
5. Home Assistant erkennt das Gerät automatisch über MQTT Discovery.

### WLAN- und MQTT-Daten im Code hinterlegen

In der Datei `secrets.h` (siehe `secretstemplate.h`) können `DEFAULT_WIFI_SSID`,
`DEFAULT_WIFI_PASSWORD`, `DEFAULT_HOSTNAME`, `DEFAULT_MQTT_HOST`,
`DEFAULT_MQTT_PORT`, `DEFAULT_MQTT_USER`, `DEFAULT_MQTT_PASSWORD` sowie
`DEFAULT_MQTT_TOPIC` gesetzt werden. Dadurch versucht das Gerät, sich automatisch
mit diesem WLAN und dem MQTT-Broker zu verbinden; die Einstellungen können später
auch über die Weboberfläche angepasst werden. Der Konfigurationsmodus wird nur
gestartet, wenn diese Verbindung fehlschlägt. Änderungen an diesen
Voreinstellungen werden beim nächsten Start automatisch in die gespeicherte
Konfiguration übernommen.

## Home Assistant Integration

Das Gerät nutzt MQTT Discovery, um automatisch in Home Assistant erkannt zu werden.
Folgende Sensoren werden automatisch erstellt:

- **PM2.5** - Feinstaub in µg/m³
- **Temperature** - Temperatur in °C
- **Humidity** - Luftfeuchtigkeit in %
- **Pressure** - Luftdruck in hPa
- **AQI** - Air Quality Index (berechnet aus PM2.5)
- **AQI Category** - AQI-Kategorie (1-5)
- **Dew Point** - Taupunkt in °C (berechnet)
- **Comfort Index** - Komfort-Index 0-100 (berechnet)
- **Uptime** - Betriebszeit in Sekunden

Alle Sensordaten werden als JSON im State-Topic `{mqtt_topic}/state` publiziert.
Das Gerät sendet auch einen Availability-Status unter `{mqtt_topic}/availability`.

### MQTT Topics

- **Discovery:** `homeassistant/sensor/{device_id}/{sensor_name}/config`
- **State:** `{mqtt_topic}/state` (JSON mit allen Sensordaten)
- **Availability:** `{mqtt_topic}/availability` (online/offline)

### JSON State Format

```json
{
  "pm25": 15,
  "temperature": 22.5,
  "humidity": 45.0,
  "pressure": 1013.25,
  "aqi": 62,
  "aqi_category": 2,
  "dew_point": 10.2,
  "comfort_index": 85.5,
  "uptime": 3600
}
```

## Berechnungen

Das Gerät führt folgende Berechnungen direkt im ESP8266 durch:

- **AQI (Air Quality Index):** Berechnung basierend auf US EPA Standard für PM2.5
- **Taupunkt:** Berechnung aus Temperatur und Luftfeuchtigkeit
- **Comfort Index:** Bewertung der Raumluftqualität (0-100, höher ist besser)

## Projektstruktur

```
IKEAAirMonitor/
├── IKEAAirMonitor.ino    # Hauptprogramm
├── Config.h              # Konfigurationsverwaltung
├── Sensors.h             # Sensoren (BME280, Vindriktning)
├── MQTTManager.h         # MQTT-Verbindung und Home Assistant Discovery
├── Calculations.h        # Berechnungen (AQI, Taupunkt, Comfort-Index)
├── WebServer.h           # Webserver für Konfiguration
├── secrets.h             # Sensible Daten (nicht im Repository)
├── secretstemplate.h     # Template für secrets.h
├── node-red/             # Legacy Node-RED Flows (nicht mehr benötigt)
└── README.md             # Diese Datei
```
