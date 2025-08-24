#pragma once
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <SoftwareSerial.h>
#include "Config.h"

extern Adafruit_BME280 bme;
extern SoftwareSerial pms;

// Vindriktning specific constants (wie in Tasmota)
#define VINDRIKTNING_HEADER1 0x16
#define VINDRIKTNING_HEADER2 0x11
#define VINDRIKTNING_HEADER3 0x0B
#define VINDRIKTNING_PACKET_SIZE 20

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
  static uint8_t buffer[VINDRIKTNING_PACKET_SIZE];
  static int bufferIndex = 0;
  static bool headerFound = false;
  
  // Process available bytes
  while (pms.available()) {
    uint8_t byte = pms.read();
    
    if (!headerFound) {
      // Look for header sequence 0x16 0x11 0x0B (wie in Tasmota)
      if (bufferIndex == 0 && byte == VINDRIKTNING_HEADER1) {
        buffer[bufferIndex++] = byte;
      } else if (bufferIndex == 1 && byte == VINDRIKTNING_HEADER2) {
        buffer[bufferIndex++] = byte;
      } else if (bufferIndex == 2 && byte == VINDRIKTNING_HEADER3) {
        buffer[bufferIndex++] = byte;
        headerFound = true;
      } else {
        bufferIndex = 0;
        if (byte == VINDRIKTNING_HEADER1) {
          buffer[bufferIndex++] = byte;
        }
      }
    } else {
      // Continue filling buffer after header
      buffer[bufferIndex++] = byte;
      
      if (bufferIndex >= VINDRIKTNING_PACKET_SIZE) {
        // Vindriktning uses a different checksum calculation
        // It's the sum of all bytes except the last one, then take the lower byte
        uint16_t checksum = 0;
        for (int i = 0; i < 19; i++) {
          checksum += buffer[i];
        }
        // The checksum is NOT just the lower byte - let's try different methods
        uint8_t calculatedChecksum = (256 - (checksum & 0xFF)) & 0xFF; // Two's complement
        
        if (calculatedChecksum == buffer[19] || (checksum & 0xFF) == buffer[19]) {
          // Extract PM2.5 value - try different positions
          // Position 9-10 (big endian): 03 84 = 900 (90.0)
          uint16_t pm25_pos9 = (buffer[9] << 8) | buffer[10];
          // Position 14-15 (big endian): 2D 02 = 11522 - not realistic
          uint16_t pm25_pos14 = (buffer[14] << 8) | buffer[15];
          // Position 5-6 (big endian): 00 00 = 0
          uint16_t pm25_pos5 = (buffer[5] << 8) | buffer[6];
          
          // The most realistic value is at position 9-10, divide by 10 for µg/m³
          uint16_t pm25 = pm25_pos9 / 10;
          
          DBG_PRINT("Vindriktning PM2.5 raw pos9: ");
          DBG_PRINT(pm25_pos9);
          DBG_PRINT(" -> ");
          DBG_PRINT(pm25);
          DBG_PRINTLN(" µg/m³");
          
          // Reset for next packet
          bufferIndex = 0;
          headerFound = false;
          
          return pm25;
        } else {
          DBG_PRINTLN("Vindriktning checksum error");
          DBG_PRINT("Calculated: ");
          DBG_PRINT(calculatedChecksum);
          DBG_PRINT(" or ");
          DBG_PRINT(checksum & 0xFF);
          DBG_PRINT(", Got: ");
          DBG_PRINTLN(buffer[19]);
          
          // Debug: Print the entire packet
          DBG_PRINT("Packet: ");
          for (int i = 0; i < 20; i++) {
            if (buffer[i] < 16) DBG_PRINT("0");
            DBG_PRINT(buffer[i], HEX);
            DBG_PRINT(" ");
          }
          DBG_PRINTLN();
          
          // For debugging, let's try to extract the value anyway
          uint16_t pm25_debug = ((buffer[9] << 8) | buffer[10]) / 10;
          DBG_PRINT("DEBUG PM2.5 value (ignoring checksum): ");
          DBG_PRINTLN(pm25_debug);
        }
        
        // Reset and try again
        bufferIndex = 0;
        headerFound = false;
      }
    }
  }
  
  return 0; // No valid data available
}

inline void readMeasurements(uint16_t &pm25, float &t, float &h, float &p, const DeviceConfig &cfg) {
  // Read PM2.5 with multiple attempts
  uint16_t rawPM = 0;
  for (int attempts = 0; attempts < 10 && rawPM == 0; attempts++) {
    rawPM = readPM25Raw();
    if (rawPM == 0) {
      delay(100); // Wait a bit before next attempt
    }
  }

  pm25 = rawPM;

  // Read BME280 values
  t = bme.readTemperature() + cfg.tempOffset;
  h = bme.readHumidity();
  p = bme.readPressure() / 100.0F;
}