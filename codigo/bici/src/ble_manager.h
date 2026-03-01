#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include "config.h"
#include "functions.h"
#include "sd_manager.h"
#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// Variables globales controladas por este módulo pero accesibles
extern bool deviceConnected;
extern bool bluetoothEnabled;
extern String transferTarget;

// Inicializa el servicio BLE
void initBLE();

// Control de Advertising
void startBLEAdvertising();
void stopBLEAdvertising();

// Notificaciones
void notifySensorData(float temp, float hum, float pm25, float pm10, float bat,
                      float lat, float lon);
void notifyGPSStatus(String status);
// Notificaciones de Transferencia de Archivos
void notifyFileStart(String filename, size_t size);
void notifyFileData(String line);
void notifyFileEnd();

#endif // BLE_MANAGER_H
