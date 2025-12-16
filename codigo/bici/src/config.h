#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

//------------------------- Configuración WiFi --------------------------
extern const char* AP_NAME;
extern const char* PASSWORD;

//------------------------- Configuración Botón --------------------------
#define USER_BUTTON (4)
const unsigned long SAMPLE_INTERVAL_MS = 20;         // Intervalo de muestreo (ms)
const uint8_t       STABLE_SAMPLES_REQUIRED = 3;     // Lecturas consecutivas para confirmar cambio
#define LONG_PRESS_THRESHOLD_MS (2000)

//------------------------- Configuración Batería --------------------------
#define BATTERY_PIN (34)
const float VOLTAGE_DIVIDER_RATIO = 2.0;  // Divisor de tensión 1:1 (100k + 100k)
const float ADC_REFERENCE_VOLTAGE = 3.3;  // Voltaje de referencia del ADC
const int ADC_RESOLUTION = 4095;          // Resolución de 12 bits

//------------------------- Configuración GPS --------------------------
#define GPS_RX_PIN  (17)   // conecta al TX del módulo GPS
#define GPS_TX_PIN  (16)   // conecta al RX del módulo GPS
const uint32_t HARDWARE_BAUDRATE = 115200;      // Baudios Terminal Serie Hardware
const uint32_t GPS_BAUDRATE = 9600;             // Baudios Terminal Serie Software
const int8_t TIME_OFFSET_H = 1;                 // Desfase de tiempo entre hora local y hora medida (+1 hora)

//------------------------- Configuración BME280/BMP280 --------------------------
const int BME_SDA = 26; // I2C SDA
const int BME_SCL = 25; // I2C SCL

//------------------------- Configuración SD Card (HSPI) --------------------------
const int SDI_PIN = 23;     // MISO
#define MISO_PIN  (SDI_PIN)
const int SDO_PIN = 19;     // MOSI
#define MOSI_PIN  (SDO_PIN)
const int CLK_PIN = 18;     // SCK
const int CS_PIN = 5;       // SS
extern const char* DATA_FILENAME; // Nombre del fichero donde almacenaremos datos en la SD

//------------------------- Configuración LEDs NeoPixel --------------------------
#define PIN         (27)
#define NUMPIXELS   (5)        // Cantidad de LEDs
#define BRIGHTNESS  (50)       // Brillo de los LEDs (0-255)

// Definición de colores
#define COLOR_ROJO     0xFF0000
#define COLOR_VERDE    0x00FF00
#define COLOR_AZUL     0x0000FF
#define COLOR_NARANJA  0xFF8000
#define COLOR_AMARILLO 0xFFFF00

//------------------------- Configuración de Mediciones --------------------------
const uint32_t TIEMPO_ENTRE_MEDIDAS = 5 * 60 * 1000; // 5 minutos

//------------------------- Configuración SPS30 --------------------------
const uint8_t SPS30_AUTO_CLEAN_DAYS = 4;  // Días para limpieza automática del sensor

#endif // CONFIG_H

//scl 22 sda 21