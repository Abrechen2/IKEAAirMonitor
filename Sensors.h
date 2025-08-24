#pragma once
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <SoftwareSerial.h>
#include "Config.h"

extern Adafruit_BME280 bme;
extern SoftwareSerial pms;

inline bool initSensors() {
  Wire.begin(D2, D3); // SDA, SCL
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
  const uint8_t FRAME_LEN = 20;
  static uint8_t buf[FRAME_LEN];
  while (pms.available()) {
    if (pms.read() == 0x16) {
      buf[0] = 0x16;
      if (pms.readBytes(buf + 1, FRAME_LEN - 1) == FRAME_LEN - 1) {
        uint8_t sum = 0;
        for (int i = 0; i < FRAME_LEN - 1; i++) sum += buf[i];
        if (sum == buf[FRAME_LEN - 1]) {
          uint16_t val = ((uint16_t)buf[5] << 8) | buf[6];
          DBG_PRINT("PM raw: ");
          DBG_PRINTLN(val);
          return val;
        } else {
          DBG_PRINTLN("PM checksum error");
        }
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

