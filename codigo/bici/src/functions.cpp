/*
 * Implementación de funciones auxiliares
 * Sensor de Contaminación Móvil Reto TICLAB Mar de Niebla 2025
 *
 * Autor: José Luis Muñiz Traviesas, Miguel Enterría, Andrés Vilas Grela;
 * 28/01/25
 */

#include "functions.h"
#include "SD.h"
#include "SPI.h"
#include <WiFi.h>
#include <sps30.h>

// ==================== Funciones de Batería ====================
float readBatteryVoltage() {
  int numSamples = 20;
  long totalAdc = 0;

  for (int i = 0; i < numSamples; i++) {
    totalAdc += analogRead(BATTERY_PIN);
    delay(2);
  }

  float avgAdc = (float)totalAdc / numSamples;
  float voltage = (avgAdc / (float)ADC_RESOLUTION) * ADC_REFERENCE_VOLTAGE *
                  VOLTAGE_DIVIDER_RATIO;
  return voltage;
}

float readBatteryPercentage() {
  float voltage = readBatteryVoltage();
  // Mapa aproximado para LiPo 1S:
  // 4.2V -> 100%
  // 3.0V -> 0%
  float percentage = (voltage - 3.0) * (100.0 / (4.2 - 3.0));

  if (percentage > 100.0)
    percentage = 100.0;
  if (percentage < 0.0)
    percentage = 0.0;

  return percentage;
}

// ==================== Funciones de LEDs ====================
void ledSequence(Mode fromMode, Adafruit_NeoPixel &pixels) {
  pixels.clear();
  pixels.show();

  switch (fromMode) {
  case IDLE: // Transición IDLE -> DATA_RECOLLECTION
    for (uint8_t i = 0; i < 2; i++) {
      pixels.setPixelColor(0, COLOR_AMARILLO);
      pixels.show();
      delay(500);
      pixels.setPixelColor(0, 0);
      pixels.show();
      delay(500);
    }
    break;

  case DATA_RECOLLECTION: // Transición DATA_RECOLLECTION -> WIFI_CONNECTION
    for (int dir = 0; dir < 2; dir++) {
      if (dir == 0) {
        for (uint16_t i = 0; i < NUMPIXELS; i++) {
          pixels.clear();
          pixels.setPixelColor(i, COLOR_AZUL);
          pixels.show();
          delay(250);
        }
      } else {
        for (int16_t i = NUMPIXELS - 1; i >= 0; i--) {
          pixels.clear();
          pixels.setPixelColor(i, COLOR_AZUL);
          pixels.show();
          delay(250);
        }
      }
    }
    break;

  case WIFI_CONNECTION: // Transición WIFI_CONNECTION -> SEND_DATA
    for (uint8_t k = 0; k < 4; k++) {
      for (uint16_t i = 0; i < NUMPIXELS; i++)
        pixels.setPixelColor(i, COLOR_VERDE);
      pixels.show();
      delay(200);
      for (uint16_t i = 0; i < NUMPIXELS; i++)
        pixels.setPixelColor(i, COLOR_ROJO);
      pixels.show();
      delay(200);
    }
    break;

  default:
    pixels.clear();
    pixels.setPixelColor(0, COLOR_VERDE);
    pixels.show();
    break;
  }
}

void blinking_led_sequence(uint32_t color, uint8_t times, uint16_t delayMs,
                           Adafruit_NeoPixel &pixels) {
  for (int i = 0; i < times; i++) {
    pixels.clear();
    pixels.fill(color);
    pixels.show();
    delay(delayMs);
    pixels.clear();
    pixels.show();
    delay(delayMs);
  }
}

void updateStatusLEDs(float pm2_5, float pm_10, int16_t ret, bool bme_ok,
                      bool SD_ok, float battery_pct, uint8_t gps_status,
                      Adafruit_NeoPixel &pixels) {
  pixels.clear();

  // LED 0 - Estado del sistema
  bool sistema_ok = true;
  if (ret < 0 || !bme_ok || !SD_ok) {
    sistema_ok = false;
  }
  if (sistema_ok) {
    pixels.setPixelColor(0, COLOR_VERDE);
  } else {
    pixels.setPixelColor(0, COLOR_ROJO);
  }

  // LED 1 - Batería
  if (battery_pct > 60.0) {
    pixels.setPixelColor(1, COLOR_VERDE);
  } else if (battery_pct >= 20.0) {
    pixels.setPixelColor(1, COLOR_AMARILLO);
  } else {
    pixels.setPixelColor(1, COLOR_ROJO);
  }

  // LED 2 - GPS
  // gps_status: 0=Error/Vacio, 1=SIMULATE_GPS, 2=FIX REAL, 3=Usando RTC Interno
  if (gps_status == 1) {
    pixels.setPixelColor(2, COLOR_CIAN);
  } else if (gps_status == 2) {
    pixels.setPixelColor(2, COLOR_VERDE);
  } else if (gps_status == 3) {
    pixels.setPixelColor(2, COLOR_AMARILLO);
  } else { // 0 o cualquier otro caso de fallo
    pixels.setPixelColor(2, COLOR_ROJO);
  }

  // LED 3 - PM2.5
  if (pm2_5 <= 12.0) {
    pixels.setPixelColor(3, COLOR_VERDE);
  } else if (pm2_5 <= 35.4) {
    pixels.setPixelColor(3, COLOR_NARANJA);
  } else {
    pixels.setPixelColor(3, COLOR_ROJO);
  }

  // LED 4 - PM10
  if (pm_10 <= 54.0) {
    pixels.setPixelColor(4, COLOR_VERDE);
  } else if (pm_10 <= 154.0) {
    pixels.setPixelColor(4, COLOR_NARANJA);
  } else {
    pixels.setPixelColor(4, COLOR_ROJO);
  }

  pixels.show();
}

// ==================== Funciones de Sensores ====================
bool initBME280(Adafruit_BME280 &bme, TwoWire &i2c, bool &bme_ok) {
  i2c.setPins(BME_SDA, BME_SCL);
  i2c.begin();

  bme_ok = true;

  if (!bme.begin(0x76, &i2c) && !bme.begin(0x77, &i2c)) {
    Serial.println("Could not find BME280 sensor!");
    bme_ok = false;
    return false;
  }

  Serial.println("BME280 encontrado. Humedad disponible");
  return true;
}

bool initSPS30(int16_t &ret) {
  sensirion_i2c_init();

  while (sps30_probe() != 0) {
    Serial.print("SPS sensor probing failed\n");
    delay(500);
  }

#ifndef PLOTTER_FORMAT
  Serial.print("SPS sensor probing successful\n");
#endif

  ret = sps30_set_fan_auto_cleaning_interval_days(SPS30_AUTO_CLEAN_DAYS);
  if (ret) {
    Serial.print("error setting the auto-clean interval: ");
    Serial.println(ret);
    return false;
  }

  ret = sps30_start_measurement();
  if (ret < 0) {
    Serial.print("error starting measurement\n");
    return false;
  }

#ifndef PLOTTER_FORMAT
  Serial.print("measurements started\n");
#endif

  return true;
}

bool initSDCard() {
  SPI.begin(CLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

  while (!SD.begin(CS_PIN)) {
    Serial.println("[SD] Montaje de tarjeta SD fallido.");
    delay(2000);
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("[SD] No hay tarjeta SD.");
    return false;
  }

  Serial.print("[SD] SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.println("[SD] SD Card Size: " + String(cardSize) + "MB");

  return true;
}

// ==================== Funciones de GPS y RTC ====================
void syncRTCWithGPS(int year, uint8_t month, uint8_t day, uint8_t hour,
                    uint8_t minute, uint8_t second) {
  struct tm timeinfo;
  timeinfo.tm_year = year - 1900; // tm_year es años desde 1900
  timeinfo.tm_mon = month - 1;    // tm_mon es 0-11
  timeinfo.tm_mday = day;
  timeinfo.tm_hour = hour + TIME_OFFSET_H;
  timeinfo.tm_min = minute;
  timeinfo.tm_sec = second;

  time_t t = mktime(&timeinfo);
  struct timeval now = {.tv_sec = t};
  settimeofday(&now, NULL);

  Serial.println("[RTC] Sincronizado con GPS");
}

String getCurrentTimeString() {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min,
          timeinfo.tm_sec);
  return String(timeStr);
}

String getCurrentDateString() {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  return String(timeinfo.tm_year + 1900) + ";" + String(timeinfo.tm_mon + 1) +
         ";" + String(timeinfo.tm_mday);
}

// ==================== Funciones de Datos ====================
String readBMEData(Adafruit_BME280 &bme, bool bme_ok) {
  String data = "";

  if (bme_ok) {
    float t = bme.readTemperature();
    float p = bme.readPressure() / 100.0F;
    float h = bme.readHumidity();
    data = String(t, 2) + ";" + String(p, 2) + ";" + String(h, 2) + ";";
  } else {
    Serial.println("BME no inicializado, no es posible recoger datos");
    data = "NAN;NAN;NAN;";
  }

  return data;
}

String readParticlesData(float &pm2_5, float &pm_10, int16_t &ret) {
  uint16_t ready = 0;
  ret = sps30_read_data_ready(&ready);

  if (ret < 0) {
    Serial.printf("error data_ready: %d\n", ret);
    return "";
  }

  if (!ready) {
    return ""; // No hay datos listos todavía
  }

  struct sps30_measurement pm;
  ret = sps30_read_measurement(&pm);

  if (ret >= 0) {
    pm2_5 = pm.mc_2p5;
    pm_10 = pm.mc_10p0;

    return String(pm.mc_2p5) + ";" + String(pm.mc_10p0) + ";" +
           WiFi.macAddress() + "\n";
  }

  return "";
}
