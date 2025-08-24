#pragma once
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <SoftwareSerial.h>
#include "Config.h"

extern Adafruit_BME280 bme;
extern SoftwareSerial pms;

#define VINDRIKTNING_DATASET_SIZE 20

inline bool initSensors() {
  Wire.begin(D3, D2); // SDA, SCL
  bool ok = bme.begin(0x76);
  pms.begin(9600);
  // Flush any existing data
  while (pms.available()) {
    pms.read();
  }
  if (ok) {
    DBG_PRINTLN("BME280 detected");
  } else {
    DBG_PRINTLN("BME280 missing");
  }
  return ok;
}

inline uint16_t readPM25Raw() {
  // Exact Tasmota implementation with improved packet handling
  if (!pms.available()) {
    return 0;
  }
  
  // Wait for start byte 0x16, skip everything else
  while ((pms.peek() != 0x16) && pms.available()) {
    pms.read();
  }
  
  // Need at least 20 bytes available
  if (pms.available() < VINDRIKTNING_DATASET_SIZE) {
    return 0;
  }

  uint8_t buffer[VINDRIKTNING_DATASET_SIZE];
  pms.readBytes(buffer, VINDRIKTNING_DATASET_SIZE);
  
  // Flush any remaining bytes to prevent packet overlap
  pms.flush();

  // Debug: Print the entire packet
  DBG_PRINT("Vindriktning packet: ");
  for (int i = 0; i < VINDRIKTNING_DATASET_SIZE; i++) {
    if (buffer[i] < 16) DBG_PRINT("0");
    DBG_PRINT(buffer[i], HEX);
    DBG_PRINT(" ");
  }
  DBG_PRINTLN();

  // Tasmota checksum: sum of all 20 bytes should be 0
  uint8_t crc = 0;
  for (uint32_t i = 0; i < VINDRIKTNING_DATASET_SIZE; i++) {
    crc += buffer[i];
  }
  
  if (crc != 0) {
    DBG_PRINT("Vindriktning checksum error, CRC sum: ");
    DBG_PRINTLN(crc);
    return 0; // Don't use invalid data
  }

  // Extract PM2.5 value from bytes 5-6 (big endian) exactly like Tasmota
  // sample data from Tasmota comment:
  //  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19
  // 16 11 0b 00 00 00 0c 00 00 03 cb 00 00 00 0c 01 00 00 00 e7
  //               |pm2_5|     |pm1_0|     |pm10 |        | CRC |
  uint16_t pm25 = (buffer[5] << 8) | buffer[6];
  
  DBG_PRINT("Vindriktning PM2.5 raw: ");
  DBG_PRINTLN(pm25);
  
  return pm25;
}

inline void readMeasurements(uint16_t &pm25, float &t, float &h, float &p, const DeviceConfig &cfg) {
  // Read PM2.5 directly (no multiple attempts needed with Tasmota approach)
  pm25 = readPM25Raw();

  // Read BME280 values
  t = bme.readTemperature() + cfg.tempOffset;
  h = bme.readHumidity();
  p = bme.readPressure() / 100.0F;
}