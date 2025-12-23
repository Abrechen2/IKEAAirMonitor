#pragma once
#include <Arduino.h>
#include <math.h>

// AQI breakpoints for PM2.5 (US EPA standard)
constexpr float AQI_PM25_BP1 = 12.0f;
constexpr float AQI_PM25_BP2 = 35.4f;
constexpr float AQI_PM25_BP3 = 55.4f;
constexpr float AQI_PM25_BP4 = 150.4f;
constexpr float AQI_PM25_BP5 = 250.4f;
constexpr float AQI_PM25_BP1_START = 12.1f;
constexpr float AQI_PM25_BP2_START = 35.5f;
constexpr float AQI_PM25_BP3_START = 55.5f;
constexpr float AQI_PM25_BP4_START = 150.5f;
constexpr float AQI_PM25_BP5_START = 250.5f;

constexpr uint16_t AQI_VALUE_BP1 = 50;
constexpr uint16_t AQI_VALUE_BP2 = 51;
constexpr uint16_t AQI_VALUE_BP3 = 101;
constexpr uint16_t AQI_VALUE_BP4 = 151;
constexpr uint16_t AQI_VALUE_BP5 = 201;
constexpr uint16_t AQI_VALUE_BP6 = 301;

constexpr float AQI_RANGE_BP1 = 12.0f;
constexpr float AQI_RANGE_BP2 = 23.3f;
constexpr float AQI_RANGE_BP3 = 19.9f;
constexpr float AQI_RANGE_BP4 = 94.9f;
constexpr float AQI_RANGE_BP5 = 99.9f;
constexpr float AQI_RANGE_BP6 = 249.9f;

// Calculate PM2.5 AQI based on US EPA standard
inline uint16_t calculatePM25AQI(uint16_t pm25) {
  if (pm25 <= AQI_PM25_BP1) return (uint16_t)round(pm25 * AQI_VALUE_BP1 / AQI_RANGE_BP1);
  if (pm25 <= AQI_PM25_BP2) return (uint16_t)round(AQI_VALUE_BP2 + (pm25 - AQI_PM25_BP1_START) * 49.0f / AQI_RANGE_BP2);
  if (pm25 <= AQI_PM25_BP3) return (uint16_t)round(AQI_VALUE_BP3 + (pm25 - AQI_PM25_BP2_START) * 49.0f / AQI_RANGE_BP3);
  if (pm25 <= AQI_PM25_BP4) return (uint16_t)round(AQI_VALUE_BP4 + (pm25 - AQI_PM25_BP3_START) * 49.0f / AQI_RANGE_BP4);
  if (pm25 <= AQI_PM25_BP5) return (uint16_t)round(AQI_VALUE_BP5 + (pm25 - AQI_PM25_BP4_START) * 99.0f / AQI_RANGE_BP5);
  return (uint16_t)round(AQI_VALUE_BP6 + (pm25 - AQI_PM25_BP5_START) * 99.0f / AQI_RANGE_BP6);
}

// Constants for dew point calculation (Magnus formula)
constexpr float DEWPOINT_A = 17.27f;
constexpr float DEWPOINT_B = 237.7f;
constexpr float HUMIDITY_MAX = 100.0f;

// Calculate dew point in Celsius
inline float calculateDewPoint(float temp, float humidity) {
  float alpha = ((DEWPOINT_A * temp) / (DEWPOINT_B + temp)) + logf(humidity / HUMIDITY_MAX);
  return (DEWPOINT_B * alpha) / (DEWPOINT_A - alpha);
}

// Comfort index constants
constexpr float COMFORT_TEMP_OPTIMAL = 22.0f;
constexpr float COMFORT_TEMP_MIN = 18.0f;
constexpr float COMFORT_TEMP_MAX = 26.0f;
constexpr float COMFORT_TEMP_PENALTY = 10.0f;
constexpr float COMFORT_HUMIDITY_OPTIMAL = 50.0f;
constexpr float COMFORT_HUMIDITY_MIN = 30.0f;
constexpr float COMFORT_HUMIDITY_MAX = 70.0f;
constexpr float COMFORT_HUMIDITY_PENALTY = 2.0f;
constexpr float COMFORT_SCORE_MAX = 100.0f;
constexpr float COMFORT_SCORE_MIN = 0.0f;
constexpr float COMFORT_SCORE_DIVISOR = 2.0f;

// Calculate comfort index (0-100, higher is better)
inline float calculateComfortIndex(float temp, float humidity) {
  float tempScore = COMFORT_SCORE_MAX;
  float humidityScore = COMFORT_SCORE_MAX;

  // Temperature scoring: optimal around 22°C
  if (temp < COMFORT_TEMP_MIN || temp > COMFORT_TEMP_MAX) {
    tempScore = fmaxf(COMFORT_SCORE_MIN, COMFORT_SCORE_MAX - fabsf(temp - COMFORT_TEMP_OPTIMAL) * COMFORT_TEMP_PENALTY);
  }

  // Humidity scoring: optimal around 50%
  if (humidity < COMFORT_HUMIDITY_MIN || humidity > COMFORT_HUMIDITY_MAX) {
    humidityScore = fmaxf(COMFORT_SCORE_MIN, COMFORT_SCORE_MAX - fabsf(humidity - COMFORT_HUMIDITY_OPTIMAL) * COMFORT_HUMIDITY_PENALTY);
  }

  return (tempScore + humidityScore) / COMFORT_SCORE_DIVISOR;
}

// AQI category thresholds
constexpr uint16_t AQI_CAT1_MAX = 50;
constexpr uint16_t AQI_CAT2_MAX = 100;
constexpr uint16_t AQI_CAT3_MAX = 150;
constexpr uint16_t AQI_CAT4_MAX = 200;

// Get AQI category (1-5)
inline uint8_t getAQICategory(uint16_t aqi) {
  if (aqi <= AQI_CAT1_MAX) return 1;
  if (aqi <= AQI_CAT2_MAX) return 2;
  if (aqi <= AQI_CAT3_MAX) return 3;
  if (aqi <= AQI_CAT4_MAX) return 4;
  return 5;
}

