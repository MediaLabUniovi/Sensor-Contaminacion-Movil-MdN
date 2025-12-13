/*
 * Código proyecto de Sensor de Contaminación Móvil Reto TICLAB Mar de Niebla 2025
 *
 * Autor: José Luis Muñiz Traviesas, Miguel Enterría, Andrés Vilas Grela; 28/01/25
*/
// TODO: Cambiar secuencia de leds si se ve necesario, puede ser poco descriptivo para el usuario.
// TODO: Si pierde conexión GPS no es capaz de vilver a captarla (solo pasa a veces).
// TODO: Poner trapo separando sps30 y antena gps del resto del circuito
// GPS
#include "Arduino.h"
#include <TinyGPS.h>            // https://github.com/neosarchizo/TinyGPS
#include <time.h>               // Para manejo de RTC interno

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
#include <Adafruit_BMP280.h>

//LEDs
#include <Adafruit_NeoPixel.h>

//WiFi
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiManager.h>        // https://github.com/tzapu/WiFiManager
#include <WiFiClientSecure.h>

// Configuración
#include "config.h"

// Funciones
#include "functions.h"
#include "mdc_contaminacion.hpp"
#include "LongPress.h"
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
LongPressConfig lpCfg = {
  .buttonPin = USER_BUTTON,
  .sampleIntervalMs = SAMPLE_INTERVAL_MS,
  .stableSamplesRequired = STABLE_SAMPLES_REQUIRED,
  .longPressThresholdMs = LONG_PRESS_THRESHOLD_MS
};
LongPressState lpState;

//-------------------------- Variables globales ----------------------------
// Estados de sensores
bool bme_ok = false;
bool bmp_ok = false;
bool SD_ok = false;

// LEDs
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// BME280
TwoWire I2CBME(1);  // Usar bus I2C 1
Adafruit_BME280 bme; // Objeto BME280
Adafruit_BMP280 bmp(&I2CBME);
// Módulo GPS
TinyGPS GPS;        // Objeto GPS        
uint32_t time_to_connect_ms = 0;   // Medida del tiempo de conexión a satélite
bool first_connect = false;     // Flag para establecer que ya ha habido una conexión
bool rtc_synced = false;        // Flag para indicar si el RTC ya fue sincronizado con GPS
// Máquina de estados
volatile Mode currentMode  = IDLE;
volatile Mode previousMode = IDLE;
int16_t ret;
float pm2_5;
float pm_10;
//--------------------------------------------------------------------------

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
    pinMode(BATTERY_PIN, INPUT);  // Configurar pin de batería como entrada
    attachInterrupt(digitalPinToInterrupt(USER_BUTTON), isr_button_pressed, FALLING);
    
    // ----------------- INICIALIZAR SENSORES -----------------
    initBME280(bme, bmp, I2CBME, bme_ok, bmp_ok);
    delay(2000);
    
    // Sensor partículas
    initSPS30(ret);
    delay(500);
    
    // Tarjeta SD
    SD_ok = initSDCard();
    if (!SD_ok) {
        Serial.println("[SD] Error crítico: No se pudo inicializar la SD.");
        while(1) delay(1000);
    }        
}

void loop(){
    switch (currentMode) {
        case IDLE:
            static bool once = true;
            if (once){ // De esta forma nos aeguramos de que no se ejecuta este bloque de código más de una vez
              once = false;
              File root = SD.open(DATA_FILENAME); // Comprobamos si existe el fichero de datos
              if(!root){  // Si no existe lo creamos y preparamos la primera línea
                  Serial.println("[SD] Fichero \"" + String(DATA_FILENAME) + "\" no encontrado, creándolo...");
                  writeFile(SD, DATA_FILENAME, "Latitud;Longitud;Año;Mes;Día;hh:mm:ss;Temp;Pres;Hum;PM2.5;PM10;Bateria\n"); // Preparamos el fichero con la primera línea
              }
              else {
                  deleteFile(SD, DATA_FILENAME);
                  Serial.println("[SD] Fichero \"" + String(DATA_FILENAME) + "\" ya creado, borrandolo y creando uno nuevo.");
                  writeFile(SD, DATA_FILENAME, "Latitud;Longitud;Año;Mes;Día;hh:mm:ss;Temp;Pres;Hum;PM2.5;PM10;Bateria\n");
              }
              root.close(); // Después de crearlo se cierra 
            }
            pixels.setPixelColor(1, COLOR_VERDE);
            pixels.show();
        break;

        case PROCESS_LONG_PRESS: {
          LongPressResult res = checkLongPress(lpState, lpCfg);
          if (res == LONG_PRESS_DETECTED) {
            Serial.println(">> Pulsación larga detectada");
          
            // Decide transición
            Mode from = previousMode;
            Mode to   = IDLE;
            if      (from == IDLE)              to = DATA_RECOLLECTION;
            else if (from == DATA_RECOLLECTION) to = WIFI_CONNECTION;
            else                                to = IDLE;
          
            // Muestra la secuencia según el modo de origen
            ledSequence(from, pixels);
          
            // Aplica el nuevo modo
            currentMode = to;
          
            Serial.print("Cambiando a modo ");
            Serial.println(currentMode);
            attachInterrupt(digitalPinToInterrupt(USER_BUTTON), isr_button_pressed, FALLING);
          } else if (res == PRESS_CANCELLED) {
            Serial.println(">> Pulsación cancelada (cortita)");
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
                        
                        // Sincronizar RTC interno la primera vez que obtenemos datos GPS válidos
                        if (!rtc_synced) {
                            syncRTCWithGPS(year, month, day, hour, minute, second);
                            rtc_synced = true;
                        }
                        
                        // Obtener fecha y hora del RTC interno
                        dataBuffer += getCurrentDateString() + ";" + getCurrentTimeString() + ";";
                        dcStage     = DC_READ_BME;
                    } else {
                        // Si no hay GPS pero el RTC ya fue sincronizado, usar el RTC
                        if (rtc_synced) {
                            Serial.println("[GPS] Sin señal, usando RTC interno");
                            // Usar coordenadas 0,0 cuando no hay GPS
                            dataBuffer += "0.000000;0.000000;";
                            dataBuffer += getCurrentDateString() + ";" + getCurrentTimeString() + ";";
                            dcStage     = DC_READ_BME;
                        } else {
                            dcStage     = DC_UPDATE_LEDS;
                        }
                    }
                break;

                case DC_READ_BME: {
                    dataBuffer += readBMEData(bme, bmp, bme_ok, bmp_ok);
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
                    appendFile(SD, DATA_FILENAME, dataBuffer.c_str());
                    Serial.println("[SD] Datos escritos: " + dataBuffer);
                    dcStage     = DC_UPDATE_LEDS;
                break;
                
                case DC_UPDATE_LEDS: {
                    // Actualizo LEDs de debug (sin bloqueos)
                    if (newDataFlag){
                      updateStatusLEDs(pm2_5, pm_10, ret, bme_ok, bmp_ok, SD_ok, pixels);
                      dcStage = DC_WAIT;
                    } else { // Si no hay GPS secuencia de LEDS y volvemos a Ver si hay señal GPS
                      blinking_led_sequence(COLOR_ROJO, 3, 250, pixels);
                      dcStage = DC_TRY_AGAIN;
                    }
                    dcTimestamp = millis();
                    
                break;
                }
                
                case DC_WAIT:
                    // Esperamos 5 s antes de reiniciar la recolección
                    if (millis() - dcTimestamp >= TIEMPO_ENTRE_MEDIDAS) {
                        //% Si quisieramos dormir el micro sería aquí, luego al despertarse miramos la causa, si es por tiempo saltamos directamente al data recollection y si es por botón miraríamos si pasar a wifi o no.
                        Serial.println("Volvemos a IDLE");
                        dcStage = DC_IDLE ;           // volvemos al principio del flujo GPS→sensores
                    }
                break;

                case DC_TRY_AGAIN:
                    // Si venimos de un fallo de coenxión del GPS, solo esperamos 20 segundos.
                    if (millis() - dcTimestamp >= 20000) {
                        Serial.println("Volvemos a IDLE");
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
          pixels.clear();
          pixels.setPixelColor(0, COLOR_VERDE);
          pixels.setPixelColor(1, COLOR_VERDE);
          pixels.setPixelColor(2, COLOR_VERDE);
          pixels.show();
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
            blinking_led_sequence(COLOR_ROJO, 5, 500, pixels);
            break; //? estará bien?
          }
          Serial.println("Conectado correctamente a WiFi");
          currentMode  = SEND_DATA;
        break;
        }

        case SEND_DATA: {
          pixels.clear();
          pixels.setPixelColor(0, COLOR_VERDE);
          pixels.setPixelColor(1, COLOR_VERDE);
          pixels.setPixelColor(2, COLOR_VERDE);
          pixels.setPixelColor(3, COLOR_VERDE);
          pixels.show();
          // 1) Abre el fichero con los datos
          File csvFile = SD.open(DATA_FILENAME, FILE_READ);
          if (!csvFile) {
              Serial.println("Error abriendo datos para envío");
              currentMode = IDLE;
              break;
          }
          size_t fileSize = csvFile.size();
        
          // 2) Prepara delimitadores y C-strings
          static const char host[]     = "medialab-uniovi.es";
          static const uint16_t port   = 443;
          static const char path[]     = "/bike_pollution/upload.php";
          static const char boundary[] = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
        
          // Cabecera del multipart
          char preamble[256];
          int plen = snprintf(preamble, sizeof(preamble),
              "--%s\r\n"
              "Content-Disposition: form-data; name=\"csv\"; filename=\"datos.txt\"\r\n"
              "Content-Type: text/csv\r\n\r\n",
              boundary);
          if (plen < 0 || plen >= sizeof(preamble)) {
              Serial.println("Error construyendo preamble");
              csvFile.close();
              currentMode = IDLE;
              break;
          }
        
          // Cierre del multipart
          char ending[64];
          int elen = snprintf(ending, sizeof(ending),
              "\r\n--%s--\r\n",
              boundary);
          if (elen < 0 || elen >= sizeof(ending)) {
              Serial.println("Error construyendo ending");
              csvFile.close();
              currentMode = IDLE;
              break;
          }
        
          // 3) Calcula Content-Length
          size_t contentLength = (size_t)plen + fileSize + (size_t)elen;
        
          // 4) Conecta vía TLS
          WiFiClientSecure client;
          client.setInsecure();  // acepta cualquier certificado
          Serial.printf("Conectando a %s:%u…\n", host, port);
          if (!client.connect(host, port)) {
              Serial.println("Error al conectar TLS");
              csvFile.close();
              currentMode = IDLE;
              break;
          }
        
          // 5) Envía petición HTTP/1.1
          client.print("POST ");
          client.print(path);
          client.print(" HTTP/1.1\r\n");
          client.print("Host: ");
          client.print(host);
          client.print("\r\n");
          client.print("User-Agent: ESP32\r\n");
          client.print("Connection: close\r\n");
          client.print("Content-Type: multipart/form-data; boundary=");
          client.print(boundary);
          client.print("\r\n");
          client.print("Content-Length: ");
          client.print(contentLength);
          client.print("\r\n\r\n");
        
          // 6) Stream del cuerpo
          // 6a) Preamble
          client.write((const uint8_t*)preamble, plen);
        
          // 6b) CSV en bloques
          uint8_t buf[256];
          while (csvFile.available()) {
              size_t n = csvFile.read(buf, sizeof(buf));
              client.write(buf, n);
          }
          csvFile.close();
        
          // 6c) Ending
          client.write((const uint8_t*)ending, elen);
        
          // 7) Lee la respuesta del servidor
          Serial.println("Esperando respuesta…");
          // Espera la cabecera
          unsigned long timeout = millis() + 5000;
          while (!client.available() && millis() < timeout) {
              delay(10);
          }
          if (!client.available()) {
              Serial.println("Timeout leyendo respuesta");
              client.stop();
              currentMode = IDLE;
              break;
          }
        
          // Imprime toda la respuesta
          while (client.available()) {
              String line = client.readStringUntil('\n');
              Serial.println(line);
          }
        
          client.stop();
          blinking_led_sequence(COLOR_VERDE, 3, 500, pixels);          
          ESP.restart(); // Se reinicia y borra el archivo en setup, revisar si se quiere hacer así.
        break;
      }
    }
}