/*
 * Código proyecto de Sensor de Contaminación Móvil Reto TICLAB Mar de Niebla 2025
 *
 * Autor: José Luis Muñiz Traviesas; 28/01/25
 */

// GPS
#include <SoftwareSerial.h>     // https://github.com/plerup/espsoftwareserial/
#include <TinyGPS.h>            // https://github.com/neosarchizo/TinyGPS
// SPS30 (Sensor PM)
#include <sps30.h>              // https://github.com/Sensirion/arduino-sps
// Tarjeta SD
#ifndef SD_INCLUDES
#define SD_INCLUDES
#include "FS.h"                 // ESP32 FileSystem (https://github.com/espressif/arduino-esp32/tree/master/libraries/FS)
#include "SD.h"                 // ESP32 SD (https://github.com/espressif/arduino-esp32/tree/master/libraries/SD)
#include "SPI.h"                // ESP32 SPI (https://github.com/espressif/arduino-esp32/tree/master/libraries/SPI)
#endif  // SD_INCLUDES
// Funciones
#include "mdn_contaminacion.hpp"
//------------------------- Definición de constantes -----------------------
const int SOFTSERIAL_RX = 34;     // Conectar a pin TX del módulo GPS               
const int SOFTSERIAL_TX = 35;     // Conectar a pin RX del módulo GPS  
const uint32_t HARDWARE_BAUDRATE = 115200;      // Baudios Terminal Serie Hardware
const uint32_t SOFTWARE_BAUDRATE = 9600;        // Baudios Terminal Serie Software
const int8_t TIME_OFFSET_H = 1;  // Desfase de tiempo entre hora local y hora medida (+1 hora)
// SD Card Pins (HSPI)
#define REASSIGN_PINS
const int SDI_PIN = 19;     // MISO
#define MISO_PIN    (SDI_PIN)
const int SDO_PIN = 23;     // MOSI
#define MOSI_PIN    (SDO_PIN)
const int CLK_PIN = 18;     // SCK
const int CS_PIN = 5;       // SS  
// Fichero tarjeta SD
const char* DATA_FILENAME = "/datos.txt"; // Nombre del fichero donde almacenaremos datos en la SD
//-------------------------- Variables globales ----------------------------
// Módulo GPS
TinyGPS GPS;                
SoftwareSerial SoftSerial(SOFTSERIAL_RX, SOFTSERIAL_TX);
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
    // Tarjeta SD
#ifdef REASSIGN_PINS
    SPI.begin(CLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
    while(!SD.begin(CS_PIN)) {
#else
    while(!SD.begin()) {
#endif
        Serial.println("Card Mount Failed");
        delay(2000);    // Esperar 2s
    }
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE) {
        Serial.println("No SD card attached");
        exit(1);
    }
    Serial.print("SD Card Type: ");
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
    Serial.println("SD Card Size: " + String(cardSize) + "MB");    
    // Fichero de datos
    File root = SD.open(DATA_FILENAME); // Comprobamos si existe el fichero de datos
    if(!root){  // Si no existe lo creamos y preparamos la primera línea
        Serial.println("Fichero \"" + String(DATA_FILENAME) + "\" no encontrado, creándolo...");
        writeFile(SD, DATA_FILENAME, "Latitud;Longitud;Año;Mes;Día;hh:mm:ss;PM2.5\n"); // Preparamos el fichero con la primera línea
    }
    else {
        Serial.println("Fichero \"" + String(DATA_FILENAME) + "\" ya creado.");
    }

}

void loop(){

    bool newDataFlag = false;
    uint32_t chars;
    unsigned short sentences, failed;
    String data = "";           // Cadena que almacena los datos a escribir en la SD

    // Leemos el GPS durante 1 segundo
    Serial.println("[*] Reading software serial for 1s");
    for(uint32_t start = millis(); millis() - start < 1000;){
        while(SoftSerial.available()){
            char c = (char)(SoftSerial.read());
            //Serial.write(c); // uncomment this line if you want to see the GPS data flowing
            if(GPS.encode(c)){ 
                Serial.println("[*] Sentence encoding successful");
                newDataFlag = true;
            }
        }
    }
    Serial.println("\n[*] Finished reading serial.");
    Serial.print("Satellite: ");
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
        Serial.println("[*] New Data Flag Recieved");
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
        Serial.print("\nDATE: ");
        Serial.print(day);
        Serial.print("/");
        Serial.print(month);
        Serial.print("/");
        Serial.print(year);
        Serial.print("; TIME: ");
        Serial.print(hour);
        Serial.print(":");
        Serial.print(minute);
        Serial.print(":");
        Serial.println(second); 
    }
    if(data != ""){
      //data.concat((String(pm2_5)+"\n").c_str()); //TODO escribir dato del sensor (Descomentar esta línea)
        data.concat("\n");      // (COMENTAR o BORRAR si se descomenta la de arriba)
        Serial.println("Escribiendo datos en \"" + String(DATA_FILENAME) + "\"");
        appendFile(SD, DATA_FILENAME, data.c_str());  // Escribimos los datos en el fichero 
    }
    GPS.stats(&chars, &sentences, &failed);
    if(chars == 0){
        Serial.println("** No characters received from GPS: check wiring **");
    }
}
