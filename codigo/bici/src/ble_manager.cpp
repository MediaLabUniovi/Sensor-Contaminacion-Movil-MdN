/**
 * ble_manager.cpp - Módulo de Gestión Bluetooth Low Energy
 * --------------------------------------------------------
 * Responsabilidades:
 * 1. Inicializar Stack BLE y Servidor GATT.
 * 2. Gestionar Callbacks de Conexión/Desconexión.
 * 3. Procesar COMANDOS recibidos desde la App (JSON):
 *    - {"measureInterval": X} -> Cambia tiempo entre medidas.
 *    - {"cmd": "listFiles"} -> Devuelve JSON con archivos en SD.
 *    - {"cmd": "uploadFile", "file": "..."} -> Inicia modo de subida WiFi.
 *    - {"cmd": "deleteFile", "file": "..."} -> Borra archivo en SD.
 * 4. Notificar datos de sensores en tiempo real (Notify Characteristic).
 */

#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <WiFi.h>

#include "ble_manager.h"
#include "config.h"

// Variables Globales propias del BLE
BLECharacteristic *pSensorDataCharacteristic;
BLECharacteristic *pSettingsCharacteristic;
bool deviceConnected = false;
bool bluetoothEnabled =
    false; // Inicio pagado por defecto (se cambia con botón)

// Referencias a variables externas de main.cpp / config
// Necesitamos modificarlas desde los callbacks
extern uint32_t TIEMPO_ENTRE_MEDIDAS;
extern uint32_t TIEMPO_ENTRE_MEDIDAS;
extern String uploadTarget;
extern volatile Mode currentMode;
extern GPSMode currentGpsMode;
String transferTarget =
    ""; // Variable local en este módulo, pero modificable por main si fuera
        // externo (pero lo hemos hecho extern en .h)

// --- Callbacks del Servidor (Conexión/Desconexión) ---
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("Cliente conectado");
  }

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("Cliente desconectado");
    // Solo reiniciar advertising si el bluetooth sigue habilitado por usuario
    if (bluetoothEnabled) {
      BLEDevice::startAdvertising();
    }
  }
};

// --- Callbacks de Configuración (Escritura desde App) ---
class SettingsCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.print("Configuración recibida: ");
      for (int i = 0; i < value.length(); i++)
        Serial.print(value[i]);
      Serial.println();

      String strVal = String(value.c_str());

      // 1. Configuración de Intervalo
      // {"measureInterval": 30}
      int idx = strVal.indexOf("\"measureInterval\":");
      if (idx != -1) {
        int start = idx + 18;
        int end = strVal.indexOf("}", start);
        if (end == -1)
          end = strVal.indexOf(",", start);
        if (end != -1) {
          String numStr = strVal.substring(start, end);
          TIEMPO_ENTRE_MEDIDAS = numStr.toInt() * 1000;
          Serial.print("Nuevo Intervalo (ms): ");
          Serial.println(TIEMPO_ENTRE_MEDIDAS);
        }
      }

      // 1.1 Configuración de Modo GPS
      // {"gpsMode": "continuous"} o {"gpsMode": "interval"}
      int idxGps = strVal.indexOf("\"gpsMode\":");
      if (idxGps != -1) {
        if (strVal.indexOf("continuous", idxGps) != -1) {
          currentGpsMode = GPS_MODE_CONTINUOUS;
          Serial.println("Modo GPS: CONTINUO");
        } else if (strVal.indexOf("interval", idxGps) != -1) {
          currentGpsMode = GPS_MODE_INTERVAL;
          Serial.println("Modo GPS: INTERVALO");
        }
      }

      // 2. Listar Archivos
      // {"cmd": "listFiles"}
      if (strVal.indexOf("\"cmd\":\"listFiles\"") != -1 ||
          strVal.indexOf("\"cmd\": \"listFiles\"") != -1) {
        String fileList = listSDFilesJSON();
        // Respondemos por notification en SensorData
        if (pSensorDataCharacteristic) {
          pSensorDataCharacteristic->setValue(fileList.c_str());
          pSensorDataCharacteristic->notify();
          Serial.println("Enviada lista de archivos: " + fileList);
        }
      }

      // 3. Borrar Archivo
      // {"cmd": "deleteFile", "file": "/data_X.csv"}
      int cmdDelete = strVal.indexOf("\"cmd\":\"deleteFile\"");
      if (cmdDelete == -1)
        cmdDelete = strVal.indexOf("\"cmd\": \"deleteFile\"");

      if (cmdDelete != -1) {
        int idxFile = strVal.indexOf("\"file\":");
        if (idxFile != -1) {
          int start = idxFile + 8;
          if (strVal.charAt(start) == '\"')
            start++;
          int end = strVal.indexOf("\"", start);
          if (end != -1) {
            String fname = strVal.substring(start, end);
            Serial.println("Borrando archivo: " + fname);
            if (deleteSDFile(fname)) {
              Serial.println("Borrado OK");
            } else {
              Serial.println("Archivo no existe");
            }
            // Confirmación
            if (pSensorDataCharacteristic) {
              String msg =
                  "{\"status\":\"deleted\",\"file\":\"" + fname + "\"}";
              pSensorDataCharacteristic->setValue(msg.c_str());
              pSensorDataCharacteristic->notify();
            }
          }
        }
      }

      // 4. Subir Archivo
      // {"cmd": "uploadFile", "file": "/data_X.csv"}
      int cmdUpload = strVal.indexOf("\"cmd\":\"uploadFile\"");
      if (cmdUpload == -1)
        cmdUpload = strVal.indexOf("\"cmd\": \"uploadFile\"");

      if (cmdUpload != -1) {
        int idxFile = strVal.indexOf("\"file\":");
        if (idxFile != -1) {
          int start = idxFile + 8;
          if (strVal.charAt(start) == '\"')
            start++;
          int end = strVal.indexOf("\"", start);
          if (end != -1) {
            String fname = strVal.substring(start, end);
            if (SD.exists(fname)) {
              uploadTarget = fname;
              Serial.println("Solicitado upload de: " + uploadTarget);
              // Cambiar modo a WIFI_CONNECTION
              currentMode = WIFI_CONNECTION;
            }
          }
        }
      }

      // 5. Transferir Archivo a App (Smartphone)
      // {"cmd": "transferFile", "file": "/data_X.csv"}
      int cmdTransfer = strVal.indexOf("\"cmd\":\"transferFile\"");
      if (cmdTransfer == -1)
        cmdTransfer = strVal.indexOf("\"cmd\": \"transferFile\"");

      if (cmdTransfer != -1) {
        int idxFile = strVal.indexOf("\"file\":");
        if (idxFile != -1) {
          int start = idxFile + 8;
          if (strVal.charAt(start) == '\"')
            start++;
          int end = strVal.indexOf("\"", start);
          if (end != -1) {
            String fname = strVal.substring(start, end);
            if (SD.exists(fname)) {
              transferTarget = fname;
              Serial.println("Solicitado transfer manual a App de: " +
                             transferTarget);
              // Cambiar modo a BLE_TRANSFER_FILE
              currentMode = BLE_TRANSFER_FILE;
            } else {
              Serial.println("Archivo no existe para transferir: " + fname);
            }
          }
        }
      }
    }
  }
};

void initBLE() {
  uint8_t macBT[6];
  // Leer específicamente la MAC asignada a Bluetooth
  esp_read_mac(macBT, ESP_MAC_BT);

  char finalMac[5];
  // Extraer como Texto Hexadecimal los últimos 2 bytes (equivale a 4
  // caracteres)
  sprintf(finalMac, "%02X%02X", macBT[4], macBT[5]);

  String deviceName = "AirQ-" + String(finalMac);

  BLEDevice::init(deviceName.c_str());
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Sensor Data
  pSensorDataCharacteristic = pService->createCharacteristic(
      SENSOR_DATA_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pSensorDataCharacteristic->addDescriptor(new BLE2902());

  // Settings
  pSettingsCharacteristic = pService->createCharacteristic(
      SETTINGS_UUID, BLECharacteristic::PROPERTY_WRITE);
  pSettingsCharacteristic->setCallbacks(new SettingsCallbacks());

  pService->start();

  // Advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("BLE Iniciado y esperando conexiones (Modo IDLE).");
}

void startBLEAdvertising() { BLEDevice::startAdvertising(); }

void stopBLEAdvertising() { BLEDevice::getAdvertising()->stop(); }

void notifySensorData(float temp, float hum, float pm25, float pm10, float bat,
                      float lat, float lon) {
  if (bluetoothEnabled && deviceConnected && pSensorDataCharacteristic) {
    String jsonPayload = "{";
    jsonPayload += "\"temp\":" + String(temp, 1) + ",";
    jsonPayload += "\"hum\":" + String(hum, 1) + ",";
    jsonPayload += "\"pm25\":" + String(pm25, 1) + ",";
    jsonPayload += "\"pm10\":" + String(pm10, 1) + ",";
    jsonPayload += "\"bat\":" + String(bat) + ",";
    jsonPayload += "\"lat\":" + String(lat, 6) + ",";
    jsonPayload += "\"lon\":" + String(lon, 6);
    jsonPayload += "}";

    pSensorDataCharacteristic->setValue(jsonPayload.c_str());
    pSensorDataCharacteristic->notify();
    Serial.println("[BLE] Enviado: " + jsonPayload);
  }
}

void notifyGPSStatus(String status) {
  if (deviceConnected && pSensorDataCharacteristic) {
    String statusJson = "{\"status\":\"" + status + "\"}";
    pSensorDataCharacteristic->setValue(statusJson.c_str());
    pSensorDataCharacteristic->notify();
    Serial.println(">> Notificado: " + status);
  }
}

void notifyFileStart(String filename, size_t size) {
  if (deviceConnected && pSensorDataCharacteristic) {
    String json = "{\"type\":\"fileStart\",\"file\":\"" + filename +
                  "\",\"size\":" + String(size) + "}";
    pSensorDataCharacteristic->setValue(json.c_str());
    pSensorDataCharacteristic->notify();
    Serial.println("[BLE] File Start: " + filename);
    delay(50); // Pequeña pausa para asegurar recepción
  }
}

void notifyFileData(String line) {
  if (deviceConnected && pSensorDataCharacteristic) {
    // Escapar comillas dobles si las hubiera en el contenido (CSV no suele,
    // pero por seguridad) Pero como es un CSV simple, asumimos que no rompe el
    // JSON. Enviamos: {"type":"fileData","line":"...contenido..."}
    String json = "{\"type\":\"fileData\",\"line\":\"" + line + "\"}";
    pSensorDataCharacteristic->setValue(json.c_str());
    pSensorDataCharacteristic->notify();
    // No ponemos Serial.println aquí para no saturar el log
    delay(10); // Pausa vital para evitar saturar la cola BLE del móvil
  }
}

void notifyFileEnd() {
  if (deviceConnected && pSensorDataCharacteristic) {
    String json = "{\"type\":\"fileEnd\"}";
    pSensorDataCharacteristic->setValue(json.c_str());
    pSensorDataCharacteristic->notify();
    Serial.println("[BLE] File End");
  }
}
