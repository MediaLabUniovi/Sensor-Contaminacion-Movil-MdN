/*
 * Código proyecto de Sensor de Contaminación Móvil Reto TICLAB Mar de Niebla 2025
 *
 * Autor: José Luis Muñiz Traviesas; 28/01/25
 */
// GPS
#include "Arduino.h"
#include <TinyGPS.h>            // https://github.com/neosarchizo/TinyGPS

// Tarjeta SD
#ifndef SD_INCLUDES
#define SD_INCLUDES
#include "SD.h"                 // ESP32 SD
#include "FS.h"                 // ESP32 FileSystem
#include "SPI.h"                // ESP32 SPI
#endif  // SD_INCLUDES

// SPS30 (Sensor PM)
#include <sps30.h>              // https://github.com/Sensirion/arduino-sps

// BME280 (Temperatura, Presión & Humedad)
#include <Wire.h>    
#include <Adafruit_BME280.h>   

//LEDs
#include <Adafruit_NeoPixel.h>

//WiFi
#include <WiFiManager.h>        //https://github.com/tzapu/WiFiManager

// Funciones
#include "mdc_contaminacion.hpp"
#include "LongPress.h"
//-------------------------- Definición de enums --------------------------
enum Mode: uint8_t { // Enumeración para la máquina de estados general
    IDLE = 0,
    PROCESS_LONG_PRESS,
    DATA_RECOLLECTION,
    WIFI_CONNECTION,
    SEND_DATA
};
enum DataRecolectStage : uint8_t { // Enumeración para recoleción de datos
  DC_IDLE = 0,
  DC_READ_GPS,
  DC_PROCESS_GPS,
  DC_READ_PARTICLES,
  DC_WRITE_SD,
  DC_UPDATE_LEDS,
  DC_WAIT
};
//------------------------- Definición de constantes -----------------------
// WiFi
const char* AP_NAME = "SensorMovil";
const char* PASSWORD = "RetoTicLab";
// Botón
#define USER_BUTTON (25)
// Parámetros de muestreo y pulsación larga
const unsigned long SAMPLE_INTERVAL_MS = 20;         // Intervalo de muestreo (ms)
const uint8_t       STABLE_SAMPLES_REQUIRED = 3;     // Lecturas consecutivas para confirmar cambio
#define LONG_PRESS_THRESHOLD_MS (2000)
// Configuración
LongPressConfig lpCfg = {
  .buttonPin = USER_BUTTON,
  .sampleIntervalMs = SAMPLE_INTERVAL_MS,
  .stableSamplesRequired = STABLE_SAMPLES_REQUIRED,
  .longPressThresholdMs = LONG_PRESS_THRESHOLD_MS
};
LongPressState lpState;
// Pines para GPS sobre UART2
#define GPS_RX_PIN  (17)   // conecta al TX del módulo GPS
#define GPS_TX_PIN  (16)   // conecta al RX del módulo GPS
const uint32_t HARDWARE_BAUDRATE = 115200;      // Baudios Terminal Serie Hardware
const uint32_t GPS_BAUDRATE = 9600;        // Baudios Terminal Serie Software
const int8_t TIME_OFFSET_H = 1;  // Desfase de tiempo entre hora local y hora medida (+1 hora)
// BME280
const int BME_SDA = 14; // I2C SDA
const int BME_SCL = 13; // I2C SCL 
bool bme_ok = false;
// SD Card Pins (HSPI)
const int SDI_PIN = 19;     // MISO
#define MISO_PIN  (SDI_PIN)
const int SDO_PIN = 23;     // MOSI
#define MOSI_PIN  (SDO_PIN)
const int CLK_PIN = 18;     // SCK
const int CS_PIN = 5;       // SS  
//LEDs
#define PIN         (27)
#define NUMPIXELS   (5)    // Cantidad de LEDs
#define BRIGHTNESS  (50)    // Brillo de los LEDs (0-255)
// Definición de colores
#define COLOR_ROJO     0xFF0000
#define COLOR_VERDE    0x00FF00
#define COLOR_AZUL     0x0000FF
#define COLOR_NARANJA  0xFF8000
#define COLOR_AMARILLO 0xFFFF00
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
// Fichero tarjeta SD
const char* DATA_FILENAME = "/datos.txt"; // Nombre del fichero donde almacenaremos datos en la SD
//-------------------------- Variables globales ----------------------------
// BME280
TwoWire I2CBME(1);  // Usar bus I2C 1
Adafruit_BME280 bme; // Objeto BME280
// Módulo GPS
TinyGPS GPS;        // Objeto GPS        
uint32_t time_to_connect_ms = 0;   // Medida del tiempo de conexión a satélite
bool first_connect = false;     // Flag para establecer que ya ha habido una conexión
// Máquina de estados
volatile Mode currentMode  = IDLE;
Mode          previousMode = IDLE;
//--------------------------------------------------------------------------

void ledSequence();

void IRAM_ATTR isr_button_pressed(){
    detachInterrupt(digitalPinToInterrupt(USER_BUTTON));
    previousMode = currentMode;
    currentMode  = PROCESS_LONG_PRESS;
}

void setup(){
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
    attachInterrupt(digitalPinToInterrupt(USER_BUTTON), isr_button_pressed, FALLING);
    // ----------------- INICIALIZAR BME280 EN Wire1 -----------------
    I2CBME.setPins(BME_SDA, BME_SCL);
    I2CBME.begin();
    if (!bme.begin(0x76, &I2CBME) && !bme.begin(0x77, &I2CBME)) {  // 0x76 es dirección I2C común
        Serial.println("Could not find BME280 sensor!");
        while(1) delay(1000);
    }
    bme_ok = true;
    Serial.println("[BME] BME280 encontrado. Humedad disponible");
    delay(2000);
    // ---------------------- sensor particulas ----------------------
    int16_t ret;
    uint8_t auto_clean_days = 4;
    uint32_t auto_clean;
    sensirion_i2c_init();
    while (sps30_probe() != 0) {
        Serial.print("SPS sensor probing failed\n");
        delay(500);
    }
    #ifndef PLOTTER_FORMAT
    Serial.print("SPS sensor probing successful\n");
    #endif /* PLOTTER_FORMAT */
    ret = sps30_set_fan_auto_cleaning_interval_days(auto_clean_days);
    if (ret) {
        Serial.print("error setting the auto-clean interval: ");
        Serial.println(ret);
    }
    ret = sps30_start_measurement();
    if (ret < 0) {
        Serial.print("error starting measurement\n");
    }
    #ifndef PLOTTER_FORMAT
    Serial.print("measurements started\n");
    #endif /* PLOTTER_FORMAT */
    delay(500);
    // Tarjeta SD
    SPI.begin(CLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
    while(!SD.begin(CS_PIN)){
    Serial.println("[SD] Montaje de tarjeta SD fallido.");
    delay(2000);    // Esperar 2s
    }
    uint8_t cardType = SD.cardType();
    if(cardType == CARD_NONE){
        Serial.println("[SD] No hay tarjeta SD.");
        while(1) delay(1000);
    }
    Serial.print("[SD] SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);    // Megabytes
    Serial.println("[SD] SD Card Size: " + String(cardSize) + "MB");    
    // Fichero de datos
    File root = SD.open(DATA_FILENAME); // Comprobamos si existe el fichero de datos
    if(!root){  // Si no existe lo creamos y preparamos la primera línea
        Serial.println("[SD] Fichero \"" + String(DATA_FILENAME) + "\" no encontrado, creándolo...");
        writeFile(SD, DATA_FILENAME, "Latitud;Longitud;Año;Mes;Día;hh:mm:ss;Temp;Pres;Hum;PM2.5\n"); // Preparamos el fichero con la primera línea
    }
    else {
        Serial.println("[SD] Fichero \"" + String(DATA_FILENAME) + "\" ya creado.");
    }
}

void loop(){
    switch (currentMode) {
        case IDLE:
            
        break;

        case PROCESS_LONG_PRESS: {
            // Cada iteración medimos si hubo long‑press o si canceló (release antes de umbral)
            LongPressResult res = checkLongPress(lpState, lpCfg);
            if (res == LONG_PRESS_DETECTED) {
              Serial.println(">> Pulsación larga detectada");
              ledSequence();
              // Transición según el modo anterior
              if      (previousMode == IDLE)            currentMode = DATA_RECOLLECTION;
              else if (previousMode == DATA_RECOLLECTION) currentMode = WIFI_CONNECTION;
              else                                      currentMode = IDLE;

              Serial.print("Cambiando a modo ");
              Serial.println(currentMode);
              // Rehabilita la ISR para la próxima pulsación
              attachInterrupt(digitalPinToInterrupt(USER_BUTTON), isr_button_pressed, FALLING);
            }
            else if (res == PRESS_CANCELLED) {
              Serial.println(">> Pulsación cancelada (cortita)");
              // Vuelves al modo previo
              currentMode = previousMode;
              Serial.print("Volviendo a modo ");
              Serial.println(currentMode);
              attachInterrupt(digitalPinToInterrupt(USER_BUTTON), isr_button_pressed, FALLING);
            }
        break;
        }

        case DATA_RECOLLECTION: {
            // Variables estáticas para este caso
            static uint8_t  dcStage       = 0;
            static uint32_t dcTimestamp   = 0;
            static bool     newDataFlag   = false;
            static String   dataBuffer    = "";

            // Permitir detectar un nuevo long‑press en cualquier momento
            LongPressResult lpRes = checkLongPress(lpState, lpCfg);
            if (lpRes == LONG_PRESS_DETECTED) {
              // Salimos inmediatamente para reentrar en PROCESS_LONG_PRESS
              Serial.println(">> Long-press dentro de DATA_RECOLECTION, interrumpiendo");
              currentMode = PROCESS_LONG_PRESS;
              attachInterrupt(digitalPinToInterrupt(USER_BUTTON),
                              isr_button_pressed,
                              FALLING);
              // Reinciamos etapa para la próxima vez
              dcStage = DC_IDLE ;
              break;
            }
        
            switch (dcStage) {
                case DC_IDLE :
                    Serial.println("[GPS] Iniciando lectura 1s...");
                    newDataFlag = false;
                    dataBuffer  = "";
                    dcTimestamp = millis();
                    dcStage     = DC_READ_GPS;
                break;
                
                case DC_READ_GPS:
                    // 1 segundo de “muestreo” GPS
                    while (Serial2.available()) {
                      char c = Serial2.read();
                      if (GPS.encode(c)) {
                        Serial.println("[GPS] NMEA decodificada antes de timeout");
                        newDataFlag = true;
                        dcStage     = DC_PROCESS_GPS;
                        break;
                      }
                    }
                    if (millis() - dcTimestamp >= 1000) {
                      Serial.println("[GPS] Timeout lectura GPS");
                      dcStage = DC_PROCESS_GPS;
                    }
                break;
                
                case DC_PROCESS_GPS:
                    // Procesamos datos GPS (si los hay)
                    if (newDataFlag) {
                      float lat, lon; unsigned long age;
                      int year; uint8_t month, day, hour, minute, second;
                      GPS.f_get_position(&lat, &lon, &age);
                      dataBuffer += String(lat,6) + ";" + String(lon,6) + ";";
                      GPS.crack_datetime(&year,&month,&day,&hour,&minute,&second,NULL,NULL);
                      hour = (hour + TIME_OFFSET_H) % 24;
                      dataBuffer += String(year) + ";" + String(month) + ";" + String(day) + ";"
                                  + String(hour) + ":" + String(minute) + ":" + String(second) + ";";
                    }
                    dcStage     = DC_READ_PARTICLES;
                break;
                
                case DC_READ_PARTICLES: {
                    // SPS30: esperamos data_ready sin delay()
                    uint16_t ready = 0;
                    int16_t ret = sps30_read_data_ready(&ready);
                    if (ret < 0) {
                      Serial.printf("error data_ready: %d\n", ret);
                      dcStage = DC_WRITE_SD;  // saltamos al siguiente paso para no bloquear aquí
                    }
                    else if (ready) {
                      struct sps30_measurement pm;
                      ret = sps30_read_measurement(&pm);
                      if (ret >= 0) {
                        dataBuffer += String(pm.mc_2p5) + ";" 
                                    + String(pm.mc_10p0 - pm.mc_2p5) + "\n";
                      }
                      dcStage = DC_WRITE_SD;
                    }
                    // si no está ready, nos quedamos en 3 hasta que lo esté
                break;
                }
              
                case DC_WRITE_SD:
                    // Lectura BME280 y escritura en SD
                    // Aquí dataBuffer ya contiene: "lat;lon;yyyy;m;d;hh:mm:ss;" + "pm2.5;pm10\n"
                    // Solo falta intercalar BME entre GPS y PM:
                    if (bme_ok) {
                      float t = bme.readTemperature();
                      float p = bme.readPressure() / 100.0F;
                      float h = bme.readHumidity();
                      // Separamos la parte de PM al final (antes del '\n')
                      int newlinePos = dataBuffer.lastIndexOf('\n');
                      String pmPart   = dataBuffer.substring(dataBuffer.lastIndexOf(';') + 1); // e.g. "pm10\n"
                      String gpsPart  = dataBuffer.substring(0, dataBuffer.indexOf(';', dataBuffer.indexOf(';', dataBuffer.indexOf(';', dataBuffer.indexOf(';', dataBuffer.indexOf(';')+1)+1)+1)+1)+1);
                      // Pero en realidad es más sencillo: dataBuffer = GPSpart + BMEpart + PMpart
                      // Dado que dataBuffer = "<GPS>;<PM2.5>;<PM10>\n"
                      // lo partimos en trozos:
                      int firstPMsep = dataBuffer.indexOf(';', dataBuffer.indexOf(';', dataBuffer.indexOf(';', dataBuffer.indexOf(';')+1)+1)+1); 
                      String gpsStr  = dataBuffer.substring(0, firstPMsep + 1);            // de inicio hasta el separador antes de PM2.5
                      String pmStr   = dataBuffer.substring(firstPMsep + 1);               // desde PM2.5 en adelante
                      // Construimos: GPS + BME + PM
                      dataBuffer = gpsStr
                                 + String(t, 2) + ";" + String(p, 2) + ";" + String(h, 2) + ";"
                                 + pmStr;
                    }
                    appendFile(SD, DATA_FILENAME, dataBuffer.c_str());
                    Serial.println("[SD] Datos escritos: " + dataBuffer);
                    dcTimestamp = millis();
                    dcStage     = DC_UPDATE_LEDS;
                break;
                
                case DC_UPDATE_LEDS:
                    // Actualizo LEDs de debug (sin bloqueos)
                    pixels.clear();
                    // TODO: volver a añadir LEDs
                    // Ejemplo simplificado: LED1 según PM2.5, LED2 PM10, LED3 estado sistema, LED0 GPS
                    // (aquí podrías reutilizar tu código original de colores)
                    pixels.show();
                    dcStage = DC_WAIT;
                break;
                
                case DC_WAIT:
                    // Esperamos 5 s antes de reiniciar la recolección
                    if (millis() - dcTimestamp >= 5000) {
                      dcStage = DC_IDLE ;           // volvemos al principio del flujo GPS→sensores
                    }
                break;
            }
        break;
        }

        case WIFI_CONNECTION: {
            /*
                * WiFiManager con detección de error
            */
            WiFi.mode(WIFI_STA);
            WiFiManager wm;
            const int MAX_INTENTOS_WIFI = 3;
            int intentos = 0;
            // Intenta conectar automáticamente con las redes guardadas
            bool conectado = wm.autoConnect(AP_NAME, PASSWORD);
            while (!conectado && intentos < MAX_INTENTOS_WIFI) {
              Serial.printf("Intento de conexión fallido #%d\n", intentos + 1);
              delay(100);
              conectado = (WiFi.status() == WL_CONNECTED);
              intentos++;
              Serial.printf("Intentos: %d",intentos);
            }
            if (!conectado) {
              Serial.println("No se pudo conectar tras varios intentos. Borrando configuración WiFi.");
              wm.resetSettings(); // borra SSID/password de la NVS
              delay(1000);
              ESP.restart();  
            }
            Serial.println("Conectado correctamente a WiFi");
            currentMode  = SEND_DATA;
        break;
        }   
          
        case SEND_DATA:
            Serial.println("Mandando datos");
            delay(1000);
        break;
    }
}


void ledSequence() {
  switch (currentMode) {
    case IDLE: // Para pasar a DATA_RECOLLECTION
      // Breve parpadeo amarillo en el LED 0
      for (uint8_t i = 0; i < 2; i++) {
        pixels.setPixelColor(0, COLOR_AMARILLO);
        pixels.show();
        delay(500);
        pixels.setPixelColor(0, 0);
        pixels.show();
        delay(500);
      }
      break;

    case DATA_RECOLLECTION: // Para pasat a WIFI_CONNECTION
      // Carrera de un LED azul adelante y atrás
      for (int dir = 0; dir < 2; dir++) {
        if (dir == 0) {
          // adelante
          for (uint16_t i = 0; i < NUMPIXELS; i++) {
            pixels.clear();
            pixels.setPixelColor(i, COLOR_AZUL);
            pixels.show();
            delay(250);
          }
        } else {
          // atrás
          for (int16_t i = NUMPIXELS - 1; i >= 0; i--) {
            pixels.clear();
            pixels.setPixelColor(i, COLOR_AZUL);
            pixels.show();
            delay(250);
          }
        }
      }
      break;

    case WIFI_CONNECTION:
      // Parpadeo verde/rojo alterno en toda la tira
      for (uint8_t k = 0; k < 4; k++) {
        // verde
        for (uint16_t i = 0; i < NUMPIXELS; i++) {
          pixels.setPixelColor(i, COLOR_VERDE);
        }
        pixels.show();
        delay(200);
        // rojo
        for (uint16_t i = 0; i < NUMPIXELS; i++) {
          pixels.setPixelColor(i, COLOR_ROJO);
        }
        pixels.show();
        delay(200);
      }
      break;

    default:
      // Vuelve al estado “LED 0 verde encendido”
      pixels.clear();
      pixels.setPixelColor(0, COLOR_VERDE);
      pixels.show();
      break;
  }
}
