/*
 * Código proyecto de Sensor de Contaminación Móvil Reto TICLAB Mar de Niebla 2025
 *
 * Autor: José Luis Muñiz Traviesas; 28/01/25
 */

// GPS
#include <SoftwareSerial.h>     // https://github.com/plerup/espsoftwareserial/
#include <TinyGPS.h>            // https://github.com/neosarchizo/TinyGPS
// Tarjeta SD
#ifndef SD_INCLUDES
#define SD_INCLUDES
#include "SD.h"                 // ESP32 SD (https://github.com/espressif/arduino-esp32/tree/master/libraries/SD)
#include "FS.h"                 // ESP32 FileSystem (https://github.com/espressif/arduino-esp32/tree/master/libraries/FS)
#include "SPI.h"                // ESP32 SPI (https://github.com/espressif/arduino-esp32/tree/master/libraries/SPI)
#endif  // SD_INCLUDES
// SPS30 (Sensor PM)
#include <sps30.h>              // https://github.com/Sensirion/arduino-sps
// BME280 (Temperatura, Presión & Humedad)
#include <Wire.h>       
#include <BME280I2C.h>          // https://github.com/finitespace/BME280
#define WIRE Wire       // Se puede cambiar a Wire, Wire1, ... 
// Funciones
#include "mdn_contaminacion.hpp"
//------------------------- Definición de constantes -----------------------
const int SOFTSERIAL_RX = 34;     // Conectar a pin TX del módulo GPS               
const int SOFTSERIAL_TX = 35;     // Conectar a pin RX del módulo GPS  
const uint32_t HARDWARE_BAUDRATE = 115200;      // Baudios Terminal Serie Hardware
const uint32_t SOFTWARE_BAUDRATE = 9600;        // Baudios Terminal Serie Software
const int8_t TIME_OFFSET_H = 1;  // Desfase de tiempo entre hora local y hora medida (+1 hora)
// BME280
const int BME_SDA = 16; // I2C SDA
const int BME_SCL = 17; // I2C SCL 
const BME280::TempUnit tempUnit(BME280::TempUnit_Celsius); // Temperatura en Celsius
const BME280::PresUnit presUnit(BME280::PresUnit_hPa);      // Presión en hectoPascales
// SD Card Pins (HSPI)
#define REASSIGN_PINS
const int SDI_PIN = 19;     // MISO
#define MISO_PIN  (SDI_PIN)
const int SDO_PIN = 23;     // MOSI
#define MOSI_PIN  (SDO_PIN)
const int CLK_PIN = 18;     // SCK
const int CS_PIN = 5;       // SS  
// Fichero tarjeta SD
const char* DATA_FILENAME = "/datos.txt"; // Nombre del fichero donde almacenaremos datos en la SD
//-------------------------- Variables globales ----------------------------
// BME280
BME280I2C BME;      // Objeto BME280 para comunicación I2C
bool humidity_flag = false;
// Módulo GPS
TinyGPS GPS;        // Objeto GPS        
SoftwareSerial SoftSerial(SOFTSERIAL_RX, SOFTSERIAL_TX);  // Objeto Puerto Serie Software (UART bit-banging)
uint32_t time_to_connect_ms = 0;   // Medida del tiempo de conexión a satélite
bool first_connect = false;     // Flag para establecer que ya ha habido una conexión
//--------------------------------------------------------------------------

void setup(){
    // Puerto serie hardware
    Serial.begin(HARDWARE_BAUDRATE);
    Serial.print("Hardware Serial baudrate: ");
    Serial.println(HARDWARE_BAUDRATE);
    // Puerto serie software
    SoftSerial.begin(SOFTWARE_BAUDRATE);
    Serial.print("Software Serial baudrate: ");
    Serial.println(SOFTWARE_BAUDRATE);
    // BME280
    WIRE.setPins(BME_SDA, BME_SCL);
    WIRE.begin();
    while(!BME.begin()){
        Serial.println("Could not find BME280 sensor!");
        delay(1000); 
    }
    switch(BME.chipModel()){
        case BME280::ChipModel_BME280:
            humidity_flag = true;
            Serial.println("[BME] BME280 encontrado. Humedad disponible");
            break;
        case BME280::ChipModel_BMP280:
            humidity_flag = false;
            Serial.println("[BME] BME280 no encontrado. Humedad no disponible");
            break;
        default:
            Serial.println("[BME] Sensor no reconocido. Error.");
    }
    // Tarjeta SD
#ifdef REASSIGN_PINS
    SPI.begin(CLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
    while(!SD.begin(CS_PIN)){
#else
    while(!SD.begin()){
#endif
        Serial.println("[SD] Montaje de tarjeta SD fallido.");
        delay(2000);    // Esperar 2s
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("[SD] No hay tarjeta SD.");
        exit(1);
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

    bool newDataFlag = false;
    uint32_t chars;
    unsigned short sentences, failed;
    String data = "";     // Cadena que almacena los datos a escribir en la SD

    // Leemos el GPS durante 1 segundo
    Serial.println("[GPS] Reading software serial for 1s");
    for(uint32_t start = millis(); millis() - start < 1000;){
        while(SoftSerial.available()){
            char c = (char)(SoftSerial.read());
            //Serial.write(c); // uncomment this line if you want to see the GPS data flowing
            if(GPS.encode(c)){ 
                Serial.println("[GPS] Sentence encoding successful");
                newDataFlag = true;
            }
        }
    }
    Serial.println("\n[GPS] Finished reading serial.");
    Serial.print("[GPS] Satellite: ");
    Serial.println(GPS.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : GPS.satellites());
    if((!first_connect) & (GPS.satellites() != TinyGPS::GPS_INVALID_SATELLITES) ){
        first_connect = true;
        time_to_connect_ms = millis();
    }
    if(newDataFlag){
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
        hour += TIME_OFFSET_H;  // Aplicamos el desfase horario para la hora local en España
        data.concat((String(year)+";").c_str()); // Añadimos datos a la cadena
        data.concat((String(month)+";").c_str());
        data.concat((String(day)+";").c_str());   
        data.concat((String(hour)+":"+String(minute)+":"+String(second)+";").c_str());      
        Serial.print("\n[GPS] DATE: "+String(day)+"/"+String(month)+"/"+String(year)); // Fecha
        Serial.println("; TIME: "+String(hour)+":"+String(minute)+":"+String(second)); // Hora
    }
    // Si obtenemos dato del GPS obtenemos datos de los sensores para agregarlos
    if(data != ""){
        //BME280 (Temp, Pres & Humedad)
        float temperatura(NAN), presion(NAN), humedad(NAN);
        temperatura = BME.temp(tempUnit);
        presion = BME.pres(presUnit);
        if(humidity_flag){
            humedad = BME.hum();
        }
        data.concat((String(temperatura)+";"+String(presion)+";"+String(humedad)+";").c_str());
        //SPS30 (PM2.5)
        float pm2_5 = NAN;  //?  Cambiar al tipo de dato de la librería usada para el SPS30
            //...           //TODO implementar escritura del sensor
        //Escritura de datos en la SD
        data.concat((String(pm2_5)+"\n").c_str()); //TODO escribir dato del sensor (Descomentar esta línea)
        Serial.println("[SD] Escribiendo datos en \"" + String(DATA_FILENAME) + "\"");
        appendFile(SD, DATA_FILENAME, data.c_str());  // Escribimos los datos en el fichero de la SD
        Serial.println("[SD] Datos escritos: " + data); 
    }
    GPS.stats(&chars, &sentences, &failed);   // Check de si se reciben datos del GPS
    if(chars == 0){
        Serial.println("[GPS] No se han recibido caracteres del GPS: comprobar cableado.");
    }
}
