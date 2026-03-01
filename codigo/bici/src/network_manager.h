#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "FS.h"
#include "SD.h"
#include "config.h"
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>

// Gestiona la conexión WiFi (Módulos de reintentos, WiFiManager)
// Retorna true si se conectó exitosamente
bool connectToWiFi(Adafruit_NeoPixel &pixels);

// Sube un archivo al servidor
// Retorna true si la subida fue exitosa
bool uploadFileToServer(String filename, Adafruit_NeoPixel &pixels);

#endif // NETWORK_MANAGER_H
