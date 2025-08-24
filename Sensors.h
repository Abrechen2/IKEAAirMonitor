#pragma once
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <SoftwareSerial.h>
#include "Config.h"

extern Adafruit_BME280 bme;
extern SoftwareSerial pms;

inline bool initSensors() {
  Wire.begin(D3, D2); // SDA, SCL
  bool ok = bme.begin(0x76);
  pms.begin(9600);
  if (ok) {
    DBG_PRINTLN("BME280 detected");
  } else {
    DBG_PRINTLN("BME280 missing");
  }
  return ok;
}

inline uint16_t readPM25Raw() {
  static uint8_t b[20];
  if (pms.available()) {
    // look for start sequence 0x16 0x11
    if (pms.read() == 0x16 && pms.peek() == 0x11) {
      b[0] = 0x16;
      b[1] = pms.read();
      for (int i = 2; i < 20; i++) {
        while (!pms.available()) {}
        b[i] = pms.read();
      }
      uint8_t sum = 0;
      for (int i = 0; i < 19; i++) sum += b[i];
      if (sum == b[19]) {
        uint16_t val = ((uint16_t)b[5] << 8) | b[6];
        DBG_PRINT("PM raw: ");
        DBG_PRINTLN(val);
        return val;
      } else {
        DBG_PRINTLN("PM checksum error");
      }
    }
  }
  DBG_PRINTLN("No PM data");
  return 0;
}

inline void readMeasurements(uint16_t &pm25, float &t, float &h, float &p, const DeviceConfig &cfg) {

  float pmf = static_cast<float>(readPM25Raw()) + cfg.pm25Cal;
  if (pmf < 0.0f) pmf = 0.0f;
  pm25 = static_cast<uint16_t>(pmf + 0.5f);
  t = bme.readTemperature();
  h = bme.readHumidity();
  p = bme.readPressure() / 100.0F;
}

