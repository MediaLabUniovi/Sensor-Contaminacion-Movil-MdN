/*
 * Funciones auxiliares
 * Sensor de Contaminación Móvil Reto TICLAB Mar de Niebla 2025
 *
 * Autor: José Luis Muñiz Traviesas, Miguel Enterría, Andrés Vilas Grela; 28/01/25
*/

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "Arduino.h"
#include <Adafruit_NeoPixel.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <TinyGPS.h>
#include <time.h>
#include "config.h"

// Enum para estados de la máquina principal
enum Mode: uint8_t {
  IDLE = 0,
  PROCESS_LONG_PRESS,
  DATA_RECOLLECTION,
  WIFI_CONNECTION,
  SEND_DATA
};

// ==================== Funciones de Batería ====================
float readBatteryVoltage();

// ==================== Funciones de LEDs ====================
void ledSequence(Mode fromMode, Adafruit_NeoPixel& pixels);
void blinking_led_sequence(uint32_t color, uint8_t times, uint16_t delayMs, Adafruit_NeoPixel& pixels);
void updateStatusLEDs(float pm2_5, float pm_10, int16_t ret, bool bme_ok, bool bmp_ok, bool SD_ok, Adafruit_NeoPixel& pixels);

// ==================== Funciones de Sensores ====================
bool initBME280(Adafruit_BME280& bme, Adafruit_BMP280& bmp, TwoWire& i2c, bool& bme_ok, bool& bmp_ok);
bool initSPS30(int16_t& ret);
bool initSDCard();

// ==================== Funciones de GPS y RTC ====================
void syncRTCWithGPS(int year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
String getCurrentTimeString();
String getCurrentDateString();

// ==================== Funciones de Datos ====================
String readBMEData(Adafruit_BME280& bme, Adafruit_BMP280& bmp, bool bme_ok, bool bmp_ok);
String readParticlesData(float& pm2_5, float& pm_10, int16_t& ret);

#endif // FUNCTIONS_H
