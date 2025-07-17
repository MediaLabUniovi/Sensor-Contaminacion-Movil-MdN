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
//------------------------- Definición de constantes -----------------------
enum Mode: uint8_t {
    IDLE = 0,
    PROCESS_LONG_PRESS,
    DATA_RECOLECTION,
    WIFI_CONNECTION
};
volatile Mode currentMode  = IDLE;
Mode          previousMode = IDLE;
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
//SCL 22
//SDA 21
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
            if      (previousMode == IDLE)            currentMode = DATA_RECOLECTION;
            else if (previousMode == DATA_RECOLECTION) currentMode = WIFI_CONNECTION;
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

        case DATA_RECOLECTION: {
            bool newDataFlag = false;
            unsigned long chars;
            unsigned short sentences, failed;
            String data = "";     // Cadena que almacena los datos a escribir en la SD
            // Leemos el GPS durante 1 segundo
            Serial.println("[GPS] Leyendo GPS por Serial2 1s");
            uint32_t t0 = millis();
            for(; millis() - t0 < 1000; ){
              if (Serial2.available()) {
                char c = Serial2.read();
                if (GPS.encode(c)) {
                  Serial.println("[GPS] Frase NMEA decodificada, saliendo del bucle");
                  newDataFlag = true;
                  break;    // aquí haces el salto inmediato
                }
              }
            }
            Serial.println("\n[GPS] Finished reading serial.");
            Serial.print("[GPS] Satellite: ");
            Serial.println(GPS.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : GPS.satellites());
            if(!first_connect && GPS.satellites() != TinyGPS::GPS_INVALID_SATELLITES){
                first_connect = true;
                time_to_connect_ms = millis();
            }
            if(newDataFlag){ // Si hay datos GPS se decodifica la trama NMEA
                float lat, lon;         // Latitud y Longitud
                unsigned long age;      // Tiempo conectado
                int year;
                uint8_t month, day, hour, minute, second;
                Serial.println("[GPS] New Data Flag Recieved");
                GPS.f_get_position(&lat, &lon, &age);
                data.concat((String(lat)+";").c_str()); // Añadimos datos a la cadena
                data.concat((String(lon)+";").c_str());
                Serial.print("LAT=");       // Latitud
                Serial.print(lat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : lat, 6);
                Serial.print(" LON=");      // Longitud
                Serial.print(lon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : lon, 6);
                Serial.print(" SAT=");      // Satélite
                Serial.print(GPS.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : GPS.satellites());
                Serial.print(" PREC=");     // Precisión (HDOP)
                Serial.print(GPS.hdop() == TinyGPS::GPS_INVALID_HDOP ? 0 : GPS.hdop());
                Serial.println("");
                GPS.crack_datetime(&year, &month, &day, &hour, &minute, &second, NULL, NULL);
                hour = (hour + TIME_OFFSET_H) % 24; // Aplicamos el desfase horario para la hora local en España
                data.concat((String(year)+";").c_str()); // Añadimos datos a la cadena
                data.concat((String(month)+";").c_str());
                data.concat((String(day)+";").c_str());   
                data.concat((String(hour)+":"+String(minute)+":"+String(second)+";").c_str());      
                Serial.print("\n[GPS] DATE: "+String(day)+"/"+String(month)+"/"+String(year)); // Fecha
                Serial.println("; TIME: "+String(hour)+":"+String(minute)+":"+String(second)); // Hora
            }
            if(data != ""){ // Si obtenemos dato del GPS obtenemos datos de los sensores para agregarlos
                //---------------- sensor particulas ----------------//
                struct sps30_measurement pm;
                char serial[SPS30_MAX_SERIAL_LEN];
                uint16_t data_ready;
                int16_t ret;
                do {
                    ret = sps30_read_data_ready(&data_ready);
                    if (ret < 0) {
                        Serial.print("error reading data-ready flag: ");
                        Serial.println(ret);
                    } else if (!data_ready)
                        Serial.print("data not ready, no new measurement available\n");
                    else
                        break;
                    delay(100); /* retry in 100ms */
                } while (1);
                ret = sps30_read_measurement(&pm);
                if (ret < 0) {
                    Serial.print("error reading measurement\n");
                } else {
                    #ifndef PLOTTER_FORMAT
                    Serial.print("PM  2.5: ");
                    Serial.println(pm.mc_2p5);
                    Serial.print("PM 10.0: ");
                    Serial.println(pm.mc_10p0);
                    Serial.println();
                    #endif
                }
                //SPS30 (PM2.5)
                float pm2_5 = pm.mc_2p5;
                float pm_10 = pm.mc_10p0-pm.mc_2p5;
                //BME280 (Temp, Pres & Humedad)
                float temperatura(NAN), presion(NAN), humedad(NAN);
                temperatura = bme.readTemperature();  // Celsius
                presion = bme.readPressure() / 100.0F;  // Convertir a hPa
                humedad = bme.readHumidity();
                data.concat((String(temperatura)+";"+String(presion)+";"+String(humedad)+";").c_str());
                data.concat((String(pm2_5)+";"+String(pm_10)+"\n").c_str());
                Serial.println("[SD] Escribiendo datos en \"" + String(DATA_FILENAME) + "\"");
                appendFile(SD, DATA_FILENAME, data.c_str());  // Escribimos los datos en el fichero de la SD
                Serial.println("[SD] Datos escritos: " + data); 
                //---------------- debug leds ----------------//
                pixels.clear();
                if(pm2_5 <= 12.0) {
                    pixels.setPixelColor(1, COLOR_VERDE);
                } else if(pm2_5 <= 35.4) {
                    pixels.setPixelColor(1, COLOR_NARANJA);
                } else {
                    pixels.setPixelColor(1, COLOR_ROJO);
                }
                // LED 3 - PM10
                if(pm_10 <= 54.0) {
                    pixels.setPixelColor(2, COLOR_VERDE);
                } else if(pm_10 <= 154.0) {
                    pixels.setPixelColor(2, COLOR_NARANJA);
                } else {
                    pixels.setPixelColor(2, COLOR_ROJO);
                }
                // LED 4 - Estado del sistema
                bool sistema_ok = true;
                // Verificar SPS30
                if(ret < 0) {
                    pixels.setPixelColor(3, COLOR_ROJO);
                    sistema_ok = false;
                }
                // Verificar BME280
                else if(bme_ok) {
                    pixels.setPixelColor(3, COLOR_NARANJA);
                    sistema_ok = false;
                }
                // Verificar SD
                else if(!SD.begin(CS_PIN)) {
                    pixels.setPixelColor(3, COLOR_AMARILLO);
                    sistema_ok = false;
                }
                // Todo OK
                else {
                    pixels.setPixelColor(3, COLOR_AZUL);
                }    
                // Control del LED 1 según estado del GPS
                if(GPS.satellites() == TinyGPS::GPS_INVALID_SATELLITES) {
                    pixels.setPixelColor(0, COLOR_ROJO);
                } else {
                    pixels.setPixelColor(0, COLOR_VERDE);
                }
                pixels.show();
            } 
            GPS.stats(&chars, &sentences, &failed);   // Check de si se reciben datos del GPS
            if(chars == 0){
                Serial.println("[GPS] No se han recibido caracteres del GPS: comprobar cableado.");
            }
            delay(5000);
        break;
        }

        case WIFI_CONNECTION:
            
        break;

        default:

        break;
    }
}


void ledSequence() {
  switch (currentMode) {
    case IDLE:
      // Breve parpadeo amarillo en el LED 0
      for (uint8_t i = 0; i < 2; i++) {
        pixels.setPixelColor(0, COLOR_AMARILLO);
        pixels.show();
        delay(150);
        pixels.setPixelColor(0, 0);
        pixels.show();
        delay(150);
      }
      break;

    case DATA_RECOLECTION:
      // Carrera de un LED azul adelante y atrás
      for (int dir = 0; dir < 2; dir++) {
        if (dir == 0) {
          // adelante
          for (uint16_t i = 0; i < NUMPIXELS; i++) {
            pixels.clear();
            pixels.setPixelColor(i, COLOR_AZUL);
            pixels.show();
            delay(80);
          }
        } else {
          // atrás
          for (int16_t i = NUMPIXELS - 1; i >= 0; i--) {
            pixels.clear();
            pixels.setPixelColor(i, COLOR_AZUL);
            pixels.show();
            delay(80);
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
