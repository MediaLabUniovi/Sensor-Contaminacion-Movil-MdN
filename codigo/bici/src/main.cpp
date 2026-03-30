/*
 * Código proyecto de Sensor de Contaminación Móvil Reto TICLAB Mar de Niebla
 * 2025
 *
 * 28/01/25
 *
 * USO DEL DISPOSITIVO:
 * - Click Corto: Alterna entre Modo Normal (LED Verde) y Modo Bluetooth (LED
 * Azul).
 * - Click Largo (2s): Inicia/Detiene la recolección de datos y grabación en SD.
 *   - En Modo Bluetooth, los datos también se envían a la App móvil en tiempo
 * real.
 */
// GPS
#include "Arduino.h"
#include <TinyGPS.h> // https://github.com/neosarchizo/TinyGPS
#include <time.h>    // Para manejo de RTC interno

// Tarjeta SD - Gestionada por sd_manager y mdc_contaminacion
#include <Preferences.h>
Preferences preferences;

// SPS30 (Sensor PM)
#include <sps30.h> // https://github.com/Sensirion/arduino-sps

// BME280 (Temperatura, Presión & Humedad)
#include <Adafruit_BME280.h>
#include <Wire.h>

// LEDs
#include <Adafruit_NeoPixel.h>

// WiFi - Gestionado por network_manager
// Se mantienen includes mínimos si fueran necesarios, pero network_manager.h ya
// los incluye.

// BLE Libraries - Gestionado por ble_manager

// Configuración
#include "config.h"

// Funciones
#include "LongPress.h"
#include "ble_manager.h"
#include "functions.h"
#include "mdc_contaminacion.hpp"
#include "network_manager.h"
#include "sd_manager.h"
//-------------------------- Definición de enums --------------------------
enum DataRecolectStage : uint8_t { // Enumeración para recolección de datos
  DC_IDLE = 0,
  DC_READ_GPS,
  DC_PROCESS_GPS,
  DC_READ_BME,
  DC_READ_PARTICLES,
  DC_WRITE_SD,
  DC_UPDATE_LEDS,
  DC_WAIT,
  DC_TRY_AGAIN
};
//------------------------- Configuración LongPress -----------------------
LongPressConfig lpCfg = {.buttonPin = USER_BUTTON,
                         .sampleIntervalMs = SAMPLE_INTERVAL_MS,
                         .stableSamplesRequired = STABLE_SAMPLES_REQUIRED,
                         .longPressThresholdMs = LONG_PRESS_THRESHOLD_MS};
LongPressState lpState;

//-------------------------- Variables globales ----------------------------
// Estados de sensores
bool bme_ok = false;
bool SD_ok = false;
uint32_t TIEMPO_ENTRE_MEDIDAS = 30 * 1000; // Valor por defecto 30 segundos

// BLE Global Variables - Gestionado por ble_manager (deviceConnected,
// bluetoothEnabled son extern) bool deviceConnected = false; - MOVED TO
// BLE_MANAGER bool bluetoothEnabled = false; - MOVED TO BLE_MANAGER

// LEDs
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// BME280
TwoWire I2CBME(1);   // Usar bus I2C 1
Adafruit_BME280 bme; // Objeto BME280

// Módulo GPS
TinyGPS GPS;                     // Objeto GPS
uint32_t time_to_connect_ms = 0; // Medida del tiempo de conexión a satélite
bool gps_connected =
    false; // Flag para establecer que ya ha habido una conexión
bool rtc_synced =
    false; // Flag para indicar si el RTC ya fue sincronizado con GPS
// Máquina de estados
volatile Mode currentMode = IDLE;
volatile Mode previousMode = IDLE;
int16_t ret;
float pm2_5;
float pm_10;

// --- Variables Globales para Gestión de Archivos ---
String currentFilename = "";
String uploadTarget = "";
// Modo GPS (Continuo por defecto)
GPSMode currentGpsMode = GPS_MODE_CONTINUOUS;
// ---------------------------------------------------

// BLE Callbacks Y SettingsCallbacks MOVIDOS A ble_manager.cpp

void IRAM_ATTR isr_button_pressed() {
  detachInterrupt(digitalPinToInterrupt(USER_BUTTON));
  previousMode = currentMode;
  currentMode = PROCESS_LONG_PRESS;
}

void setup() {
  // Inicializar NeoPixel
  pixels.begin();
  pixels.setBrightness(BRIGHTNESS);
  pixels.clear();
  // LED 1 verde (sistema encendido)
  pixels.setPixelColor(0, COLOR_VERDE);
  pixels.show();
  // Puerto serie hardware
  Serial.begin(HARDWARE_BAUDRATE);
  Serial.print("Hardware Serial baudrate: ");
  Serial.println(HARDWARE_BAUDRATE);
  // Puerto serie software
  Serial2.begin(GPS_BAUDRATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.print("GPS Serial2 baudrate: ");
  Serial.println(GPS_BAUDRATE);
  delay(500);
  pinMode(USER_BUTTON, INPUT_PULLUP);
  pinMode(BATTERY_PIN, INPUT); // Configurar pin de batería como entrada
  attachInterrupt(digitalPinToInterrupt(USER_BUTTON), isr_button_pressed,
                  FALLING);

  // ----------------- LEER AJUSTES NVM -------------
  preferences.begin("airq_config", false);
  TIEMPO_ENTRE_MEDIDAS = preferences.getUInt("interval", 30000);
  currentGpsMode = (GPSMode)preferences.getUInt("gpsMode", 1); // 1 = GPS_MODE_CONTINUOUS

  // ----------------- INICIALIZAR BLE -----------------
  initBLE();

  // ----------------- INICIALIZAR SENSORES -----------------
  initBME280(bme, I2CBME, bme_ok);
  delay(2000);

  // Sensor partículas
  initSPS30(ret);
  delay(500);

  // Tarjeta SD
  SD_ok = initSDCard();
  if (!SD_ok) {
    Serial.println("[SD] Error crítico: No se pudo inicializar la SD.");
    while (1)
      delay(1000);
  }
}

// Variables para temporizar el color puro tras click corto en IDLE
unsigned long modeShowTimer = 0;
bool showingMode = false;

void loop() {
  switch (currentMode) {
  case IDLE: {
    static bool once = true;

    if (once) {
      once = false;
      Serial.println("Sistema Listo. Esperando comando o pulsación.");
    }

    // Check if we need to show the mode color for 2 seconds after a short press
    if (showingMode) {
      if (millis() - modeShowTimer < 2000) {
        if (bluetoothEnabled) {
          pixels.setPixelColor(0, COLOR_AZUL);
        } else {
          pixels.setPixelColor(0, COLOR_VERDE);
        }
      } else {
        showingMode = false; // 2 seconds have passed
      }
    }

    if (!showingMode) {
      // Normal IDLE behavior: Check for pending files efficiently
      bool hasPending = hasPendingFiles();

      // Determine base state: fail (RED), pending (PURPLE), or OK (Mode Color)
      if (ret < 0 || !bme_ok || !SD_ok) {
        pixels.setPixelColor(0, COLOR_ROJO);
      } else if (hasPending) {
        pixels.setPixelColor(0, COLOR_MORADO);
      } else {
        if (bluetoothEnabled) {
          pixels.setPixelColor(0, COLOR_AZUL);
        } else {
          pixels.setPixelColor(0, COLOR_VERDE);
        }
      }
    }

    // Keep LED 1 green to indicate system is ON (or according to battery if
    // needed, currently VERDE as per base code)
    pixels.setPixelColor(1, COLOR_VERDE);
    pixels.show();
    break;
  }

  case PROCESS_LONG_PRESS: {
    LongPressResult res = checkLongPress(lpState, lpCfg);
    if (res == LONG_PRESS_DETECTED) {
      Serial.println(">> Pulsación larga detectada");

      // Decide transición
      Mode from = previousMode;
      Mode to = IDLE;
      if (from == IDLE) {
        to = DATA_RECOLLECTION;
        // ---------------- GESTIÓN BLE AL INICIAR RUTA ----------------
        if (!bluetoothEnabled) {
          // Si el modo elegido es NORMAL (Verde), apagamos el BLE para ahorrar
          // batería
          Serial.println(">> Iniciando ruta en Modo NORMAL: Apagando BLE.");
          stopBLEAdvertising();
        } else {
          Serial.println(
              ">> Iniciando ruta en Modo BLUETOOTH: BLE se mantiene activo.");
          // Feedback Inmediato: Notificar que estamos buscando GPS
          // Feedback Inmediato: Notificar que estamos buscando GPS
          notifyGPSStatus("searching_gps");
        }

        // --- GESTIÓN ARCHIVOS SD (Inicio de Sesión) ---
        // Rotamos y creamos nuevo archivo UNA VEZ por sesión
        currentFilename = performFileRotation();
        writeCSVHeader(currentFilename);
        // ----------------------------------------------

        // ------------------------------------------------------------
      } else if (from == DATA_RECOLLECTION) {
        // ---------------- GESTIÓN BLE AL TERMINAR RUTA ----------------
        if (!bluetoothEnabled) {
          // Modo NORMAL: Al salir, activamos BLE y vamos a WIFI
          to = WIFI_CONNECTION;
          Serial.println(
              ">> Fin de ruta (Normal): Reactivando BLE e intentando WiFi.");
          startBLEAdvertising();
          uploadTarget = currentFilename;
        } else {
          // Modo BLUETOOTH: Al salir, volvemos directamente a IDLE
          // El usuario gestiona la subida desde la App si quiere
          to = IDLE;
          Serial.println(">> Fin de ruta (BLE): Volviendo a IDLE.");
        }
        // ------------------------------------------------------------

        // ------------------------------------------------------------
      } else
        to = IDLE;

      // Muestra la secuencia según el modo de origen
      ledSequence(from, pixels);

      // Aplica el nuevo modo
      currentMode = to;

      Serial.print("Cambiando a modo ");
      Serial.println(currentMode);
      attachInterrupt(digitalPinToInterrupt(USER_BUTTON), isr_button_pressed,
                      FALLING);
    } else if (res == PRESS_CANCELLED) {
      // ---------------- LOGICA MODO BLUETOOTH (Short Press) ----------------
      // Solo cambiamos la "intención" (flag) y el LED.
      // El Bluetooth sigue activo en IDLE para permitir configuración.

      bluetoothEnabled = !bluetoothEnabled; // Alternar preferencia

      showingMode = true;
      modeShowTimer = millis();

      if (bluetoothEnabled) {
        Serial.println(
            ">> Modo seleccionado: BLUETOOTH (Se mantendrá activo al iniciar)");
        pixels.setPixelColor(0, COLOR_AZUL);
      } else {
        Serial.println(">> Modo seleccionado: NORMAL (Se apagará al iniciar)");
        pixels.setPixelColor(0, COLOR_VERDE);
      }
      pixels.show();

      currentMode = previousMode; // Volvemos al modo donde estábamos
      Serial.print("Volviendo a modo ");
      Serial.println(currentMode);
      attachInterrupt(digitalPinToInterrupt(USER_BUTTON), isr_button_pressed,
                      FALLING);
    }
    break;
  }

  case DATA_RECOLLECTION: {
    // Variables estáticas para este caso
    static uint8_t dcStage = 0;
    static uint32_t dcTimestamp = 0;
    static bool newDataFlag = false;
    static String dataBuffer = "";
    static float last_lat = 0.0;
    static float last_lon = 0.0;

    // Permitir detectar un nuevo long‑press en cualquier momento
    LongPressResult lpRes = checkLongPress(lpState, lpCfg);
    if (lpRes == LONG_PRESS_DETECTED) {
      // Salimos inmediatamente para reentrar en PROCESS_LONG_PRESS
      Serial.println(
          ">> Long-press dentro de DATA_RECOLECTION, interrumpiendo");
      currentMode = PROCESS_LONG_PRESS;
      attachInterrupt(digitalPinToInterrupt(USER_BUTTON), isr_button_pressed,
                      FALLING);
      // Reinciamos etapa para la próxima vez
      dcStage = DC_IDLE;
      break;
    }
    switch (dcStage) {
    case DC_IDLE: {
      // (Rotación movida a PROCESS_LONG_PRESS para ejecutarse una sola vez al
      // inicio)

      // Feedback visual del inicio del ciclo de recogida
      pixels.setPixelColor(0, COLOR_NARANJA);
      pixels.show();

      Serial.println("[GPS] Iniciando lectura 1s...");
      newDataFlag = false;
      dataBuffer = "";
      dcTimestamp = millis();
      dcStage = DC_READ_GPS;
      break;
    }

    case DC_READ_GPS:
      if (currentGpsMode == GPS_MODE_SIMULATED) {
        Serial.println(
            "[GPS-SIM] Saltando lectura HW, usando coordenadas simuladas...");
        newDataFlag = true;
        dcStage = DC_PROCESS_GPS;
      } else {
        // 5 segundos de "muestreo" GPS para dar tiempo a conectar en cold-starts
        while (Serial2.available()) {
          char c = Serial2.read();
          if (GPS.encode(c)) {
            Serial.println("[GPS] NMEA decodificada antes de timeout");
            newDataFlag = true;
            dcStage = DC_PROCESS_GPS;
            break;
          }
        }
        if (millis() - dcTimestamp >= 5000) {
          Serial.println("[GPS] Timeout lectura GPS");
          dcStage = DC_PROCESS_GPS;
        }
      }
      break;

    case DC_PROCESS_GPS:
      // Procesamos datos GPS (si los hay)
      if (newDataFlag) {
        if (currentGpsMode == GPS_MODE_SIMULATED) {
          last_lat = 43.53573;
          last_lon = -5.66152;
          dataBuffer += String(last_lat, 6) + ";" + String(last_lon, 6) + ";";
          // Simulamos también que el RTC interno está correcto (o lo usarmos tal
          // cual)
          if (!rtc_synced) {
            // Sincronizar con hora fija o dejar que use 00:00 si es boot
            syncRTCWithGPS(2025, 6, 15, 12, 0, 0);
            rtc_synced = true;
          }
          dataBuffer +=
              getCurrentDateString() + ";" + getCurrentTimeString() + ";";
          dcStage = DC_READ_BME;
        } else {
          float lat, lon;
          unsigned long age;
          int year;
          uint8_t month, day, hour, minute, second;
          GPS.f_get_position(&lat, &lon, &age);
          if (age != TinyGPS::GPS_INVALID_AGE) {
            last_lat = lat;
            last_lon = lon;
          }
          dataBuffer += String(last_lat, 6) + ";" + String(last_lon, 6) + ";";
          GPS.crack_datetime(&year, &month, &day, &hour, &minute, &second, NULL,
                             NULL);

          // Sincronizar RTC interno la primera vez que obtenemos datos GPS
          // válidos
          if (!rtc_synced) {
            syncRTCWithGPS(year, month, day, hour, minute, second);
            rtc_synced = true;
          }

          // Obtener fecha y hora del RTC interno
          dataBuffer +=
              getCurrentDateString() + ";" + getCurrentTimeString() + ";";
          dcStage = DC_READ_BME;
        }
      } else {
        // Si no hay GPS pero el RTC ya fue sincronizado, usar el RTC
        if (rtc_synced) {
          Serial.println("[GPS] Sin señal, usando última posición conocida");
          // Usar la última coordenada conocida en lugar de 0,0
          dataBuffer += String(last_lat, 6) + ";" + String(last_lon, 6) + ";";
          dataBuffer +=
              getCurrentDateString() + ";" + getCurrentTimeString() + ";";
          // IMPORTANTE: Tenemos datos válidos (tiempo RTC + Sensores que
          // leeremos)
          newDataFlag = true;
          dcStage = DC_READ_BME;
        } else {
          dcStage = DC_UPDATE_LEDS;
        }
      }
      break;

    case DC_READ_BME: {
      dataBuffer += readBMEData(bme, bme_ok);
      dcStage = DC_READ_PARTICLES;
      break;
    }

    case DC_READ_PARTICLES: {
      String particlesData = readParticlesData(pm2_5, pm_10, ret);
      if (!particlesData.isEmpty()) {
        dataBuffer += particlesData;
        dcStage = DC_WRITE_SD;
      }
      // Si no hay datos listos, nos quedamos en este estado
      break;
    }

    case DC_WRITE_SD:
      // Lectura BME280 y escritura en SD
      // USAMOS currentFilename
      appendFile(SD, currentFilename.c_str(), dataBuffer.c_str());
      Serial.println("[SD] Datos escritos: " + dataBuffer);
      dcStage = DC_UPDATE_LEDS;
      break;

    case DC_UPDATE_LEDS: {
      // Actualizo LEDs de debug (sin bloqueos)
      if (newDataFlag) {

        // Determinar status GPS para los LEDs: 0=Error/Vacio, 1=SIMULATE_GPS,
        // 2=FIX REAL, 3=Usando RTC Interno
        uint8_t current_gps_status = 0;
        if (currentGpsMode == GPS_MODE_SIMULATED) {
          current_gps_status = 1;
          last_lat = 43.53573; // Asegurar consistencia
          last_lon = -5.66152;
        } else {
          unsigned long age;
          float temp_lat, temp_lon;
          GPS.f_get_position(&temp_lat, &temp_lon, &age);
          if (age != TinyGPS::GPS_INVALID_AGE &&
              age < 1500) { // Fix reciente (< 1.5s)
            current_gps_status = 2;
          } else if (rtc_synced) { // Sin fix pero con RTC
            current_gps_status = 3;
          } else { // Sin nada
            current_gps_status = 0;
          }
        }

        updateStatusLEDs(pm2_5, pm_10, ret, bme_ok, SD_ok,
                         readBatteryPercentage(), current_gps_status, pixels);

        // --- BLE NOTIFICATION ---
        notifySensorData(bme.readTemperature(), bme.readHumidity(), pm2_5,
                         pm_10, readBatteryPercentage(), last_lat, last_lon);
        // ------------------------
        // ------------------------

        dcStage = DC_WAIT;
      } else { // Si no hay GPS secuencia de LEDS y volvemos a Ver si hay señal
               // GPS
        blinking_led_sequence(COLOR_ROJO, 3, 250, pixels);
        dcStage = DC_TRY_AGAIN;
      }
      dcTimestamp = millis();

      break;
    }

    case DC_WAIT:
      // Esperamos 5 s antes de reiniciar la recolección
      if (millis() - dcTimestamp >= TIEMPO_ENTRE_MEDIDAS) {
        //% Si quisieramos dormir el micro sería aquí, luego al despertarse
        // miramos la causa, si es por tiempo saltamos directamente al data
        // recollection y si es por botón miraríamos si pasar a wifi o no.
        Serial.println("Volvemos a IDLE");
        dcStage = DC_IDLE; // volvemos al principio del flujo GPS→sensores
      } else {
        // --- LOGICA MODO CONTINUO ---
        // Si estamos en modo continuo, seguimos leyendo del puerto serie del
        // GPS para que no se llene el buffer y TinyGPS tenga datos frescos
        // siempre. Leer en bloques para procesar más datos por iteración
        if (currentGpsMode == GPS_MODE_CONTINUOUS) {
          uint16_t bytes_read = 0;
          while (Serial2.available() && bytes_read < 500) {
            GPS.encode(Serial2.read());
            bytes_read++;
          }
        }
      }
      break;

    case DC_TRY_AGAIN:
      // Si venimos de un fallo de coenxión del GPS, solo esperamos 5 segundos.
      if (millis() - dcTimestamp >= 5000) {
        Serial.println("Volvemos a IDLE");
        dcStage = DC_IDLE; // volvemos al principio del flujo GPS→sensores
      }
      break;
    }
    break;
  }

  case WIFI_CONNECTION: {
    pixels.clear();
    pixels.setPixelColor(0, COLOR_VERDE);
    pixels.show();

    // Delegamos en network_manager
    // IMPORTANT: Deinit BLE to free radio/RAM for WiFi
    if (bluetoothEnabled) {
      BLEDevice::deinit(true);
      delay(500);
    }

    if (connectToWiFi(pixels)) {
      currentMode = SEND_DATA;
    } else {
      // Si falla la conexión, restauramos BLE si estaba activo y volvemos a
      // IDLE
      if (bluetoothEnabled) {
        initBLE();
      }
      currentMode = IDLE;
    }
    break;
  }

  case SEND_DATA: {
    // Delegamos en network_manager
    if (uploadFileToServer(uploadTarget, pixels)) {
      blinking_led_sequence(COLOR_VERDE, 3, 500, pixels);
      ESP.restart();
    } else {
      currentMode = IDLE;
    }
    break;
  }

  case BLE_TRANSFER_FILE: {
    pixels.clear();
    pixels.setPixelColor(0, COLOR_AZUL);
    pixels.show();

    if (transferTarget == "") {
      Serial.println("Error: No transfer target");
      currentMode = IDLE;
      break;
    }

    Serial.println("Iniciando transferencia BLE de: " + transferTarget);
    File f = SD.open(transferTarget.c_str(), FILE_READ);
    if (!f) {
      Serial.println("Error al abrir archivo para transferir");
      notifyFileEnd(); // Notificamos fin (o podríamos notificar error)
      delay(100);
      currentMode = IDLE;
      break;
    }

    size_t fSize = f.size();
    notifyFileStart(transferTarget, fSize);

    // Bucle de lectura y envío (Bloqueante por simplicidad, aunque idealmente
    // sería no bloqueante)
    while (f.available()) {
      // Leer línea completa
      String line = f.readStringUntil('\n');
      line.trim(); // Quitar posibles \r
      if (line.length() > 0) {
        notifyFileData(line);
      }
      // Alimentar Watchdog del ESP32
      yield();
    }

    f.close();
    notifyFileEnd();

    Serial.println("Transferencia completada.");
    transferTarget = ""; // Reset
    currentMode = IDLE;
    break;
  }
  }
}